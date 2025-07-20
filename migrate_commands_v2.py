#!/usr/bin/env python3
"""
OCCT Command Migration Script V2
===============================

Improved version that handles multiline strings properly by using AST parsing approach.
"""

import re
import os
import sys
import argparse
import shutil
from pathlib import Path

class CommandMigrator:
    def __init__(self, dry_run=False, backup=False):
        self.dry_run = dry_run
        self.backup = backup
        self.changes_made = 0
        self.files_processed = 0

    def migrate_file(self, filepath):
        """Migrate a single file."""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
        except UnicodeDecodeError:
            print(f"Warning: Could not read {filepath} as UTF-8, skipping...")
            return False
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            return False
            
        original_content = content
        changes_in_file = 0
        
        # Step 1: Replace header includes
        if '#include <Draw_Interpretor.hxx>' in content:
            content = content.replace('#include <Draw_Interpretor.hxx>', '#include <Draw_CommandInterface.hxx>')
            changes_in_file += 1
            if not self.dry_run:
                print("  ✓ Updated header include")
        
        # Step 2: Update function signatures - handle multiline case
        # Pattern: void ClassName::Commands(Draw_Interpretor& theCommands)
        signature_pattern = r'(\w+::\w+\s*\(\s*)Draw_Interpretor(\s*&\s*\w+\s*\))'
        new_content, count = re.subn(signature_pattern, r'\1DRAW_INTERPRETOR\2', content)
        if count > 0:
            content = new_content
            changes_in_file += count
            if not self.dry_run:
                print(f"  ✓ Updated function signatures: {count} changes")
        
        # Step 3: Find and replace .Add() calls using a more robust approach
        # This handles multiline strings by finding the complete .Add() call
        add_calls = self.find_add_calls(content)
        
        for old_call, new_call in add_calls:
            if old_call in content:
                content = content.replace(old_call, new_call)
                changes_in_file += 1
                if not self.dry_run:
                    print("  ✓ Converted .Add() call")
        
        # If changes were made
        if changes_in_file > 0:
            self.changes_made += changes_in_file
            
            if self.dry_run:
                print(f"[DRY RUN] Would modify {filepath}: {changes_in_file} changes")
                return True
            
            # Create backup if requested
            if self.backup:
                backup_path = str(filepath) + '.backup'
                shutil.copy2(filepath, backup_path)
                print(f"  ✓ Backup created: {backup_path}")
            
            # Write the modified content
            try:
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(content)
                print(f"✓ Modified {filepath}: {changes_in_file} changes")
                return True
            except Exception as e:
                print(f"Error writing {filepath}: {e}")
                return False
        
        return False

    def find_add_calls(self, content):
        """Find all .Add() calls and return (old, new) pairs."""
        replacements = []
        
        # Find all .Add() calls using a more flexible approach
        # This regex finds the start of .Add calls and then we'll parse the complete call
        add_pattern = r'(\w+)\.Add\s*\('
        
        pos = 0
        while True:
            match = re.search(add_pattern, content[pos:])
            if not match:
                break
            
            # Find the complete .Add() call by counting parentheses
            start_pos = pos + match.start()
            call_start = pos + match.start()
            paren_pos = pos + match.end() - 1  # Position of opening parenthesis
            
            # Count parentheses to find the end of the call
            paren_count = 1
            i = paren_pos + 1
            while i < len(content) and paren_count > 0:
                if content[i] == '(':
                    paren_count += 1
                elif content[i] == ')':
                    paren_count -= 1
                i += 1
            
            if paren_count == 0:
                # Find the semicolon
                while i < len(content) and content[i] in ' \t\n':
                    i += 1
                if i < len(content) and content[i] == ';':
                    i += 1
                    
                # Extract the complete call
                full_call = content[call_start:i]
                
                # Try to convert this call
                converted = self.convert_add_call(full_call)
                if converted and converted != full_call:
                    replacements.append((full_call, converted))
            
            pos = call_start + 1
        
        return replacements

    def convert_add_call(self, call_text):
        """Convert a single .Add() call to new format."""
        # Extract components using regex
        # Pattern: object.Add(args...)
        match = re.match(r'(\w+)\.Add\s*\((.*)\)\s*;', call_text, re.DOTALL)
        if not match:
            return None
        
        object_name = match.group(1)
        args_text = match.group(2).strip()
        
        # Parse arguments - this is simplified but handles most cases
        args = self.parse_arguments(args_text)
        
        if len(args) == 5:  # name, help, __FILE__, function, group
            name, help_text, file_arg, func, group = args
            if file_arg.strip() == '__FILE__':
                return f'DRAW_ADD_COMMAND({object_name}, {name}, {help_text}, __FILE__, {func}, {group});'
        elif len(args) == 4:  # name, help, function, group OR name, help, __FILE__, function
            if '__FILE__' in args[2]:
                name, help_text, file_arg, func = args
                return f'DRAW_ADD_COMMAND({object_name}, {name}, {help_text}, __FILE__, {func}, "User Commands");'
            else:
                name, help_text, func, group = args
                return f'DRAW_ADD_SIMPLE_COMMAND({object_name}, {name}, {help_text}, {func}, {group});'
        elif len(args) == 3:  # name, help, function
            name, help_text, func = args
            return f'DRAW_ADD_DEFAULT_COMMAND({object_name}, {name}, {help_text}, {func});'
        
        return None

    def parse_arguments(self, args_text):
        """Parse the arguments of an .Add() call."""
        args = []
        current_arg = ""
        paren_count = 0
        quote_char = None
        i = 0
        
        while i < len(args_text):
            char = args_text[i]
            
            if quote_char:
                current_arg += char
                if char == quote_char and (i == 0 or args_text[i-1] != '\\'):
                    quote_char = None
            elif char in '"\'':
                current_arg += char
                quote_char = char
            elif char == '(':
                current_arg += char
                paren_count += 1
            elif char == ')':
                current_arg += char
                paren_count -= 1
            elif char == ',' and paren_count == 0:
                args.append(current_arg.strip())
                current_arg = ""
            else:
                current_arg += char
            
            i += 1
        
        if current_arg.strip():
            args.append(current_arg.strip())
        
        return args

    def find_command_files(self, path):
        """Find all potential command files in a directory."""
        command_files = []
        
        if os.path.isfile(path):
            return [path]
        
        for root, dirs, files in os.walk(path):
            # Skip build directories
            dirs[:] = [d for d in dirs if not d.startswith('build')]
            
            for file in files:
                if file.endswith(('.cxx', '.cpp', '.hxx', '.hpp')):
                    filepath = os.path.join(root, file)
                    # Check if file likely contains command registrations
                    try:
                        with open(filepath, 'r', encoding='utf-8') as f:
                            content = f.read()
                            if ('Draw_Interpretor' in content and 
                                ('.Add(' in content or 'Commands(' in content)):
                                command_files.append(filepath)
                    except:
                        continue
        
        return command_files

    def migrate(self, path):
        """Migrate all command files in the given path."""
        print(f"Scanning for command files in: {path}")
        
        files_to_migrate = self.find_command_files(path)
        
        if not files_to_migrate:
            print("No command files found to migrate.")
            return
        
        print(f"Found {len(files_to_migrate)} files to migrate:")
        for f in files_to_migrate:
            print(f"  - {f}")
        
        if self.dry_run:
            print("\n--- DRY RUN MODE ---")
        
        print("\nMigrating files...")
        for filepath in files_to_migrate:
            self.files_processed += 1
            if self.migrate_file(filepath):
                pass  # Success message already printed
        
        print(f"\nMigration complete!")
        print(f"Files processed: {self.files_processed}")
        print(f"Total changes made: {self.changes_made}")
        
        if self.dry_run:
            print("\nThis was a dry run. No files were actually modified.")
            print("Run without --dry-run to apply changes.")

def main():
    parser = argparse.ArgumentParser(
        description="Migrate OCCT Draw command registrations to unified format (V2)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    
    parser.add_argument('path', help='File or directory to migrate')
    parser.add_argument('--dry-run', action='store_true', 
                       help='Preview changes without modifying files')
    parser.add_argument('--backup', action='store_true',
                       help='Create backup files (.backup extension)')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.path):
        print(f"Error: Path '{args.path}' does not exist.")
        sys.exit(1)
    
    migrator = CommandMigrator(dry_run=args.dry_run, backup=args.backup)
    migrator.migrate(args.path)

if __name__ == '__main__':
    main()