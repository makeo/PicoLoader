#!/usr/bin/env python3
"""
UF2 Merge Tool

A command-line tool to combine multiple UF2 files into a single UF2 file.

This tool provides functionality to combine multiple UF2 files.
It is designed to combine UF2 files with different family IDs.
Note: Does not support advanced UF2 features like partition tables.

Usage:
    python uf2merge.py input1.uf2 input2.uf2 ... -o output.uf2
    python uf2merge.py *.uf2 -o combined.uf2


Copyright (C) 2025 makeo

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

import argparse
import sys
import os
import struct
from typing import List, Dict
from collections import defaultdict

UF2_BLOCK_SIZE = 512


# ============================================================================
# UF2 Parser and Combiner Functions
# ============================================================================

def parse_uf2_blocks(uf2_data: bytes) -> List[bytearray]:
    blocks = []
    for i in range(0, len(uf2_data), UF2_BLOCK_SIZE):
        if i + UF2_BLOCK_SIZE <= len(uf2_data):
            block = bytearray(uf2_data[i:i + UF2_BLOCK_SIZE])
            blocks.append(block)
    
    return blocks


def get_family_id(block: bytearray) -> int:
    # familyID is at offset 28, 4 bytes, little-endian
    return struct.unpack('<I', block[28:32])[0]


def get_target_address(block: bytearray) -> int:
    # target address is at offset 12, 4 bytes, little-endian
    return struct.unpack('<I', block[12:16])[0]


def set_block_number(block: bytearray, block_no: int) -> None:
    # blockNo is at offset 20, 4 bytes, little-endian
    struct.pack_into('<I', block, 20, block_no)


def set_num_blocks(block: bytearray, num_blocks: int) -> None:
    # numBlocks is at offset 24, 4 bytes, little-endian
    struct.pack_into('<I', block, 24, num_blocks)


def combine_uf2_files(uf2_files_data: List[bytes]) -> bytes:
    all_blocks = []
    for uf2_data in uf2_files_data:
        blocks = parse_uf2_blocks(uf2_data)
        all_blocks.extend(blocks)
    
    all_blocks.sort(key=get_target_address)
    
    family_groups: Dict[int, List[bytearray]] = defaultdict(list)
    
    for block in all_blocks:
        family_id = get_family_id(block)
        family_groups[family_id].append(block)
    
    reindexed_blocks = []
    final_blocks = []
    
    for family_id, blocks in family_groups.items():
        total_blocks_in_family = len(blocks)
        
        for index, block in enumerate(blocks):
            new_block = bytearray(block)
            
            set_block_number(new_block, index)
            set_num_blocks(new_block, total_blocks_in_family)
            
            if index == total_blocks_in_family - 1:
                final_blocks.append(new_block)
            else:
                reindexed_blocks.append(new_block)
    
    reindexed_blocks.extend(final_blocks)
    
    final_data = bytearray(len(reindexed_blocks) * UF2_BLOCK_SIZE)
    
    for i, block in enumerate(reindexed_blocks):
        start_offset = i * UF2_BLOCK_SIZE
        final_data[start_offset:start_offset + UF2_BLOCK_SIZE] = block
    
    return bytes(final_data)


# ============================================================================
# File I/O Functions
# ============================================================================

def read_uf2_file(filepath: str) -> bytes:
    try:
        with open(filepath, 'rb') as f:
            return f.read()
    except FileNotFoundError:
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        raise
    except IOError as e:
        print(f"Error reading file '{filepath}': {e}", file=sys.stderr)
        raise


def write_uf2_file(filepath: str, data: bytes) -> None:
    try:
        with open(filepath, 'wb') as f:
            f.write(data)
    except IOError as e:
        print(f"Error writing file '{filepath}': {e}", file=sys.stderr)
        raise


def validate_uf2_files(filepaths: List[str]) -> List[str]:
    validated_files = []
    
    for filepath in filepaths:
        if not os.path.isfile(filepath):
            print(f"Error: File '{filepath}' does not exist.", file=sys.stderr)
            sys.exit(1)
        
        if not filepath.lower().endswith('.uf2'):
            print(f"Warning: File '{filepath}' does not have .uf2 extension.", file=sys.stderr)
        
        validated_files.append(filepath)
    
    return validated_files


# ============================================================================
# Command-Line Interface
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Combine multiple UF2 files into a single UF2 file.",
        epilog="Example: python uf2merge.py input1.uf2 input2.uf2 -o combined.uf2"
    )
    
    parser.add_argument(
        'input_files',
        nargs='+',
        help='Input UF2 files to combine'
    )
    
    parser.add_argument(
        '-o', '--output',
        required=True,
        help='Output UF2 file path'
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    
    args = parser.parse_args()
    
    try:
        input_files = validate_uf2_files(args.input_files)
    except SystemExit:
        return 1
    
    if args.verbose:
        print(f"Combining {len(input_files)} UF2 files:")
        for filepath in input_files:
            print(f"  - {filepath}")
        print(f"Output: {args.output}")
    
    uf2_files_data = []
    try:
        for filepath in input_files:
            if args.verbose:
                print(f"Reading {filepath}...")
            data = read_uf2_file(filepath)
            uf2_files_data.append(data)
            if args.verbose:
                print(f"  Read {len(data)} bytes")
    except (FileNotFoundError, IOError):
        return 1
    
    if args.verbose:
        print("Combining UF2 files...")
    
    try:
        combined_data = combine_uf2_files(uf2_files_data)
        if args.verbose:
            print(f"Combined data size: {len(combined_data)} bytes")
    except Exception as e:
        print(f"Error combining UF2 files: {e}", file=sys.stderr)
        return 1
    
    try:
        if args.verbose:
            print(f"Writing output to {args.output}...")
        write_uf2_file(args.output, combined_data)
        if args.verbose:
            print("Successfully created combined UF2 file!")
        else:
            print(f"Combined {len(input_files)} UF2 files into {args.output}")
    except IOError:
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())