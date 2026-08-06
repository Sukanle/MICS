#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import math
import os
import platform
import subprocess
import time
from pathlib import Path


def timestamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d_%H%M%S")


def run_cmd(cmd: list[str], cwd: Path, print_raw_output: bool) -> tuple[int, str, int]:
    begin_ns = time.perf_counter_ns()
    proc = subprocess.run(
        cmd,
        cwd=str(cwd),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    end_ns = time.perf_counter_ns()
    output = (proc.stdout or "") + (proc.stderr or "")
    if print_raw_output and output:
        print(output, end="" if output.endswith("\n") else "\n")
    return proc.returncode, output, end_ns - begin_ns


def parse_runtime_output(output: str) -> dict[str, str]:
    mapping = {
        "Validated classes": "validated_classes",
        "Metadata errors": "metadata_errors",
        "Runtime validation total ns": "runtime_validation_total_ns",
        "Runtime metadata check ns": "runtime_metadata_check_ns",
        "Runtime function invoke ns": "runtime_value_check_ns",
        "Runtime value check ns": "runtime_value_check_ns",
        "Runtime overhead ns": "runtime_overhead_ns",
        "Per class avg ns": "per_class_avg_ns",
        "Runtime metadata report": "runtime_meta_report",
        "Runtime metadata enabled": "runtime_metadata_enabled",
    }
    result: dict[str, str] = {}
    for raw_line in output.splitlines():
        if ":" not in raw_line:
            continue
        key, value = raw_line.split(":", 1)
        key = key.strip()
        value = value.strip()
        if key in mapping:
            result[mapping[key]] = value
    return result


def resolve_binary_path(build_dir: Path, binary_name: str) -> Path:
    suffix = ".exe" if os.name == "nt" and not binary_name.endswith(".exe") else ""
    return build_dir / f"{binary_name}{suffix}"


def write_summary_csv(path: Path, rows: list[tuple[str, str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as fp:
        writer = csv.writer(fp)
        writer.writerow(["key", "value"])
        for key, value in rows:
            writer.writerow([key, value])


def read_summary_csv(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    with path.open("r", encoding="utf-8", newline="") as fp:
        reader = csv.reader(fp)
        _ = next(reader, None)
        for row in reader:
            if len(row) < 2:
                continue
            data[row[0]] = row[1]
    return data


def to_int(value: str, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def timestamp_to_dt(raw: str) -> dt.datetime | None:
    try:
        return dt.datetime.strptime(raw, "%Y%m%d_%H%M%S")
    except ValueError:
        return None


def build_trend_svg(report_dir: Path, svg_path: Path, aggregate_csv_path: Path) -> None:
    stress_files = sorted(report_dir.glob("stress_*.csv"))
    points: list[dict[str, object]] = []
    for csv_file in stress_files:
        data = read_summary_csv(csv_file)
        ts = data.get("timestamp", "")
        ts_dt = timestamp_to_dt(ts)
        if ts_dt is None:
            continue
        build_ns = to_int(data.get("build_ns", ""), -1)
        if build_ns < 0:
            continue
        dump_meta = to_int(data.get("dump_runtime_metadata", "0"), 0)
        run_exit = to_int(data.get("run_exit", "1"), 1)
        points.append(
            {
                "timestamp": ts,
                "datetime": ts_dt,
                "build_ns": build_ns,
                "build_ms": build_ns / 1_000_000.0,
                "dump_runtime_metadata": dump_meta,
                "run_exit": run_exit,
            }
        )

    points.sort(key=lambda item: item["datetime"])

    with aggregate_csv_path.open("w", encoding="utf-8", newline="") as fp:
        writer = csv.writer(fp)
        writer.writerow(["timestamp", "build_ns", "build_ms", "dump_runtime_metadata", "run_exit"])
        for p in points:
            writer.writerow(
                [
                    p["timestamp"],
                    p["build_ns"],
                    f"{p['build_ms']:.6f}",
                    p["dump_runtime_metadata"],
                    p["run_exit"],
                ]
            )

    width = 1200
    height = 560
    margin_left = 90
    margin_right = 30
    margin_top = 50
    margin_bottom = 90
    plot_w = width - margin_left - margin_right
    plot_h = height - margin_top - margin_bottom

    if not points:
        empty_svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">
<rect x="0" y="0" width="{width}" height="{height}" fill="#ffffff"/>
<text x="{width // 2}" y="{height // 2}" text-anchor="middle" font-size="20" fill="#333333">No valid stress_*.csv data</text>
</svg>
"""
        svg_path.write_text(empty_svg, encoding="utf-8")
        return

    x_vals = [p["datetime"].timestamp() for p in points]
    y_vals = [float(p["build_ms"]) for p in points]
    min_x, max_x = min(x_vals), max(x_vals)
    min_y, max_y = min(y_vals), max(y_vals)

    if math.isclose(min_x, max_x):
        max_x += 1.0
    if math.isclose(min_y, max_y):
        max_y += 1.0

    def sx(x: float) -> float:
        return margin_left + (x - min_x) / (max_x - min_x) * plot_w

    def sy(y: float) -> float:
        return margin_top + (max_y - y) / (max_y - min_y) * plot_h

    series_off = [p for p in points if int(p["dump_runtime_metadata"]) == 0]
    series_on = [p for p in points if int(p["dump_runtime_metadata"]) == 1]

    def polyline(series: list[dict[str, object]], color: str) -> str:
        if not series:
            return ""
        coords = " ".join(f"{sx(p['datetime'].timestamp()):.2f},{sy(float(p['build_ms'])):.2f}" for p in series)
        return f'<polyline fill="none" stroke="{color}" stroke-width="2" points="{coords}" />'

    def points_svg(series: list[dict[str, object]], color: str) -> str:
        chunks: list[str] = []
        for p in series:
            cx = sx(p["datetime"].timestamp())
            cy = sy(float(p["build_ms"]))
            chunks.append(f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="3.5" fill="{color}" />')
        return "\n".join(chunks)

    y_ticks = 6
    y_grid: list[str] = []
    y_labels: list[str] = []
    for i in range(y_ticks + 1):
        ratio = i / y_ticks
        y_val = max_y - (max_y - min_y) * ratio
        y_pos = margin_top + plot_h * ratio
        y_grid.append(f'<line x1="{margin_left}" y1="{y_pos:.2f}" x2="{width - margin_right}" y2="{y_pos:.2f}" stroke="#ececec"/>')
        y_labels.append(f'<text x="{margin_left - 10}" y="{y_pos + 4:.2f}" text-anchor="end" font-size="12" fill="#444">{y_val:.3f}</text>')

    x_labels: list[str] = []
    for idx, p in enumerate(points):
        if len(points) > 12 and idx % max(1, len(points) // 12) != 0 and idx != len(points) - 1:
            continue
        x_pos = sx(p["datetime"].timestamp())
        x_labels.append(
            f'<text x="{x_pos:.2f}" y="{height - margin_bottom + 24}" text-anchor="middle" font-size="10" fill="#444" transform="rotate(25 {x_pos:.2f},{height - margin_bottom + 24})">{p["timestamp"]}</text>'
        )

    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">
<rect x="0" y="0" width="{width}" height="{height}" fill="#ffffff"/>
<text x="{margin_left}" y="28" font-size="20" font-weight="bold" fill="#222">Build Time Trend (ms)</text>
<text x="{margin_left}" y="46" font-size="12" fill="#666">Only build time is used as evaluation metric; grouped by runtime metadata dump flag.</text>
{''.join(y_grid)}
<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{height - margin_bottom}" stroke="#333"/>
<line x1="{margin_left}" y1="{height - margin_bottom}" x2="{width - margin_right}" y2="{height - margin_bottom}" stroke="#333"/>
{''.join(y_labels)}
{''.join(x_labels)}
{polyline(series_off, '#1f77b4')}
{polyline(series_on, '#d62728')}
{points_svg(series_off, '#1f77b4')}
{points_svg(series_on, '#d62728')}
<rect x="{width - 300}" y="22" width="12" height="12" fill="#1f77b4"/><text x="{width - 282}" y="32" font-size="12" fill="#333">dump_runtime_metadata=0</text>
<rect x="{width - 300}" y="42" width="12" height="12" fill="#d62728"/><text x="{width - 282}" y="52" font-size="12" fill="#333">dump_runtime_metadata=1</text>
<text x="{width / 2:.2f}" y="{height - 12}" text-anchor="middle" font-size="12" fill="#555">timestamp</text>
<text x="20" y="{height / 2:.2f}" text-anchor="middle" font-size="12" fill="#555" transform="rotate(-90 20,{height / 2:.2f})">build time (ms)</text>
</svg>
"""
    svg_path.write_text(svg, encoding="utf-8")


def classify_trace_event(name: str, detail: str) -> str:
    text = f"{name} {detail}".lower()
    if "consteval" in text or "constexpr" in text or "evaluate" in text or "constantexpr" in text:
        return "consteval"
    if "instantiate" in text or "template" in text:
        return "template_instantiation"
    if name == "Source" or "parse" in text or any(ext in text for ext in (".h", ".hpp", ".hh", ".hxx")):
        return "header_parse"
    return "other"


def analyze_ftime_trace(build_dir: Path, report_dir: Path, ts: str) -> dict[str, str]:
    trace_files = sorted(build_dir.rglob("*.json"))
    category_us = {
        "header_parse": 0,
        "consteval": 0,
        "template_instantiation": 0,
        "other": 0,
    }
    top_events: list[tuple[int, str, str, str]] = []
    valid_trace_files: list[Path] = []

    for trace_path in trace_files:
        try:
            data = json.loads(trace_path.read_text(encoding="utf-8", errors="replace"))
        except Exception:
            continue
        events = data.get("traceEvents") if isinstance(data, dict) else None
        if not isinstance(events, list):
            continue

        saw_trace = False
        for event in events:
            if not isinstance(event, dict):
                continue
            if event.get("ph") != "X":
                continue
            name = str(event.get("name", ""))
            dur = event.get("dur", 0)
            if not isinstance(dur, (int, float)):
                continue
            args = event.get("args", {})
            detail = ""
            if isinstance(args, dict):
                detail = str(args.get("detail", ""))
            category = classify_trace_event(name, detail)
            category_us[category] += int(dur)
            top_events.append((int(dur), category, name, detail))
            saw_trace = True

        if saw_trace:
            valid_trace_files.append(trace_path)

    top_events.sort(key=lambda item: item[0], reverse=True)
    top_events = top_events[:30]

    summary_txt = report_dir / f"ftime_trace_summary_{ts}.txt"
    summary_csv = report_dir / f"ftime_trace_summary_{ts}.csv"

    with summary_csv.open("w", encoding="utf-8", newline="") as fp:
        writer = csv.writer(fp)
        writer.writerow(["category", "duration_us"])
        for key in ("header_parse", "consteval", "template_instantiation", "other"):
            writer.writerow([key, category_us[key]])

    bottleneck = "unknown"
    header_us = category_us["header_parse"]
    consteval_us = category_us["consteval"]
    if header_us > consteval_us:
        bottleneck = "header_parse"
    elif consteval_us > header_us:
        bottleneck = "consteval"
    else:
        bottleneck = "tie"

    lines = [
        f"trace_json_count={len(valid_trace_files)}",
        f"header_parse_us={header_us}",
        f"consteval_us={consteval_us}",
        f"template_instantiation_us={category_us['template_instantiation']}",
        f"other_us={category_us['other']}",
        f"bottleneck={bottleneck}",
        "",
        "top_events_us:",
    ]
    for dur, category, name, detail in top_events:
        lines.append(f"- {dur} us | {category} | {name} | {detail}")
    if valid_trace_files:
        lines.append("")
        lines.append("trace_files:")
        for path in valid_trace_files:
            lines.append(f"- {path}")

    summary_txt.write_text("\n".join(lines), encoding="utf-8")

    return {
        "ftime_trace_json_count": str(len(valid_trace_files)),
        "ftime_trace_header_parse_us": str(header_us),
        "ftime_trace_consteval_us": str(consteval_us),
        "ftime_trace_template_instantiation_us": str(category_us["template_instantiation"]),
        "ftime_trace_other_us": str(category_us["other"]),
        "ftime_trace_bottleneck": bottleneck,
        "ftime_trace_summary_txt": str(summary_txt),
        "ftime_trace_summary_csv": str(summary_csv),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Cross-platform reflection stress runner.")
    parser.add_argument("--generator", default="MinGW Makefiles", help="CMake generator")
    parser.add_argument("--build-type", default="Release", help="CMake build type")
    parser.add_argument("--build-dir", default="build-mingw", help="Build directory")
    parser.add_argument("--report-dir", default="reports", help="Report output directory")
    parser.add_argument("--binary-name", default="reflect_stress", help="Built binary name")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1, help="Parallel build jobs")
    parser.add_argument("--print-raw-output", action="store_true", help="Print full raw outputs from configure/build/run")
    parser.add_argument("--print-summary", action="store_true", help="Print parsed summary metrics")
    parser.add_argument("--dump-runtime-metadata", action="store_true", help="Enable runtime_meta CSV dump from reflect_stress binary")
    parser.add_argument("--ftime-trace", action="store_true", help="Enable clang -ftime-trace and auto analyze trace json")
    parser.add_argument("--binary-arg", action="append", default=[], help="Forward one argument to reflect_stress (repeatable)")
    parser.add_argument("--cmake-extra", action="append", default=[], help="Extra CMake configure arg (repeatable)")
    args = parser.parse_args()

    project_dir = Path(__file__).resolve().parent
    build_dir = project_dir / args.build_dir
    report_dir = project_dir / args.report_dir
    report_dir.mkdir(parents=True, exist_ok=True)

    ts = timestamp()
    report_txt = report_dir / f"stress_{ts}.txt"
    report_csv = report_dir / f"stress_{ts}.csv"

    print(f"[INFO] Report TXT: {report_txt}")
    print(f"[INFO] Report CSV: {report_csv}")

    txt_lines: list[str] = [
        f"[META] timestamp={ts}",
        f"[META] platform={platform.platform()}",
        f"[META] toolchain_generator={args.generator}",
        f"[META] build_type={args.build_type}",
        f"[META] build_dir={build_dir}",
    ]

    cmake_extra_args = list(args.cmake_extra)
    if args.ftime_trace:
        cmake_extra_args.append("-DCMAKE_CXX_FLAGS=-ftime-trace")

    configure_cmd = [
        "cmake",
        "-S",
        str(project_dir),
        "-B",
        str(build_dir),
        "-G",
        args.generator,
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        *cmake_extra_args,
    ]
    build_cmd = ["cmake", "--build", str(build_dir), "--", "-j", str(args.jobs)]

    configure_exit, configure_out, configure_ns = run_cmd(configure_cmd, project_dir, args.print_raw_output)
    txt_lines.append("[CONFIGURE OUTPUT]")
    txt_lines.append(configure_out.rstrip())
    if configure_exit != 0:
        write_summary_csv(
            report_csv,
            [
                ("status", "configure_failed"),
                ("timestamp", ts),
                ("configure_ns", str(configure_ns)),
                ("run_exit", str(configure_exit)),
            ],
        )
        report_txt.write_text("\n".join(line for line in txt_lines if line), encoding="utf-8")
        return configure_exit

    build_exit, build_out, build_ns = run_cmd(build_cmd, project_dir, args.print_raw_output)
    txt_lines.append("[BUILD OUTPUT]")
    txt_lines.append(build_out.rstrip())
    if build_exit != 0:
        write_summary_csv(
            report_csv,
            [
                ("status", "build_failed"),
                ("timestamp", ts),
                ("configure_ns", str(configure_ns)),
                ("build_ns", str(build_ns)),
                ("run_exit", str(build_exit)),
            ],
        )
        report_txt.write_text("\n".join(line for line in txt_lines if line), encoding="utf-8")
        return build_exit

    binary_path = resolve_binary_path(build_dir, args.binary_name)
    run_cmd_args = [str(binary_path)]
    if args.dump_runtime_metadata:
        run_cmd_args.append("--dump-runtime-metadata")
    run_cmd_args.extend(args.binary_arg)
    run_exit, run_out, run_ns = run_cmd(run_cmd_args, project_dir, args.print_raw_output)
    parsed = parse_runtime_output(run_out)

    txt_lines.append("[RUN OUTPUT]")
    txt_lines.append(run_out.rstrip())

    pipeline_ns = configure_ns + build_ns + run_ns
    trace_info = {
        "ftime_trace_json_count": "0",
        "ftime_trace_header_parse_us": "0",
        "ftime_trace_consteval_us": "0",
        "ftime_trace_template_instantiation_us": "0",
        "ftime_trace_other_us": "0",
        "ftime_trace_bottleneck": "disabled",
        "ftime_trace_summary_txt": "disabled",
        "ftime_trace_summary_csv": "disabled",
    }
    if args.ftime_trace:
        trace_info = analyze_ftime_trace(build_dir, report_dir, ts)

    summary_rows = [
        ("timestamp", ts),
        ("toolchain_generator", args.generator),
        ("build_type", args.build_type),
        ("configure_ns", str(configure_ns)),
        ("build_ns", str(build_ns)),
        ("run_ns", str(run_ns)),
        ("pipeline_ns", str(pipeline_ns)),
        ("run_exit", str(run_exit)),
        ("print_raw_output", "1" if args.print_raw_output else "0"),
        ("dump_runtime_metadata", "1" if args.dump_runtime_metadata else "0"),
        ("ftime_trace", "1" if args.ftime_trace else "0"),
        ("validated_classes", parsed.get("validated_classes", "NA")),
        ("metadata_errors", parsed.get("metadata_errors", "NA")),
        ("runtime_validation_total_ns", parsed.get("runtime_validation_total_ns", "NA")),
        ("runtime_metadata_check_ns", parsed.get("runtime_metadata_check_ns", "NA")),
        ("runtime_value_check_ns", parsed.get("runtime_value_check_ns", "NA")),
        ("runtime_overhead_ns", parsed.get("runtime_overhead_ns", "NA")),
        ("per_class_avg_ns", parsed.get("per_class_avg_ns", "NA")),
        ("runtime_metadata_enabled", parsed.get("runtime_metadata_enabled", "NA")),
        ("runtime_meta_report", parsed.get("runtime_meta_report", "NA")),
        ("ftime_trace_json_count", trace_info["ftime_trace_json_count"]),
        ("ftime_trace_header_parse_us", trace_info["ftime_trace_header_parse_us"]),
        ("ftime_trace_consteval_us", trace_info["ftime_trace_consteval_us"]),
        ("ftime_trace_template_instantiation_us", trace_info["ftime_trace_template_instantiation_us"]),
        ("ftime_trace_other_us", trace_info["ftime_trace_other_us"]),
        ("ftime_trace_bottleneck", trace_info["ftime_trace_bottleneck"]),
        ("ftime_trace_summary_txt", trace_info["ftime_trace_summary_txt"]),
        ("ftime_trace_summary_csv", trace_info["ftime_trace_summary_csv"]),
    ]
    write_summary_csv(report_csv, summary_rows)

    txt_lines.extend(
        [
            f"[META] configure_ns={configure_ns}",
            f"[META] build_ns={build_ns}",
            f"[META] run_ns={run_ns}",
            f"[META] pipeline_ns={pipeline_ns}",
            f"[META] print_raw_output={1 if args.print_raw_output else 0}",
            f"[META] dump_runtime_metadata={1 if args.dump_runtime_metadata else 0}",
            f"[META] ftime_trace={1 if args.ftime_trace else 0}",
            f"[META] ftime_trace_bottleneck={trace_info['ftime_trace_bottleneck']}",
            f"[META] ftime_trace_summary_txt={trace_info['ftime_trace_summary_txt']}",
        ]
    )
    report_txt.write_text("\n".join(line for line in txt_lines if line), encoding="utf-8")

    trend_svg = report_dir / "build_trend.svg"
    trend_csv = report_dir / "build_trend_aggregated.csv"
    build_trend_svg(report_dir, trend_svg, trend_csv)
    if args.print_summary:
        print(f"trend_svg: {trend_svg}")
        print(f"trend_aggregated_csv: {trend_csv}")

    if args.print_summary:
        for key, value in summary_rows:
            print(f"{key}: {value}")

    return run_exit


if __name__ == "__main__":
    raise SystemExit(main())