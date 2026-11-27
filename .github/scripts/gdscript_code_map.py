#!/usr/bin/env python3
"""Build a syntax-derived navigation map for GDScript source.

This complements CodeGraphContext, whose installed language set does not include
GDScript. It reports direct calls only when the target is declared in the same
script file. Source and tests remain authoritative.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from tree_sitter import Language, Parser
import tree_sitter_gdscript


EXCLUDED_DIRS = {".git", ".godot", ".venv", ".cache", "build", "dist"}
GRAMMAR_VERSION = "tree-sitter-gdscript 6.1.0"


@dataclass
class FunctionInfo:
    name: str
    qualified_name: str
    line: int
    node: object


def node_text(node: object) -> str:
    return node.text.decode("utf-8", errors="replace").strip()


def make_parser() -> Parser:
    language = Language(tree_sitter_gdscript.language())
    try:
        return Parser(language)
    except TypeError:
        parser = Parser()
        parser.set_language(language)
        return parser


def iter_gd_files(root: Path) -> Iterable[Path]:
    for path in sorted(root.rglob("*.gd")):
        relative = path.relative_to(root)
        if not any(part in EXCLUDED_DIRS for part in relative.parts):
            yield path


def collect_symbols(root_node: object) -> tuple[str | None, list[FunctionInfo], list[str]]:
    script_class: str | None = None
    functions: list[FunctionInfo] = []
    classes: list[str] = []

    def visit(node: object, class_scope: tuple[str, ...] = ()) -> None:
        nonlocal script_class

        if node.type == "class_name_statement":
            name_node = node.child_by_field_name("name")
            if name_node is not None:
                script_class = node_text(name_node)

        if node.type == "class_definition":
            name_node = node.child_by_field_name("name")
            nested_name = node_text(name_node) if name_node is not None else "anonymous"
            full_name = ".".join((*class_scope, nested_name))
            classes.append(full_name)
            body = node.child_by_field_name("body")
            if body is not None:
                visit(body, (*class_scope, nested_name))
            return

        if node.type == "function_definition":
            name_node = node.child_by_field_name("name")
            if name_node is not None:
                function_name = node_text(name_node)
                qualified = ".".join((*class_scope, function_name))
                functions.append(
                    FunctionInfo(
                        name=function_name,
                        qualified_name=qualified,
                        line=node.start_point[0] + 1,
                        node=node,
                    )
                )
            return

        for child in node.named_children:
            visit(child, class_scope)

    visit(root_node)
    return script_class, functions, classes


def direct_local_calls(function: FunctionInfo, local_names: set[str]) -> list[str]:
    found: set[str] = set()

    def visit(node: object) -> None:
        if node is not function.node and node.type == "function_definition":
            return

        if node.type == "call" and node.named_children:
            callee = node.named_children[0]
            if callee.type == "identifier":
                called_name = node_text(callee)
                if called_name in local_names:
                    found.add(called_name)

        for child in node.named_children:
            visit(child)

    visit(function.node)
    return sorted(found)


def render_map(root: Path, commit: str, generated_at: str) -> tuple[str, int, int, int, list[str]]:
    parser = make_parser()
    file_rows = []
    parse_errors: list[str] = []

    for path in iter_gd_files(root):
        relative = path.relative_to(root).as_posix()
        tree = parser.parse(path.read_bytes())
        if tree.root_node.has_error:
            parse_errors.append(relative)
        script_class, functions, classes = collect_symbols(tree.root_node)
        file_rows.append((relative, script_class, functions, classes))

    total_functions = sum(len(functions) for _, _, functions, _ in file_rows)
    total_classes = sum(
        len(classes) + (1 if script_class else 0)
        for _, script_class, _, classes in file_rows
    )
    total_calls = 0

    lines = [
        "# GDScript Code Map",
        "",
        f"- Source commit: {commit}",
        f"- Generated (UTC): {generated_at}",
        f"- Parser: {GRAMMAR_VERSION}",
        f"- Files: **{len(file_rows)}**",
        f"- Script/nested classes: **{total_classes}**",
        f"- Functions: **{total_functions}**",
        f"- Files with syntax errors: **{len(parse_errors)}**",
        "",
        "> Syntax-derived navigation only. Source and tests remain authoritative.",
        "> Call edges are included only for direct calls to functions declared in the same file.",
        "",
    ]

    for relative, script_class, functions, classes in file_rows:
        lines.extend([f"## {relative}", ""])
        if script_class:
            lines.append(f"- Global class: {script_class}")
        if classes:
            lines.append("- Nested classes: " + ", ".join(classes))
        if not functions:
            lines.extend(["- Functions: none detected", ""])
            continue

        local_names = {function.name for function in functions}
        lines.append("- Functions:")
        for function in functions:
            calls = direct_local_calls(function, local_names)
            total_calls += len(calls)
            call_text = "; calls " + ", ".join(f"{name}()" for name in calls) if calls else ""
            lines.append(
                f"  - {function.qualified_name}() — line {function.line}{call_text}"
            )
        lines.append("")

    if parse_errors:
        lines.extend(["## Parse errors", ""])
        lines.extend(f"- {path}" for path in parse_errors)
        lines.append("")

    return "\n".join(lines).rstrip() + "\n", len(file_rows), total_functions, total_calls, parse_errors


def main() -> int:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--root", type=Path, default=Path("."))
    cli.add_argument("--output", type=Path, required=True)
    cli.add_argument("--commit", default="unknown")
    cli.add_argument("--generated-at", default="set-by-ci")
    cli.add_argument(
        "--expect-call",
        action="append",
        default=[],
        metavar="CALLER:CALLEE",
    )
    args = cli.parse_args()

    root = args.root.resolve()
    if not root.is_dir():
        print(f"Source root does not exist: {root}", file=sys.stderr)
        return 2

    output, file_count, function_count, call_count, parse_errors = render_map(
        root, args.commit, args.generated_at
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output, encoding="utf-8")

    for expected in args.expect_call:
        if ":" not in expected:
            print(f"Invalid --expect-call value: {expected}", file=sys.stderr)
            return 2
        caller, callee = expected.split(":", 1)
        caller_lines = [
            line for line in output.splitlines()
            if f"{caller}()" in line and "calls " in line
        ]
        if not caller_lines or f"{callee}()" not in caller_lines[0]:
            print(f"Expected local call edge not found: {expected}", file=sys.stderr)
            return 1

    print(
        f"GDScript map: {file_count} files, {function_count} functions, "
        f"{call_count} local call edges, {len(parse_errors)} files with syntax errors."
    )
    if parse_errors:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
