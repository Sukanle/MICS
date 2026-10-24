#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import datetime as dt
import os
import shutil
import subprocess
import sys
from pathlib import Path

DEFAULT_COUNTS = [1000, 2000, 4000, 8000, 10000]


def default_generator() -> str:
    if shutil.which("ninja"):
        return "Ninja"
    return "MinGW Makefiles" if os.name == "nt" else "Unix Makefiles"


def run_cmd(cmd: list[str], cwd: Path, allow_failure: bool = False) -> None:
    proc = subprocess.run(cmd, cwd=str(cwd), text=True)
    if proc.returncode != 0 and not allow_failure:
        raise RuntimeError(f"Command failed ({proc.returncode}): {' '.join(cmd)}")


def parse_key_value_csv(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    with path.open("r", encoding="utf-8", newline="") as fp:
        reader = csv.reader(fp)
        _ = next(reader, None)
        for row in reader:
            if len(row) >= 2:
                data[row[0]] = row[1]
    return data


def newest_stress_csv(report_dir: Path) -> Path:
    files = sorted(report_dir.glob("stress_*.csv"), key=lambda p: p.stat().st_mtime)
    if not files:
        raise RuntimeError(f"No stress_*.csv found in {report_dir}")
    return files[-1]


def parse_counts(raw: str) -> list[int]:
    values = [int(x.strip()) for x in raw.split(",") if x.strip()]
    if not values:
        raise ValueError("counts cannot be empty")
    for value in values:
        if value <= 0:
            raise ValueError("counts must be positive")
    return values


def classify_growth(records: list[dict[str, str]]) -> tuple[str, list[str]]:
    notes: list[str] = []
    ratios: list[float] = []
    for i in range(1, len(records)):
        prev_n = int(records[i - 1]["class_count"])
        curr_n = int(records[i]["class_count"])
        prev_t = int(records[i - 1]["build_ns"])
        curr_t = int(records[i]["build_ns"])
        if prev_n <= 0 or prev_t <= 0:
            continue
        count_ratio = curr_n / prev_n
        time_ratio = curr_t / prev_t
        normalized = time_ratio / count_ratio
        ratios.append(normalized)
        notes.append(
            f"{prev_n}->{curr_n}: time_ratio={time_ratio:.3f}, count_ratio={count_ratio:.3f}, normalized={normalized:.3f}"
        )

    if not ratios:
        return "insufficient-data", notes

    avg = sum(ratios) / len(ratios)
    if avg <= 1.30:
        return "near-linear", notes
    if avg <= 1.80:
        return "super-linear", notes
    return "possible-exponential", notes


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run class-count scaling benchmark for reflection stress test."
    )
    parser.add_argument(
        "--counts",
        default=",".join(str(x) for x in DEFAULT_COUNTS),
        help="Comma-separated class counts",
    )
    parser.add_argument("--fields-per-class", type=int, default=100)
    parser.add_argument("--inheritance-chain", type=int, default=10)
    parser.add_argument("--workspace", default=".")
    parser.add_argument("--output-dir", default="stress_project_test")
    parser.add_argument(
        "--generator", default=default_generator(), help="CMake generator"
    )
    parser.add_argument("--build-type", default="Release", help="CMake build type")
    parser.add_argument("--build-dir", default="build", help="Build directory")
    parser.add_argument(
        "--binary-name", default="reflect_stress", help="Built binary name"
    )
    parser.add_argument(
        "--clean-build",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Delete the build directory before each count",
    )
    parser.add_argument("--dump-runtime-metadata", action="store_true")
    parser.add_argument("--print-raw-output", action="store_true")
    parser.add_argument("--ftime-trace", action="store_true")
    parser.add_argument("--ubsan", action="store_true")
    args = parser.parse_args()

    counts = parse_counts(args.counts)
    workspace = Path(args.workspace).resolve()
    output_dir = (workspace / args.output_dir).resolve()
    report_dir = output_dir / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)

    gen_script = workspace / "tools" / "generate_stress_project.py"
    runner_script = output_dir / "run_stress.py"

    rows: list[dict[str, str]] = []

    for count in counts:
        gen_cmd = [
            sys.executable,
            str(gen_script),
            "--count",
            str(count),
            "--fields-per-class",
            str(args.fields_per_class),
            "--inheritance-chain",
            str(args.inheritance_chain),
            "--output",
            str(output_dir),
        ]
        run_cmd(gen_cmd, workspace)

        build_dir = output_dir / args.build_dir
        if args.clean_build and build_dir.exists():
            shutil.rmtree(build_dir)

        run_cmdline = [
            sys.executable,
            str(runner_script),
            "--generator",
            args.generator,
            "--build-type",
            args.build_type,
            "--build-dir",
            args.build_dir,
            "--binary-name",
            args.binary_name,
            "--print-summary",
        ]
        if args.print_raw_output:
            run_cmdline.append("--print-raw-output")
        if args.dump_runtime_metadata:
            run_cmdline.append("--dump-runtime-metadata")
        if args.ftime_trace:
            run_cmdline.append("--ftime-trace")
        if args.ubsan:
            run_cmdline.extend(
                [
                    "--cxx-flags",
                    "-fsanitize=undefined",
                    "--linker-flags",
                    "-fsanitize=undefined",
                ]
            )

        run_cmd(run_cmdline, workspace, allow_failure=True)

        stress_csv = newest_stress_csv(report_dir)
        kv = parse_key_value_csv(stress_csv)

        build_ns = int(kv.get("build_ns", "0") or 0)
        build_ms = build_ns / 1_000_000.0
        binary_path = kv.get("binary_path", "NA")
        binary_size = int(kv.get("binary_size_bytes", "-1") or -1)

        if binary_size < 0 and binary_path != "NA":
            candidate = Path(binary_path)
            if candidate.exists():
                binary_size = candidate.stat().st_size

        row = {
            "timestamp": kv.get("timestamp", "NA"),
            "class_count": str(count),
            "fields_per_class": str(args.fields_per_class),
            "inheritance_chain": str(args.inheritance_chain),
            "build_type": args.build_type,
            "dump_runtime_metadata": "1" if args.dump_runtime_metadata else "0",
            "build_ns": str(build_ns),
            "build_ms": f"{build_ms:.3f}",
            "run_exit": kv.get("run_exit", "NA"),
            "verification": kv.get("verification", "NA"),
            "metadata_errors": kv.get("metadata_errors", "NA"),
            "runtime_meta_report": kv.get("runtime_meta_report", "NA"),
            "binary_name": args.binary_name,
            "binary_path": binary_path,
            "binary_size_bytes": str(binary_size),
            "stress_csv": str(stress_csv),
        }
        rows.append(row)

    verdict, notes = classify_growth(rows)

    ts = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    out_csv = report_dir / f"scaling_summary_{ts}.csv"
    out_txt = report_dir / f"scaling_summary_{ts}.txt"

    with out_csv.open("w", encoding="utf-8", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    with out_txt.open("w", encoding="utf-8") as fp:
        fp.write("Scaling benchmark summary\n")
        fp.write(f"counts={counts}\n")
        fp.write(f"fields_per_class={args.fields_per_class}\n")
        fp.write(f"inheritance_chain={args.inheritance_chain}\n")
        fp.write(f"generator={args.generator}\n")
        fp.write(f"build_type={args.build_type}\n")
        fp.write(f"build_dir={output_dir / args.build_dir}\n")
        fp.write(f"binary_name={args.binary_name}\n")
        fp.write(f"dump_runtime_metadata={1 if args.dump_runtime_metadata else 0}\n")
        fp.write(f"ftime_trace={1 if args.ftime_trace else 0}\n")
        fp.write(f"ubsan={1 if args.ubsan else 0}\n")
        fp.write(f"growth_verdict={verdict}\n")
        fp.write("\nRatios:\n")
        for note in notes:
            fp.write(f"- {note}\n")
        fp.write("\nManual checks:\n")
        fp.write(
            "- IDE indexing speed: open generated_classes.hpp in VS/CLion and observe completion latency.\n"
        )
        fp.write(
            "- ftime-trace flame graph: open generated .json in Chrome tracing UI.\n"
        )
        fp.write(
            "- Memory peak: monitor compiler process in Task Manager during build.\n"
        )

    print(f"[OK] Scaling CSV: {out_csv}")
    print(f"[OK] Scaling TXT: {out_txt}")
    print(f"[OK] Growth verdict: {verdict}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
