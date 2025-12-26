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
OCCT 8.0.0 Typedef Header Removal Script

Removes typedef-only header files identified by collect_typedefs.py.
Also updates FILES.cmake files to remove references to deleted headers.

Usage:
    python3 remove_typedef_headers.py [options] <src_directory>

Options:
    --input FILE        Input JSON from collect_typedefs.py (default: collected_typedefs.json)
    --dry-run           Preview changes without modifying/deleting files
    --verbose           Show detailed progress
    --keep-backup       Move files to backup directory instead of deleting
"""

import os
import re
import json
import shutil
import argparse
from pathlib import Path
from typing import Dict, List, Set, Optional
from dataclasses import dataclass, field


@dataclass
class RemovalResult:
    """Result of removal operations."""
    headers_removed: List[str] = field(default_factory=list)
    cmake_files_updated: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)


class TypedefHeaderRemover:
    """Removes typedef-only headers and updates build files."""

    def __init__(self, src_dir: str, collected_data: Dict,
                 dry_run: bool = False, verbose: bool = False,
                 keep_backup: bool = False):
        self.src_dir = Path(src_dir)
        self.dry_run = dry_run
        self.verbose = verbose
        self.keep_backup = keep_backup
        self.backup_dir = Path('backup_typedef_headers')

        # Get typedef-only headers from collected data
        self.typedef_only_headers: List[str] = collected_data.get('typedef_only_headers', [])

        print(f"Found {len(self.typedef_only_headers)} typedef-only headers to remove")

    def log(self, message: str):
        """Print message if verbose."""
        if self.verbose:
            print(message)

    def get_header_basename(self, header_path: str) -> str:
        """Get the basename of a header file."""
        return os.path.basename(header_path)

    def find_cmake_file(self, header_path: str) -> Optional[Path]:
        """Find the FILES.cmake that contains the header."""
        header_dir = (self.src_dir / header_path).parent
        cmake_file = header_dir / 'FILES.cmake'

        if cmake_file.exists():
            return cmake_file

        return None

    def update_cmake_file(self, cmake_path: Path, headers_to_remove: Set[str]) -> bool:
        """
        Update FILES.cmake to remove references to deleted headers.
        Returns True if file was modified.
        """
        try:
            with open(cmake_path, 'r', encoding='utf-8') as f:
                content = f.read()
        except Exception as e:
            print(f"Error reading {cmake_path}: {e}")
            return False

        original = content
        modified = content

        for header_name in headers_to_remove:
            # Remove the header from the cmake file
            # Handle various formats:
            #   HeaderName.hxx
            #   ${PACKAGE}_HeaderName.hxx
            #   HeaderName.hxx  # with trailing spaces/tabs

            # Pattern 1: Just the header name on a line
            pattern1 = rf'^\s*{re.escape(header_name)}\s*$'
            modified = re.sub(pattern1, '', modified, flags=re.MULTILINE)

            # Pattern 2: Header with possible variable prefix
            pattern2 = rf'^\s*\$\{{[^}}]+\}}_{re.escape(header_name)}\s*$'
            modified = re.sub(pattern2, '', modified, flags=re.MULTILINE)

        # Clean up multiple blank lines
        modified = re.sub(r'\n{3,}', '\n\n', modified)

        if modified != original:
            if not self.dry_run:
                with open(cmake_path, 'w', encoding='utf-8') as f:
                    f.write(modified)
            return True

        return False

    def remove_header(self, header_path: str, result: RemovalResult):
        """Remove a single header file."""
        full_path = self.src_dir / header_path

        if not full_path.exists():
            self.log(f"  Header not found (already removed?): {header_path}")
            return

        self.log(f"  Removing: {header_path}")

        if not self.dry_run:
            if self.keep_backup:
                # Create backup directory structure
                backup_path = self.backup_dir / header_path
                backup_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.move(str(full_path), str(backup_path))
            else:
                full_path.unlink()

        result.headers_removed.append(header_path)

    def run(self) -> RemovalResult:
        """Run the removal process."""
        result = RemovalResult()

        if not self.typedef_only_headers:
            print("No typedef-only headers to remove.")
            return result

        print(f"\n{'DRY RUN: ' if self.dry_run else ''}Removing {len(self.typedef_only_headers)} typedef-only headers...")

        # Group headers by their directory (for CMAKE updates)
        headers_by_dir: Dict[str, Set[str]] = {}

        for header_path in self.typedef_only_headers:
            dir_path = os.path.dirname(header_path)
            header_name = os.path.basename(header_path)

            if dir_path not in headers_by_dir:
                headers_by_dir[dir_path] = set()
            headers_by_dir[dir_path].add(header_name)

        # Process each directory
        for dir_path, header_names in headers_by_dir.items():
            self.log(f"\nProcessing directory: {dir_path}")

            # Remove the headers
            for header_name in header_names:
                header_path = os.path.join(dir_path, header_name)
                self.remove_header(header_path, result)

            # Update FILES.cmake
            cmake_path = self.src_dir / dir_path / 'FILES.cmake'
            if cmake_path.exists():
                if self.update_cmake_file(cmake_path, header_names):
                    self.log(f"  Updated: {cmake_path}")
                    result.cmake_files_updated.append(str(cmake_path))

        return result


def print_summary(result: RemovalResult, dry_run: bool):
    """Print removal summary."""
    print("\n" + "=" * 60)
    print("TYPEDEF HEADER REMOVAL SUMMARY")
    print("=" * 60)

    if dry_run:
        print("(DRY RUN - no files were modified)")

    print(f"\nHeaders {'to be ' if dry_run else ''}removed: {len(result.headers_removed)}")
    print(f"CMAKE files {'to be ' if dry_run else ''}updated: {len(result.cmake_files_updated)}")

    if result.errors:
        print(f"\nErrors ({len(result.errors)}):")
        for error in result.errors[:10]:
            print(f"  {error}")

    if result.headers_removed:
        print(f"\nRemoved headers (first 20):")
        for header in result.headers_removed[:20]:
            print(f"  {header}")
        if len(result.headers_removed) > 20:
            print(f"  ... and {len(result.headers_removed) - 20} more")


def main():
    parser = argparse.ArgumentParser(
        description='OCCT 8.0.0 Typedef Header Removal Script'
    )
    parser.add_argument(
        'src_directory',
        nargs='?',
        default='.',
        help='Source directory (default: current directory)'
    )
    parser.add_argument(
        '--input', '-i',
        default='collected_typedefs.json',
        help='Input JSON file from collect_typedefs.py (default: collected_typedefs.json)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Preview changes without modifying/deleting files'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Show detailed progress'
    )
    parser.add_argument(
        '--keep-backup',
        action='store_true',
        help='Move files to backup directory instead of deleting'
    )

    args = parser.parse_args()

    print("OCCT Typedef Header Remover")
    print("=" * 60)

    # Load collected data
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: {input_path} not found. Run collect_typedefs.py first.")
        return 1

    with open(input_path, 'r', encoding='utf-8') as f:
        collected_data = json.load(f)

    remover = TypedefHeaderRemover(
        src_dir=args.src_directory,
        collected_data=collected_data,
        dry_run=args.dry_run,
        verbose=args.verbose,
        keep_backup=args.keep_backup
    )

    result = remover.run()

    # Print summary
    print_summary(result, args.dry_run)

    # Save results
    output_file = 'removal_results.json'
    results_dict = {
        'headers_removed': result.headers_removed,
        'cmake_files_updated': result.cmake_files_updated,
        'errors': result.errors,
        'dry_run': args.dry_run
    }
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(results_dict, f, indent=2, ensure_ascii=False)
    print(f"\nResults saved to: {output_file}")

    if args.dry_run:
        print("\nRun without --dry-run to apply changes")
    else:
        if args.keep_backup:
            print(f"\nBackups saved to: backup_typedef_headers/")
        print("\nTypedef header removal complete!")
        print("Don't forget to rebuild and test the project.")


if __name__ == '__main__':
    main()
