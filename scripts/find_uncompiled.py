#!/usr/bin/env python3
"""
Find files in the repo (git tracked) that are not listed in compile_commands.json.

Usage:
  python3 find_uncompiled.py [--compile compile_commands.json] [--repo-root .]
"""
import json
import os
import subprocess
import argparse

SOURCE_EXTS = {'.c', '.cpp', '.cc', '.cxx', '.c++', '.m', '.mm', '.S', '.s'}
HEADER_EXTS = {'.h', '.hpp', '.hh', '.inl', '.ipp'}

def git_toplevel():
    return subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).decode().strip()

def git_ls_files(repo_root):
    out = subprocess.check_output(['git', 'ls-files'], cwd=repo_root).decode().splitlines()
    return [p.strip() for p in out if p.strip()]

def load_compiled(compile_commands_path, repo_root):
    with open(compile_commands_path, 'r', encoding='utf-8') as f:
        entries = json.load(f)
    compiled = set()
    for e in entries:
        directory = e.get('directory', repo_root)
        file = e.get('file')
        if not file:
            continue
        # resolve to absolute path
        if os.path.isabs(file):
            abs_path = os.path.normpath(file)
        else:
            abs_path = os.path.normpath(os.path.join(directory, file))
        # attempt to make repo-relative if possible
        try:
            rel = os.path.relpath(abs_path, repo_root)
        except Exception:
            rel = abs_path
        compiled.add(rel.replace(os.path.sep, '/'))
    return compiled

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compile', default='build/compile_commands.json', help='Path to compile_commands.json')
    p.add_argument('--repo-root', default=None, help='Repo root (defaults to git toplevel)')
    p.add_argument('--show-headers', action='store_true', help='Include headers in output (default true)')
    args = p.parse_args()

    repo_root = args.repo_root or git_toplevel()
    compile_path = os.path.abspath(args.compile)

    if not os.path.isfile(compile_path):
        print(f"ERROR: compile_commands.json not found at {compile_path}")
        raise SystemExit(2)

    compiled = load_compiled(compile_path, repo_root)
    tracked = git_ls_files(repo_root)

    tracked_sources = [p for p in tracked if os.path.splitext(p)[1] in SOURCE_EXTS]
    tracked_headers = [p for p in tracked if os.path.splitext(p)[1] in HEADER_EXTS]

    compiled_norm = set(p.replace(os.path.sep, '/') for p in compiled)

    sources_not_compiled = sorted([p for p in tracked_sources if p not in compiled_norm])
    headers_not_compiled = sorted([p for p in tracked_headers if p not in compiled_norm])

    print("# Sources tracked but not in compile_commands.json:")
    for p in sources_not_compiled:
        print(p)
    print("\n# Headers tracked but not in compile_commands.json:")
    for p in headers_not_compiled:
        print(p)

    print(f"\nSummary: {len(sources_not_compiled)} sources, {len(headers_not_compiled)} headers not compiled (tracked by git).")

if __name__ == '__main__':
    main()

