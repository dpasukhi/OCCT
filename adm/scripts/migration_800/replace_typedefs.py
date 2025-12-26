#!/usr/bin/env python3
# Copyright (c) 2025 OPEN CASCADE SAS
#
# This file is part of Open CASCADE Technology software library.
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License version 2.1 as published
# by the Free Software Foundation, with special exception defined in the file
# OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
# distribution for complete text of the license and disclaimer of any warranty.
#
# Alternatively, this file may be used under the terms of Open CASCADE
# commercial license or contractual agreement.

"""
OCCT 8.0.0 Typedef Replacement Script (Optimized)

Replaces NCollection typedef usages with direct NCollection_* types.
Uses optimized single-pass regex matching for speed.

Usage:
    python3 replace_typedefs.py [options] <src_directory>

Options:
    --input FILE        Input JSON from collect_typedefs.py
    --dry-run           Preview changes without modifying files
    --verbose           Show detailed progress
    --file PATH         Process only a specific file
    --jobs N            Number of parallel jobs (default: 4)
"""

import os
import re
import json
import argparse
from pathlib import Path
from typing import Dict, List, Set, Tuple
from concurrent.futures import ProcessPoolExecutor, as_completed
import multiprocessing

# Global state for workers
TYPEDEF_MAP: Dict[str, Tuple[str, str]] = {}  # name -> (full_type, collection_type)
TYPEDEF_PATTERN: re.Pattern = None
TYPEDEF_ONLY_HEADERS: Set[str] = set()
DRY_RUN: bool = False
SRC_DIR: str = ""

SKIP_DIRS = {'install', 'build', 'mac64', 'win64', 'lin64', '.git', '__pycache__'}
SOURCE_EXTENSIONS = {'.hxx', '.cxx', '.lxx', '.pxx', '.gxx', '.h', '.c', '.mm'}


def init_worker(typedef_map, pattern_str, typedef_only_headers, dry_run, src_dir):
    """Initialize worker with shared data."""
    global TYPEDEF_MAP, TYPEDEF_PATTERN, TYPEDEF_ONLY_HEADERS, DRY_RUN, SRC_DIR
    TYPEDEF_MAP = typedef_map
    TYPEDEF_PATTERN = re.compile(pattern_str)
    TYPEDEF_ONLY_HEADERS = typedef_only_headers
    DRY_RUN = dry_run
    SRC_DIR = src_dir


def process_file(file_path_str: str) -> Dict:
    """Process a single file."""
    file_path = Path(file_path_str)
    result = {
        'file_path': file_path_str,
        'replacements': {},
        'modified': False,
        'error': None
    }

    # Skip typedef-only headers
    try:
        rel_path = str(file_path.relative_to(SRC_DIR))
    except ValueError:
        rel_path = file_path_str

    if rel_path in TYPEDEF_ONLY_HEADERS:
        return result

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        result['error'] = str(e)
        return result

    original = content
    used_collections: Set[str] = set()

    # Process content - multiple passes for typedef-on-typedef
    max_passes = 5
    for _ in range(max_passes):
        new_content = []
        made_changes = False
        in_typedef = False  # Track multi-line typedef definitions

        for line in content.split('\n'):
            stripped = line.strip()

            # Track multi-line typedef definitions
            if stripped.startswith('typedef '):
                in_typedef = True
                new_content.append(line)
                if stripped.endswith(';'):
                    in_typedef = False
                continue

            # Skip continuation of typedef definition
            if in_typedef:
                new_content.append(line)
                if stripped.endswith(';'):
                    in_typedef = False
                continue

            # Skip include lines
            if stripped.startswith('#include'):
                new_content.append(line)
                continue

            # Find and replace all typedef usages in this line
            new_line = line
            for match in TYPEDEF_PATTERN.finditer(line):
                typedef_name = match.group(0)
                if typedef_name in TYPEDEF_MAP:
                    full_type, col_type = TYPEDEF_MAP[typedef_name]
                    new_line = re.sub(r'\b' + re.escape(typedef_name) + r'\b', full_type, new_line)
                    result['replacements'][typedef_name] = result['replacements'].get(typedef_name, 0) + 1
                    used_collections.add(col_type)
                    made_changes = True

            new_content.append(new_line)

        content = '\n'.join(new_content)
        if not made_changes:
            break

    if content != original:
        result['modified'] = True
        result['collections'] = list(used_collections)
        if not DRY_RUN:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)

    return result


def get_source_files(src_dir: Path) -> List[str]:
    """Get all source files."""
    files = []
    for ext in SOURCE_EXTENSIONS:
        for f in src_dir.rglob(f'*{ext}'):
            if not any(skip in f.parts for skip in SKIP_DIRS):
                files.append(str(f))
    return sorted(files)


def main():
    parser = argparse.ArgumentParser(description='OCCT Typedef Replacement Script')
    parser.add_argument('src_directory', nargs='?', default='.', help='Source directory')
    parser.add_argument('--input', '-i', default='collected_typedefs.json', help='Input JSON')
    parser.add_argument('--dry-run', action='store_true', help='Preview only')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--file', dest='single_file', help='Process single file')
    parser.add_argument('--jobs', '-j', type=int, default=4, help='Parallel jobs (default: 4)')

    args = parser.parse_args()

    print("OCCT Typedef Replacer (Optimized)")
    print("=" * 60)

    # Load data
    with open(args.input, 'r') as f:
        data = json.load(f)

    # Build optimized lookup
    typedef_map: Dict[str, Tuple[str, str]] = {}
    typedef_names = []

    for td in data.get('typedefs', []):
        name = td['typedef_name']
        full_type = td['full_type']
        col_type = td['collection_type']
        namespace = td.get('namespace', '')

        typedef_map[name] = (full_type, col_type)
        typedef_names.append(name)

        # Also add namespace-qualified version if typedef is in a namespace
        if namespace:
            qualified_name = f"{namespace}::{name}"
            typedef_map[qualified_name] = (full_type, col_type)
            typedef_names.append(qualified_name)

    # Create single combined pattern - sort by length (longest first) for correct matching
    typedef_names.sort(key=len, reverse=True)
    pattern_str = r'\b(' + '|'.join(re.escape(n) for n in typedef_names) + r')\b'

    typedef_only_headers = set(data.get('typedef_only_headers', []))

    print(f"Loaded {len(typedef_map)} typedef replacements")
    print(f"Using {args.jobs} parallel workers")
    if args.dry_run:
        print("(DRY RUN)")

    # Get files
    src_dir = Path(args.src_directory)
    files = [args.single_file] if args.single_file else get_source_files(src_dir)
    print(f"Processing {len(files)} files...")

    # Process in parallel
    total_replacements: Dict[str, int] = {}
    modified_count = 0
    processed = 0

    with ProcessPoolExecutor(
        max_workers=args.jobs,
        initializer=init_worker,
        initargs=(typedef_map, pattern_str, typedef_only_headers, args.dry_run, str(src_dir))
    ) as executor:
        futures = {executor.submit(process_file, f): f for f in files}

        for future in as_completed(futures):
            processed += 1
            if processed % 2000 == 0:
                print(f"Progress: {processed}/{len(files)}")

            try:
                result = future.result()
                if result['modified']:
                    modified_count += 1
                    if args.verbose:
                        print(f"  Modified: {result['file_path']}")

                for name, count in result.get('replacements', {}).items():
                    total_replacements[name] = total_replacements.get(name, 0) + count

            except Exception as e:
                print(f"Error: {futures[future]}: {e}")

    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Files processed: {len(files)}")
    print(f"Files modified: {modified_count}")
    print(f"Total replacements: {sum(total_replacements.values())}")

    if total_replacements:
        print("\nTop 15 replaced typedefs:")
        for name, count in sorted(total_replacements.items(), key=lambda x: -x[1])[:15]:
            print(f"  {name}: {count}")

    # Save results
    with open('replacement_results.json', 'w') as f:
        json.dump({
            'files_modified': modified_count,
            'total_replacements': sum(total_replacements.values()),
            'by_typedef': total_replacements,
            'dry_run': args.dry_run
        }, f, indent=2)

    if args.dry_run:
        print("\nRun without --dry-run to apply changes")


if __name__ == '__main__':
    main()
