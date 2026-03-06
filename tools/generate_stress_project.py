#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def class_name(index: int) -> str:
    return f"TrashClass{index:04d}"


def field_name(index: int) -> str:
    return f"field_{index:03d}"


def checksum_name() -> str:
    return "checksum"


def class_base(index: int, inheritance_chain: int) -> str | None:
    if inheritance_chain <= 1:
        return None
    chain_pos = (index - 1) % inheritance_chain
    if chain_pos == 0:
        return None
    return class_name(index - 1)


def generate_header(count: int, fields_per_class: int, inheritance_chain: int) -> str:
    lines: list[str] = []
    lines.append("#pragma once")
    lines.append('#include "reflect.hpp"')
    lines.append("#include <cstdint>")
    lines.append("#include <string_view>")
    lines.append("")
    lines.append("namespace SRefl = reflect::Static;")
    lines.append("")
    lines.append("#define REFLECT_CLASS(TYPE, BODY) \\")
    lines.append("    RFS_OBJ_BEGIN(TYPE)           \\")
    lines.append("        BODY                      \\")
    lines.append("    RFS_OBJ_END()")
    lines.append("")

    for i in range(1, count + 1):
        cls = class_name(i)
        base = class_base(i, inheritance_chain)
        checksum = checksum_name()
        class_decl = f"class {cls}" if base is None else f"class {cls} : public {base}"
        lines.append(f"{class_decl} {{")
        lines.append("public:")
        for f_idx in range(fields_per_class):
            fname = field_name(f_idx)
            init = i * 1000 + f_idx
            lines.append(f"    std::int32_t {fname} = {init};")
        sum_expr = " + ".join(field_name(f_idx) for f_idx in range(fields_per_class))
        lines.append(f"    [[nodiscard]] std::int64_t {checksum}() const noexcept {{")
        lines.append(f"        return static_cast<std::int64_t>({sum_expr});")
        lines.append("    }")
        lines.append("};")
        lines.append("")
        lines.append(f"REFLECT_CLASS({cls},")
        for f_idx in range(fields_per_class):
            lines.append(f"    RFS_OBJ_MEM({field_name(f_idx)})")
        lines.append(f"    RFS_OBJ_MEM({checksum})")
        lines.append(")")
        lines.append("")

    return "\n".join(lines) + "\n"


def generate_main_cpp(count: int, fields_per_class: int) -> str:
    lines: list[str] = []
    lines.append('#include "generated_classes.hpp"')
    lines.append("#include <chrono>")
    lines.append("#include <cstdint>")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdio>")
    lines.append("#include <ctime>")
    lines.append("#include <filesystem>")
    lines.append("#include <fstream>")
    lines.append("#include <iomanip>")
    lines.append("#include <iostream>")
    lines.append("#include <sstream>")
    lines.append("#include <string>")
    lines.append("#include <string_view>")
    lines.append("")
    lines.append("std::string make_timestamp() {")
    lines.append("    const std::time_t now = std::time(nullptr);")
    lines.append("    std::tm tm_now{};")
    lines.append("#if defined(_WIN32)")
    lines.append("    localtime_s(&tm_now, &now);")
    lines.append("#else")
    lines.append("    localtime_r(&now, &tm_now);")
    lines.append("#endif")
    lines.append("    std::ostringstream oss;")
    lines.append("    oss << std::put_time(&tm_now, \"%Y%m%d_%H%M%S\");")
    lines.append("    return oss.str();")
    lines.append("}")
    lines.append("")
    lines.append("struct TimingSummary {")
    lines.append("    std::int64_t total_ns = 0;")
    lines.append("    std::int64_t metadata_check_ns = 0;")
    lines.append("    std::int64_t value_check_ns = 0;")
    lines.append("    std::int64_t rows = 0;")
    lines.append("};")
    lines.append("")
    lines.append("template<typename Obj, typename MemberType>")
    lines.append("std::size_t offset_of_reflected(const Obj& obj, MemberType Obj::* ptr) {")
    lines.append("    const auto* base = reinterpret_cast<const unsigned char*>(&obj);")
    lines.append("    const auto* mem = reinterpret_cast<const unsigned char*>(&(obj.*ptr));")
    lines.append("    return static_cast<std::size_t>(mem - base);")
    lines.append("}")
    lines.append("")
    lines.append("template<typename MemberTraits, typename Obj>")
    lines.append("int validate_field(const MemberTraits& member, Obj& obj, std::int32_t expected,")
    lines.append("                   std::size_t expected_offset, std::string_view expected_name,")
    lines.append("                   std::string_view class_name, std::ofstream* meta_out,")
    lines.append("                   TimingSummary& summary) {")
    lines.append("    int errors = 0;")
    lines.append("    const auto begin = std::chrono::steady_clock::now();")
    lines.append("    const auto meta_begin = begin;")
    lines.append("    const std::string_view member_name = member.getName();")
    lines.append("    if (member_name != expected_name) ++errors;")
    lines.append("    const auto meta_end = std::chrono::steady_clock::now();")
    lines.append("    const auto value_begin = meta_end;")
    lines.append("    const std::int32_t actual = obj.*member._ptr;")
    lines.append("    const std::size_t actual_offset = offset_of_reflected(obj, member._ptr);")
    lines.append("    if (actual != expected) ++errors;")
    lines.append("    if (actual_offset != expected_offset) ++errors;")
    lines.append("    const auto value_end = std::chrono::steady_clock::now();")
    lines.append("    const std::int64_t meta_ns =")
    lines.append("        std::chrono::duration_cast<std::chrono::nanoseconds>(meta_end - meta_begin).count();")
    lines.append("    const std::int64_t value_ns =")
    lines.append("        std::chrono::duration_cast<std::chrono::nanoseconds>(value_end - value_begin).count();")
    lines.append("    const std::int64_t total_ns =")
    lines.append("        std::chrono::duration_cast<std::chrono::nanoseconds>(value_end - begin).count();")
    lines.append("    summary.metadata_check_ns += meta_ns;")
    lines.append("    summary.value_check_ns += value_ns;")
    lines.append("    summary.total_ns += total_ns;")
    lines.append("    ++summary.rows;")
    lines.append("    if (meta_out != nullptr && meta_out->is_open()) {")
    lines.append("        (*meta_out) << class_name << ','")
    lines.append("                    << \"field\" << ','")
    lines.append("                    << member_name << ','")
    lines.append("                    << expected << ','")
    lines.append("                    << actual << ','")
    lines.append("                    << expected_offset << ','")
    lines.append("                    << actual_offset << ','")
    lines.append("                    << meta_ns << ','")
    lines.append("                    << value_ns << ','")
    lines.append("                    << total_ns << ','")
    lines.append("                    << (errors == 0 ? 1 : 0) << '\\n';")
    lines.append("    }")
    lines.append("    return errors;")
    lines.append("}")
    lines.append("")
    lines.append("template<typename FuncTraits, typename Obj>")
    lines.append("int validate_checksum(const FuncTraits& func, Obj& obj, std::int64_t expected,")
    lines.append("                      std::string_view expected_name, std::string_view class_name,")
    lines.append("                      std::ofstream* meta_out, TimingSummary& summary) {")
    lines.append("    int errors = 0;")
    lines.append("    const auto begin = std::chrono::steady_clock::now();")
    lines.append("    const auto meta_begin = begin;")
    lines.append("    const std::string_view name = func.getName();")
    lines.append("    if (name != expected_name) ++errors;")
    lines.append("    const auto meta_end = std::chrono::steady_clock::now();")
    lines.append("    const auto value_begin = meta_end;")
    lines.append("    const std::int64_t actual = (obj.*func._ptr)();")
    lines.append("    if (actual != expected) ++errors;")
    lines.append("    const auto value_end = std::chrono::steady_clock::now();")
    lines.append("    const std::int64_t meta_ns =")
    lines.append("        std::chrono::duration_cast<std::chrono::nanoseconds>(meta_end - meta_begin).count();")
    lines.append("    const std::int64_t value_ns =")
    lines.append("        std::chrono::duration_cast<std::chrono::nanoseconds>(value_end - value_begin).count();")
    lines.append("    const std::int64_t total_ns =")
    lines.append("        std::chrono::duration_cast<std::chrono::nanoseconds>(value_end - begin).count();")
    lines.append("    summary.metadata_check_ns += meta_ns;")
    lines.append("    summary.value_check_ns += value_ns;")
    lines.append("    summary.total_ns += total_ns;")
    lines.append("    ++summary.rows;")
    lines.append("    if (meta_out != nullptr && meta_out->is_open()) {")
    lines.append("        (*meta_out) << class_name << ','")
    lines.append("                    << \"function\" << ','")
    lines.append("                    << name << ','")
    lines.append("                    << expected << ','")
    lines.append("                    << actual << ','")
    lines.append("                    << \"NA\" << ','")
    lines.append("                    << \"NA\" << ','")
    lines.append("                    << meta_ns << ','")
    lines.append("                    << value_ns << ','")
    lines.append("                    << total_ns << ','")
    lines.append("                    << (errors == 0 ? 1 : 0) << '\\n';")
    lines.append("    }")
    lines.append("    return errors;")
    lines.append("}")
    lines.append("")

    for i in range(1, count + 1):
        cls = class_name(i)
        checksum = checksum_name()
        checksum_expected = sum(i * 1000 + f_idx for f_idx in range(fields_per_class))
        lines.append(f"int validate_{i:04d}(std::ofstream* meta_out, TimingSummary& summary) {{")
        lines.append(f"    using Info = SRefl::TypeInfo<{cls}>;")
        lines.append(f"    {cls} obj;")
        lines.append("    int errors = 0;")
        for f_idx in range(fields_per_class):
            fname = field_name(f_idx)
            expected_val = i * 1000 + f_idx
            lines.append(f"    errors += validate_field(Info::Registry::_{fname}, obj, {expected_val},")
            lines.append(f"                             offset_of_reflected(obj, &{cls}::{fname}), \"{fname}\",")
            lines.append(f"                             \"{cls}\", meta_out, summary);")
        lines.append(f"    errors += validate_checksum(Info::Registry::_{checksum}, obj, {checksum_expected}LL,")
        lines.append(f"                              \"{checksum}\", \"{cls}\", meta_out, summary);")
        lines.append("    return errors;")
        lines.append("}")
        lines.append("")

    lines.append("int main(int argc, char** argv) {")
    lines.append("    bool dump_runtime_metadata = false;")
    lines.append("    for (int i = 1; i < argc; ++i) {")
    lines.append("        const std::string arg = argv[i];")
    lines.append("        if (arg == \"--dump-runtime-metadata\" || arg == \"--dump-meta\") {")
    lines.append("            dump_runtime_metadata = true;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    std::ofstream meta_out;")
    lines.append("    std::string meta_path = \"disabled\";")
    lines.append("    if (dump_runtime_metadata) {")
    lines.append("        std::filesystem::create_directories(\"reports\");")
    lines.append("        meta_path = \"reports/runtime_meta_\" + make_timestamp() + \".csv\";")
    lines.append("        meta_out.open(meta_path, std::ios::out | std::ios::trunc);")
    lines.append("        if (!meta_out.is_open()) {")
    lines.append("            std::printf(\"Failed to open metadata report: %s\\n\", meta_path.c_str());")
    lines.append("            return 2;")
    lines.append("        }")
    lines.append("        meta_out << \"class_name,kind,name,expected,actual,expected_offset,actual_offset,meta_check_ns,value_check_ns,total_case_ns,ok\\n\";")
    lines.append("    }")
    lines.append("    TimingSummary summary{};")
    lines.append("    int total_errors = 0;")
    for i in range(1, count + 1):
        lines.append(f"    total_errors += validate_{i:04d}(dump_runtime_metadata ? &meta_out : nullptr, summary);")
    lines.append("    const std::int64_t total_ns = summary.total_ns;")
    lines.append("    const std::int64_t metadata_ns = summary.metadata_check_ns;")
    lines.append("    const std::int64_t value_ns = summary.value_check_ns;")
    lines.append("    const std::int64_t overhead_ns = total_ns - metadata_ns - value_ns;")
    lines.append("    const long long per_class_avg_ns =")
    lines.append("        summary.rows == 0 ? 0 : static_cast<long long>(total_ns / summary.rows);")
    lines.append("    if (meta_out.is_open()) meta_out.close();")
    lines.append(f"    std::printf(\"Validated classes: {count}\\n\");")
    lines.append("    std::printf(\"Metadata errors: %d\\n\", total_errors);")
    lines.append("    std::printf(\"Runtime validation total ns: %lld\\n\", static_cast<long long>(total_ns));")
    lines.append("    std::printf(\"Runtime metadata check ns: %lld\\n\", static_cast<long long>(metadata_ns));")
    lines.append("    std::printf(\"Runtime value check ns: %lld\\n\", static_cast<long long>(value_ns));")
    lines.append("    std::printf(\"Runtime overhead ns: %lld\\n\", static_cast<long long>(overhead_ns));")
    lines.append("    std::printf(\"Per class avg ns: %lld\\n\", per_class_avg_ns);")
    lines.append("    std::printf(\"Runtime metadata enabled: %d\\n\", dump_runtime_metadata ? 1 : 0);")
    lines.append("    std::printf(\"Runtime metadata report: %s\\n\", meta_path.c_str());")
    lines.append("    if (total_errors == 0) {")
    lines.append("        std::printf(\"Verification passed\\n\");")
    lines.append("    } else {")
    lines.append("        std::printf(\"Verification failed\\n\");")
    lines.append("    }")
    lines.append("    return total_errors == 0 ? 0 : 1;")
    lines.append("}")

    return "\n".join(lines) + "\n"


def generate_cmake() -> str:
    return """cmake_minimum_required(VERSION 3.20)
project(ReflectionGarbageStress LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(reflect_stress main.cpp)
target_include_directories(reflect_stress PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if (MINGW)
    target_compile_options(reflect_stress PRIVATE -O2 -pipe)
endif()
"""


def generate_runner_py() -> str:
    return r'''#!/usr/bin/env python3
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
'''


def generate_mingw_bat() -> str:
    return r"""@echo off
setlocal
python "%~dp0run_stress.py" --generator "MinGW Makefiles" %*
exit /b %ERRORLEVEL%
"""


def generate_runner_ps1() -> str:
    return r"""$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
python "$ScriptDir/run_stress.py" @args
exit $LASTEXITCODE
"""


def generate_runner_sh() -> str:
    return """#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
python3 "$SCRIPT_DIR/run_stress.py" "$@"
"""


def generate_readme(count: int, fields_per_class: int, inheritance_chain: int) -> str:
    return f"""# Reflection Garbage Stress Project

此目录由脚本自动生成，用于压力测试：

- 编译阶段压力：默认生成 `{count}` 个类与反射注册代码。
- 每类字段：`{fields_per_class}`
- 继承链深度：`{inheritance_chain}`（每 {inheritance_chain} 个类形成一条链）
- 运行阶段校验：程序会逐类检查字段/函数元信息名称、调用结果、类型名是否正确。
- 反射模式：**静态反射（SRefl）优先**，不依赖动态反射功能。

## 生成命令

在仓库根目录执行：

```bash
python tools/generate_stress_project.py --count {count}
```

可选参数：

- `--fields-per-class {fields_per_class}`
- `--inheritance-chain {inheritance_chain}`

默认输出目录是 `stress_project_test`。

## Python 运行器（跨平台）

推荐直接使用 Python 运行器（高精度纳秒计时）：

```bash
python stress_project_test/run_stress.py --generator "MinGW Makefiles"
```

常用参数：

- `--print-raw-output`：打印 configure/build/run 原始输出（源数据）
- `--print-summary`：在终端打印解析后的关键指标
- `--dump-runtime-metadata`：开启运行时逐类元数据写入（默认关闭）
- `--ftime-trace`：开启 `clang -ftime-trace` 并自动分析瓶颈
- `--binary-arg xxx`：透传参数给 `reflect_stress`（可重复）
- `--cmake-extra xxx`：附加 CMake 配置参数（可重复）

示例：

```bash
python stress_project_test/run_stress.py --generator "MinGW Makefiles" --ftime-trace --print-summary
```

## 快捷包装脚本

- Windows CMD: `run_mingw.bat`
- PowerShell: `run_stress.ps1`
- Bash: `run_stress.sh`

例如：

```powershell
./run_stress.ps1 --generator "MinGW Makefiles" --print-raw-output --print-summary
```

## MinGW 手动命令（可选）

```bash
cmake -S stress_project_test -B stress_project_test/build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build stress_project_test/build-mingw -- -j
stress_project_test/build-mingw/reflect_stress.exe
```

或在 `stress_project_test` 目录直接执行：

```bat
run_mingw.bat
```

## 规模压力基准（1000~10000）

在仓库根目录执行：

```bash
python tools/run_scaling_benchmark.py --counts 1000,2000,4000,8000,10000 --fields-per-class 100 --inheritance-chain 10 --ftime-trace
```

也可使用 Shell/PowerShell 包装脚本：

- `tools/run_scaling_benchmark.sh`
- `tools/run_scaling_benchmark.ps1`

输出：

- `reports/scaling_summary_*.csv`
- `reports/scaling_summary_*.txt`

`scaling_summary_*.txt` 会给出线性/超线性/疑似指数增长判定。

## 元数据文件（可视化观测）

每次执行运行器会在 `reports/` 目录输出带时间戳的文件：

- `stress_yyyyMMdd_HHmmss.csv`：结构化指标，便于导入 Excel/可视化工具
- `stress_yyyyMMdd_HHmmss.txt`：完整文本日志（包含程序输出）
- `build_trend.svg`：自动汇总 `stress_*.csv` 的编译耗时趋势图（横轴时间，纵轴 build_ms）
- `build_trend_aggregated.csv`：趋势图聚合后的明细数据
- `runtime_meta_yyyyMMdd_HHmmss.csv`：仅在 `--dump-runtime-metadata` 开启时生成
- `ftime_trace_summary_yyyyMMdd_HHmmss.txt`：`-ftime-trace` 分类汇总（含瓶颈判定）
- `ftime_trace_summary_yyyyMMdd_HHmmss.csv`：`header_parse/consteval/template` 耗时分类

`runtime_meta_*.csv` 中包含：`class_name`、`kind`、`name`、`expected`、`actual`、`expected_offset`、`actual_offset`、`ok`。

`runtime_meta_*.csv` 中还包含每类细粒度耗时：`meta_check_ns`、`value_check_ns`、`total_case_ns`。

`stress_*.csv` 中包含：`configure_ns`、`build_ns`、`run_ns`、`pipeline_ns`、`print_raw_output`、`dump_runtime_metadata`、`validated_classes`、`metadata_errors`、`runtime_validation_total_ns`、`runtime_metadata_check_ns`、`runtime_value_check_ns`、`runtime_overhead_ns`、`per_class_avg_ns`、`run_exit`、`runtime_meta_report` 等关键指标。

趋势图评判标准仅使用编译时间（`build_ns` / `build_ms`），并按 `dump_runtime_metadata` 分组显示，避免运行期磁盘 I/O 干扰编译性能判断。

`-ftime-trace` 瓶颈判断规则：

- `ftime_trace_header_parse_us > ftime_trace_consteval_us`：瓶颈偏向头文件解析
- `ftime_trace_consteval_us > ftime_trace_header_parse_us`：瓶颈偏向 consteval/constexpr 计算

运行返回码：

- `0`：元信息校验全部通过
- 非 `0`：存在校验失败
"""


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a large C++ reflection stress project."
    )
    parser.add_argument(
        "--count",
        type=int,
        default=5000,
        help="Number of classes to generate (default: 5000)",
    )
    parser.add_argument(
        "--fields-per-class",
        type=int,
        default=100,
        help="Number of fields in each class (default: 100)",
    )
    parser.add_argument(
        "--inheritance-chain",
        type=int,
        default=10,
        help="Inheritance chain depth (default: 10)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("stress_project_test"),
        help="Output directory for generated project (default: stress_project_test)",
    )

    args = parser.parse_args()
    if args.count <= 0:
        raise ValueError("--count must be greater than 0")
    if args.fields_per_class <= 0:
        raise ValueError("--fields-per-class must be greater than 0")
    if args.inheritance_chain <= 0:
        raise ValueError("--inheritance-chain must be greater than 0")

    output_dir = args.output
    output_dir.mkdir(parents=True, exist_ok=True)

    (output_dir / "generated_classes.hpp").write_text(
        generate_header(args.count, args.fields_per_class, args.inheritance_chain), encoding="utf-8"
    )
    (output_dir / "main.cpp").write_text(
        generate_main_cpp(args.count, args.fields_per_class), encoding="utf-8"
    )
    (output_dir / "CMakeLists.txt").write_text(generate_cmake(), encoding="utf-8")
    (output_dir / "run_stress.py").write_text(generate_runner_py(), encoding="utf-8")
    (output_dir / "run_mingw.bat").write_text(generate_mingw_bat(), encoding="utf-8")
    (output_dir / "run_stress.ps1").write_text(generate_runner_ps1(), encoding="utf-8")
    (output_dir / "run_stress.sh").write_text(generate_runner_sh(), encoding="utf-8")
    (output_dir / "README.md").write_text(
        generate_readme(args.count, args.fields_per_class, args.inheritance_chain), encoding="utf-8"
    )

    print(f"[OK] Generated stress project at: {output_dir.resolve()}")
    print(f"[OK] Class count: {args.count}")


if __name__ == "__main__":
    main()
