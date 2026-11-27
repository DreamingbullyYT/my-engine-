#!/usr/bin/env python3
"""Build a .dmod ZIP package. Lua-only mods need Python's standard library only."""
from __future__ import annotations

import argparse
import json
import re
import sys
import zipfile
from pathlib import Path, PurePosixPath

MAX_PACKAGE_BYTES = 32 * 1024 * 1024
MAX_ENTRY_BYTES = 16 * 1024 * 1024
MAX_SCRIPT_BYTES = 4 * 1024 * 1024
ID_RE = re.compile(r"^[a-z0-9_-]+(?:\.[a-z0-9_-]+)*$")
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+$")


class PackageError(ValueError):
    pass


def safe_member_path(value: object, field: str) -> str:
    if not isinstance(value, str) or not value or value.startswith("/") or value.endswith("/"):
        raise PackageError(f"{field} must be a non-empty package-relative path")
    if "\\" in value or ":" in value or "//" in value:
        raise PackageError(f"{field} contains an unsafe path: {value!r}")
    parts = value.split("/")
    if any(part in ("", ".", "..") for part in parts):
        raise PackageError(f"{field} contains an unsafe path: {value!r}")
    return PurePosixPath(value).as_posix()


def load_manifest(root: Path) -> dict:
    manifest_path = root / "manifest.json"
    if not manifest_path.is_file() or manifest_path.is_symlink():
        raise PackageError("source directory must contain a regular manifest.json")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise PackageError(f"cannot read manifest.json: {exc}") from exc
    if not isinstance(manifest, dict):
        raise PackageError("manifest.json must contain a JSON object")
    if type(manifest.get("format_version")) is not int or manifest["format_version"] != 1:
        raise PackageError("format_version must be integer 1")
    mod_id = manifest.get("id")
    if not isinstance(mod_id, str) or not ID_RE.fullmatch(mod_id):
        raise PackageError("id must use lowercase letters, digits, dot, underscore or hyphen")
    if not isinstance(manifest.get("name"), str) or not manifest["name"].strip():
        raise PackageError("name must be a non-empty string")
    if not isinstance(manifest.get("version"), str) or not VERSION_RE.fullmatch(manifest["version"]):
        raise PackageError("version must use MAJOR.MINOR.PATCH")
    runtime = manifest.get("runtime", "lua")
    if runtime not in ("lua", "native", "hybrid"):
        raise PackageError('runtime must be "lua", "native" or "hybrid"')
    manifest["runtime"] = runtime

    if runtime in ("lua", "hybrid"):
        entry = safe_member_path(manifest.get("entry_script", "scripts/main.lua"), "entry_script")
        manifest["entry_script"] = entry
        script = root.joinpath(*PurePosixPath(entry).parts)
        if not script.is_file() or script.is_symlink():
            raise PackageError(f"entry script does not exist as a regular file: {entry}")
        if script.stat().st_size <= 0 or script.stat().st_size > MAX_SCRIPT_BYTES:
            raise PackageError(f"entry script must be between 1 byte and {MAX_SCRIPT_BYTES} bytes")
    elif "entry_script" in manifest:
        raise PackageError("native-only manifests must not declare entry_script")

    if runtime in ("native", "hybrid"):
        library = safe_member_path(manifest.get("native_library"), "native_library")
        manifest["native_library"] = library
        native_path = root.joinpath(*PurePosixPath(library).parts)
        if not native_path.is_file() or native_path.is_symlink():
            raise PackageError(f"native library does not exist as a regular file: {library}")
        if native_path.stat().st_size <= 0 or native_path.stat().st_size > MAX_ENTRY_BYTES:
            raise PackageError(f"native library must be between 1 byte and {MAX_ENTRY_BYTES} bytes")
    elif "native_library" in manifest:
        raise PackageError("Lua-only manifests must not declare native_library")
    return manifest


def collect_files(root: Path, output: Path) -> list[tuple[str, Path]]:
    members: list[tuple[str, Path]] = []
    total = 0
    output_resolved = output.resolve()
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise PackageError(f"symbolic links are not allowed in .dmod packages: {path}")
        if not path.is_file() or path.name == ".DS_Store":
            continue
        if path.resolve() == output_resolved:
            continue
        relative = path.relative_to(root).as_posix()
        safe_member_path(relative, "package member")
        if relative == ".git" or relative.startswith(".git/") or "/.git/" in relative:
            continue
        total += path.stat().st_size
        if total > MAX_PACKAGE_BYTES:
            raise PackageError(f"uncompressed package contents exceed {MAX_PACKAGE_BYTES} bytes")
        members.append((relative, path))
    if not any(name == "manifest.json" for name, _ in members):
        raise PackageError("manifest.json missing from package")
    return members


def build_package(source: Path, output: Path) -> Path:
    root = source.resolve(strict=True)
    if not root.is_dir():
        raise PackageError("source must be a directory")
    manifest = load_manifest(root)
    members = collect_files(root, output)
    output.parent.mkdir(parents=True, exist_ok=True)
    manifest_bytes = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        archive.writestr("manifest.json", manifest_bytes)
        for name, path in members:
            if name == "manifest.json":
                continue
            archive.write(path, name)
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description="Pack a Lua/native/hybrid mod as one .dmod file")
    parser.add_argument("source", type=Path, help="directory containing manifest.json and mod files")
    parser.add_argument("-o", "--output", type=Path, help="output .dmod path (defaults to <id>.dmod)")
    args = parser.parse_args()
    try:
        root = args.source.resolve(strict=True)
        manifest = load_manifest(root)
        output = args.output or Path(f"{manifest['id']}.dmod")
        if output.suffix.lower() != ".dmod":
            raise PackageError("output file extension must be .dmod")
        result = build_package(root, output)
        with zipfile.ZipFile(result) as archive:
            bad = archive.testzip()
            if bad is not None:
                raise PackageError(f"ZIP CRC validation failed for {bad}")
        print(f"Created {result} ({result.stat().st_size} bytes)")
        return 0
    except (OSError, PackageError, zipfile.BadZipFile) as exc:
        print(f"dmod-pack: error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
