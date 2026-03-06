#!/usr/bin/env python3
"""
Generate FM synthesizer quarter-sine lookup table.

This script generates a pre-computed quarter-wave sine table (0° to 90°)
for the FM synthesizer to use in fast fixed-point synthesis.

The table size is read from FM_SINE_QUARTER_SIZE in cfg.h or fm_synth.h.

Usage:
    python3 gen_fm_sine_table.py [output_file] [source_dir]

Default output: fm_sine_quarter.inc
Default source: .
"""

import math
import sys
import os
import re


def read_table_size(source_dir="."):
    """
    Read FM_SINE_QUARTER_SIZE from cfg.h or fm_synth.h.
    
    Args:
        source_dir: Directory containing the header files
        
    Returns:
        Table size as integer, or 1024 if not found
    """
    table_size = 1024  # Default fallback
    
    # Try cfg.h first
    cfg_h_path = os.path.join(source_dir, "cfg.h")
    if os.path.exists(cfg_h_path):
        with open(cfg_h_path, 'r') as f:
            content = f.read()
            match = re.search(r'#define\s+FM_SINE_QUARTER_SIZE\s+(\d+)', content)
            if match:
                return int(match.group(1))
    
    # Fall back to fm_synth.h
    fm_synth_h_path = os.path.join(source_dir, "fm_synth.h")
    if os.path.exists(fm_synth_h_path):
        with open(fm_synth_h_path, 'r') as f:
            content = f.read()
            match = re.search(r'#define\s+FM_SINE_QUARTER_SIZE\s+(\d+)', content)
            if match:
                return int(match.group(1))
    
    return table_size


def generate_sine_table(table_size, output_file="fm_sine_quarter.inc"):
    """
    Generate a quarter-sine lookup table and write to C include file.
    
    Args:
        table_size: Number of entries
        output_file: Output filename for the C include file
    """
    values = []
    
    # Generate sine values from 0 to pi/2 radians (0° to 90°)
    for i in range(table_size):
        angle = (i / table_size) * (math.pi / 2)
        # Scale to 16-bit signed range (±32767)
        value = int(math.sin(angle) * 32767)
        values.append(value)
    
    # Write in C array format (12 values per line for readability)
    with open(output_file, 'w') as f:
        for i in range(0, len(values), 12):
            chunk = values[i:i+12]
            # Format each line with proper spacing
            line = "    " + ", ".join(f"{v:6d}" for v in chunk)
            
            # Add comma unless it's the last line
            if i + 12 < len(values):
                line += ","
            
            f.write(line + "\n")
    
    return len(values)


if __name__ == "__main__":
    # Parse command-line arguments
    output_file = sys.argv[1] if len(sys.argv) > 1 else "fm_sine_quarter.inc"
    source_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    
    table_size = read_table_size(source_dir)
    count = generate_sine_table(table_size, output_file=output_file)
    print(f"Generated {count} sine table entries (size={table_size}) in {output_file}")
