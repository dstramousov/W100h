#!/usr/bin/env python3
"""Install W100h desktop, PT3 MIME, and optional Midnight Commander integration."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


APP_ID = "org.w100h.player"
MIME_TYPE = "audio/x-pt3"
MC_BEGIN = "# BEGIN W100h managed section"
MC_END = "# END W100h managed section"


def _desktop_value(value: str) -> str:
    return (
        value.replace("\\", "\\\\")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )


def _exec_path(path: Path) -> str:
    value = str(path).replace("%", "%%")
    escaped = (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("`", "\\`")
        .replace("$", "\\$")
    )
    return f'"{escaped}"'


def _shell_quote_double(path: Path) -> str:
    value = str(path)
    escaped = (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("`", "\\`")
        .replace("$", "\\$")
    )
    return f'"{escaped}"'


def _write_atomic(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary_path = Path(temporary_name)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
        temporary_path.replace(path)
    except BaseException:
        temporary_path.unlink(missing_ok=True)
        raise


def _copy_atomic(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(prefix=f".{destination.name}.", dir=destination.parent)
    os.close(fd)
    temporary_path = Path(temporary_name)
    try:
        shutil.copy2(source, temporary_path)
        temporary_path.replace(destination)
    except BaseException:
        temporary_path.unlink(missing_ok=True)
        raise


def _xdg_data_home() -> Path:
    configured = os.environ.get("XDG_DATA_HOME")
    if configured:
        return Path(configured).expanduser().resolve()
    return (Path.home() / ".local" / "share").resolve()


def _repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def _run_optional(command: list[str]) -> bool:
    executable = shutil.which(command[0])
    if executable is None:
        return False
    subprocess.run(
        [executable, *command[1:]],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return True


def _remove_managed_mc_block(content: str) -> str:
    pattern = re.compile(
        rf"\n?{re.escape(MC_BEGIN)}\n.*?\n{re.escape(MC_END)}\n?",
        flags=re.DOTALL,
    )
    cleaned = pattern.sub("\n", content)
    return cleaned.rstrip() + "\n"


def _find_mc_system_extension_file() -> Path | None:
    candidates = [
        Path("/etc/mc/mc.ext.ini"),
        Path("/usr/share/mc/mc.ext.ini"),
        Path("/usr/local/etc/mc/mc.ext.ini"),
        Path("/usr/local/share/mc/mc.ext.ini"),
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def _install_mc_rule(binary: Path) -> str:
    if shutil.which("mc") is None:
        return "Midnight Commander is not installed; MC rule skipped."

    user_path = Path.home() / ".config" / "mc" / "mc.ext.ini"
    if user_path.is_file():
        content = user_path.read_text(encoding="utf-8")
    else:
        system_path = _find_mc_system_extension_file()
        if system_path is None:
            return "Midnight Commander extension template was not found; MC rule skipped."
        content = system_path.read_text(encoding="utf-8")

    content = _remove_managed_mc_block(content)
    metadata_match = re.search(r"(?m)^\[mc\.ext\.ini\]\s*$", content)
    if metadata_match is None:
        return "Midnight Commander extension file has an unknown format; MC rule skipped."

    next_section = re.search(r"(?m)^\[[^\n]+\]\s*$", content[metadata_match.end():])
    insert_at = (
        metadata_match.end() + next_section.start()
        if next_section is not None
        else len(content)
    )
    rule = (
        f"\n{MC_BEGIN}\n"
        "[W100h PT3]\n"
        "Shell=.pt3\n"
        "ShellIgnoreCase=true\n"
        f"Open={_shell_quote_double(binary)} %f >/dev/null 2>&1 &\n"
        f"{MC_END}\n\n"
    )
    updated = content[:insert_at].rstrip() + "\n" + rule + content[insert_at:].lstrip("\n")
    _write_atomic(user_path, updated)
    user_path.chmod(0o644)
    return f"Installed Midnight Commander PT3 rule: {user_path}"


def _uninstall_mc_rule() -> str:
    user_path = Path.home() / ".config" / "mc" / "mc.ext.ini"
    if not user_path.is_file():
        return "Midnight Commander PT3 rule was not installed."
    content = user_path.read_text(encoding="utf-8")
    updated = _remove_managed_mc_block(content)
    if updated == content:
        return "Midnight Commander PT3 rule was not installed."
    _write_atomic(user_path, updated)
    user_path.chmod(0o644)
    return f"Removed W100h rule from Midnight Commander: {user_path}"


def install(*, install_mc: bool) -> int:
    if not sys.platform.startswith("linux"):
        print("ERROR: Linux desktop integration is supported only on Linux.", file=sys.stderr)
        return 2

    repo_root = _repo_root()
    binary = (repo_root / "build" / "bin" / "w100h").resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        print("ERROR: build/bin/w100h is missing. Run ./m first.", file=sys.stderr)
        return 2

    data_home = _xdg_data_home()
    applications_dir = data_home / "applications"
    mime_root = data_home / "mime"
    mime_packages_dir = mime_root / "packages"
    desktop_path = applications_dir / f"{APP_ID}.desktop"
    mime_path = mime_packages_dir / f"{APP_ID}.xml"
    icon_path = data_home / "icons" / "hicolor" / "scalable" / "apps" / f"{APP_ID}.svg"

    desktop_template = repo_root / "packaging" / "linux" / f"{APP_ID}.desktop.in"
    mime_source = repo_root / "packaging" / "linux" / f"{APP_ID}.xml"
    icon_source = repo_root / "assets" / "icons" / f"{APP_ID}.svg"
    for source in (desktop_template, mime_source, icon_source):
        if not source.is_file():
            print(f"ERROR: missing desktop integration asset: {source}", file=sys.stderr)
            return 2

    template = desktop_template.read_text(encoding="utf-8")
    for token in ("@W100H_EXEC@", "@W100H_WORKDIR@"):
        if token not in template:
            print(f"ERROR: desktop-entry template is missing {token}.", file=sys.stderr)
            return 2

    desktop_content = template.replace("@W100H_EXEC@", _exec_path(binary)).replace(
        "@W100H_WORKDIR@", _desktop_value(str(repo_root))
    )
    _write_atomic(desktop_path, desktop_content)
    desktop_path.chmod(0o644)
    _copy_atomic(mime_source, mime_path)
    mime_path.chmod(0o644)
    _copy_atomic(icon_source, icon_path)
    icon_path.chmod(0o644)

    _run_optional(["update-mime-database", str(mime_root)])
    _run_optional(["update-desktop-database", str(applications_dir)])

    xdg_mime = shutil.which("xdg-mime")
    if xdg_mime is not None:
        subprocess.run(
            [xdg_mime, "default", f"{APP_ID}.desktop", MIME_TYPE],
            check=False,
        )

    print(f"Installed W100h desktop entry: {desktop_path}")
    print(f"Registered PT3 MIME type: {MIME_TYPE}")
    print(f"Installed W100h icon: {icon_path}")
    if install_mc:
        print(_install_mc_rule(binary))
    print("Nautilus: reopen the Files window if the PT3 association is not visible immediately.")
    print("Midnight Commander: restart mc after installing the rule.")
    return 0


def uninstall(*, uninstall_mc: bool) -> int:
    data_home = _xdg_data_home()
    applications_dir = data_home / "applications"
    mime_root = data_home / "mime"

    (applications_dir / f"{APP_ID}.desktop").unlink(missing_ok=True)
    (mime_root / "packages" / f"{APP_ID}.xml").unlink(missing_ok=True)
    (
        data_home / "icons" / "hicolor" / "scalable" / "apps" / f"{APP_ID}.svg"
    ).unlink(missing_ok=True)

    _run_optional(["update-mime-database", str(mime_root)])
    _run_optional(["update-desktop-database", str(applications_dir)])
    print("Removed W100h desktop/MIME/icon integration for the current user.")
    if uninstall_mc:
        print(_uninstall_mc_rule())
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Install W100h PT3 integration for Nautilus and Midnight Commander."
    )
    parser.add_argument(
        "--uninstall",
        action="store_true",
        help="remove desktop, MIME, icon, and managed Midnight Commander integration",
    )
    parser.add_argument(
        "--no-mc",
        action="store_true",
        help="do not add or remove the managed Midnight Commander PT3 rule",
    )
    args = parser.parse_args()
    if args.uninstall:
        return uninstall(uninstall_mc=not args.no_mc)
    return install(install_mc=not args.no_mc)


if __name__ == "__main__":
    raise SystemExit(main())
