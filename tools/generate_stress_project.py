#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import shutil


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
    lines.append("    SKL_RFS_OBJ_BEGIN(TYPE)           \\")
    lines.append("        BODY                      \\")
    lines.append("    SKL_RFS_OBJ_END()")
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


def generate_readme(count: int, fields_per_class: int, inheritance_chain: int, zh_cn: bool = False) -> str:
    if zh_cn:
        return f"""# Reflection Garbage Stress Project（反射垃圾压力测试）

此目录由脚本自动生成，用于压力测试静态反射系统的编译与运行性能。

## 项目参数
- 类数量：{count}
- 每类字段数：{fields_per_class}
- 继承链深度：{inheritance_chain}

## 生成命令
```bash
python tools/generate_stress_project.py --count {count} --fields-per-class {fields_per_class} --inheritance-chain {inheritance_chain}
```

## 运行
```bash
python stress_project_test/run_stress.py --generator "MinGW Makefiles"
```

常用参数：
- `--print-raw-output`：打印 configure/build/run 原始输出
- `--print-summary`：打印解析后的关键指标
- `--dump-runtime-metadata`：开启运行时逐类元数据写入
- `--ftime-trace`：开启 clang -ftime-trace 并自动分析瓶颈
- `--binary-arg xxx`：透传参数给 reflect_stress（可重复）
- `--cmake-extra xxx`：附加 CMake 配置参数（可重复）

## 快捷脚本
- Windows CMD: `run_mingw.bat`
- PowerShell: `run_stress.ps1`
- Bash: `run_stress.sh`

## 报告
每次运行会在 `reports/` 目录输出带时间戳的文件：
- `stress_yyyyMMdd_HHmmss.csv`：结构化指标
- `stress_yyyyMMdd_HHmmss.txt`：完整文本日志
- `build_trend.svg`：编译耗时趋势图
- `runtime_meta_yyyyMMdd_HHmmss.csv`：运行时元数据（需 --dump-runtime-metadata）
- `ftime_trace_summary_yyyyMMdd_HHmmss.*`：编译时间追踪分析（需 --ftime-trace）

运行返回码：0 表示全部通过，非 0 表示存在校验失败。
"""
    else:
        return f"""# Reflection Garbage Stress Project

This directory is auto-generated for stress testing the static reflection system's compile-time and runtime performance.

## Project Parameters
- Classes: {count}
- Fields per class: {fields_per_class}
- Inheritance chain depth: {inheritance_chain}

## Generation Command
```bash
python tools/generate_stress_project.py --count {count} --fields-per-class {fields_per_class} --inheritance-chain {inheritance_chain}
```

## Running
```bash
python stress_project_test/run_stress.py --generator "MinGW Makefiles"
```

Common options:
- `--print-raw-output`: Print raw configure/build/run output
- `--print-summary`: Print parsed key metrics
- `--dump-runtime-metadata`: Enable per-class runtime metadata dump
- `--ftime-trace`: Enable clang -ftime-trace and auto-analyze bottlenecks
- `--binary-arg xxx`: Forward argument to reflect_stress (repeatable)
- `--cmake-extra xxx`: Extra CMake configure argument (repeatable)

## Wrapper Scripts
- Windows CMD: `run_mingw.bat`
- PowerShell: `run_stress.ps1`
- Bash: `run_stress.sh`

## Reports
Each run produces timestamped files in the `reports/` directory:
- `stress_yyyyMMdd_HHmmss.csv`: Structured metrics
- `stress_yyyyMMdd_HHmmss.txt`: Full text log
- `build_trend.svg`: Build time trend chart
- `runtime_meta_yyyyMMdd_HHmmss.csv`: Runtime metadata (requires --dump-runtime-metadata)
- `ftime_trace_summary_yyyyMMdd_HHmmss.*`: Compile-time trace analysis (requires --ftime-trace)

Exit code: 0 means all passed, non-zero means verification failures.
"""


def main() -> None:
    import sys

    lang_zh = "--zh_cn" in sys.argv

    if lang_zh:
        desc = "生成大型 C++ 反射压力测试项目。"
        count_help = "生成的类数量（默认：5000）"
        fields_help = "每个类的字段数量（默认：100）"
        chain_help = "继承链深度（默认：10）"
        output_help = "输出目录（默认：stress_project_test）"
        zh_help = "以中文显示帮助信息"
    else:
        desc = "Generate a large C++ reflection stress test project."
        count_help = "Number of classes to generate (default: 5000)"
        fields_help = "Number of fields per class (default: 100)"
        chain_help = "Inheritance chain depth (default: 10)"
        output_help = "Output directory (default: stress_project_test)"
        zh_help = "Show help in Chinese"

    parser = argparse.ArgumentParser(description=desc)
    parser.add_argument("--count", type=int, default=5000, help=count_help)
    parser.add_argument("--fields-per-class", type=int, default=100, help=fields_help)
    parser.add_argument("--inheritance-chain", type=int, default=10, help=chain_help)
    parser.add_argument("--output", type=Path, default=Path("stress_project_test"), help=output_help)
    parser.add_argument("--zh_cn", action="store_true", help=zh_help)

    args = parser.parse_args()
    if args.count <= 0:
        raise ValueError("--count must be greater than 0" if not lang_zh else "--count 必须大于 0")
    if args.fields_per_class <= 0:
        raise ValueError("--fields-per-class must be greater than 0" if not lang_zh else "--fields-per-class 必须大于 0")
    if args.inheritance_chain <= 0:
        raise ValueError("--inheritance-chain must be greater than 0" if not lang_zh else "--inheritance-chain 必须大于 0")

    output_dir = args.output
    output_dir.mkdir(parents=True, exist_ok=True)

    (output_dir / "generated_classes.hpp").write_text(
        generate_header(args.count, args.fields_per_class, args.inheritance_chain), encoding="utf-8"
    )
    (output_dir / "main.cpp").write_text(
        generate_main_cpp(args.count, args.fields_per_class), encoding="utf-8"
    )
    (output_dir / "CMakeLists.txt").write_text(generate_cmake(), encoding="utf-8")
    tools_dir = Path(__file__).resolve().parent
    shutil.copy(tools_dir / "run_stress.py", output_dir / "run_stress.py")
    shutil.copy(tools_dir / "run_stress.bat", output_dir / "run_stress.bat")
    shutil.copy(tools_dir / "run_stress.ps1", output_dir / "run_stress.ps1")
    shutil.copy(tools_dir / "run_stress.sh", output_dir / "run_stress.sh")
    (output_dir / "README.md").write_text(
        generate_readme(args.count, args.fields_per_class, args.inheritance_chain, args.zh_cn), encoding="utf-8"
    )

    if lang_zh:
        print(f"[OK] 已生成压力测试项目: {output_dir.resolve()}")
        print(f"[OK] 类数量: {args.count}")
    else:
        print(f"[OK] Generated stress project at: {output_dir.resolve()}")
        print(f"[OK] Class count: {args.count}")


if __name__ == "__main__":
    main()