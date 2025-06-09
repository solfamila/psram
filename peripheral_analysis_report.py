#!/usr/bin/env python3
"""
Peripheral Analysis Report Generator
Converts JSON analysis results into human-readable reports
"""

import json
import sys
from collections import defaultdict

def format_address(addr_str):
    """Format address string for better readability"""
    try:
        addr = int(addr_str, 16) if addr_str.startswith('0x') else int(addr_str)
        return f"0x{addr:08X}"
    except:
        return addr_str

def format_bits(bits_list):
    """Format bit list for better readability"""
    if not bits_list:
        return "None"
    
    # Group consecutive bits
    individual_bits = []
    ranges = []
    
    for bit_str in bits_list:
        if '-' in bit_str:
            ranges.append(bit_str)
        else:
            individual_bits.append(bit_str)
    
    result = []
    if individual_bits:
        result.extend(individual_bits)
    if ranges:
        result.extend(ranges)
    
    return ", ".join(result)

def generate_report(json_file, output_file=None):
    """Generate a comprehensive analysis report"""
    
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    report_lines = []
    
    # Header
    report_lines.append("=" * 80)
    report_lines.append("MIMXRT700 XSPI PSRAM Peripheral Register Access Analysis Report")
    report_lines.append("=" * 80)
    report_lines.append("")
    
    # Summary statistics
    total_accesses = 0
    peripheral_stats = defaultdict(lambda: {'reads': 0, 'writes': 0, 'rmw': 0})
    register_stats = defaultdict(int)
    
    for peripheral in data['peripheral_accesses']:
        peripheral_name = peripheral['peripheral_name']
        accesses = peripheral['accesses']
        total_accesses += len(accesses)
        
        for access in accesses:
            access_type = access['access_type']
            register_name = access['register_name']
            
            if access_type == 'read':
                peripheral_stats[peripheral_name]['reads'] += 1
            elif access_type == 'write':
                peripheral_stats[peripheral_name]['writes'] += 1
            elif access_type == 'read-modify-write':
                peripheral_stats[peripheral_name]['rmw'] += 1
            
            register_stats[f"{peripheral_name}::{register_name}"] += 1
    
    # Summary section
    report_lines.append("EXECUTIVE SUMMARY")
    report_lines.append("-" * 40)
    report_lines.append(f"Total Register Accesses: {total_accesses}")
    report_lines.append(f"Peripherals Analyzed: {len(data['peripheral_accesses'])}")
    report_lines.append("")
    
    # Peripheral summary
    report_lines.append("PERIPHERAL ACCESS SUMMARY")
    report_lines.append("-" * 40)
    for peripheral_name, stats in peripheral_stats.items():
        total = stats['reads'] + stats['writes'] + stats['rmw']
        report_lines.append(f"{peripheral_name}:")
        report_lines.append(f"  Total Accesses: {total}")
        report_lines.append(f"  Reads: {stats['reads']}")
        report_lines.append(f"  Writes: {stats['writes']}")
        report_lines.append(f"  Read-Modify-Write: {stats['rmw']}")
        report_lines.append("")
    
    # Most accessed registers
    report_lines.append("MOST ACCESSED REGISTERS")
    report_lines.append("-" * 40)
    sorted_registers = sorted(register_stats.items(), key=lambda x: x[1], reverse=True)
    for reg_name, count in sorted_registers[:10]:
        peripheral, register = reg_name.split("::")
        report_lines.append(f"{peripheral} {register}: {count} accesses")
    report_lines.append("")
    
    # Detailed analysis by peripheral
    report_lines.append("DETAILED PERIPHERAL ANALYSIS")
    report_lines.append("=" * 80)
    
    for peripheral in data['peripheral_accesses']:
        peripheral_name = peripheral['peripheral_name']
        base_address = peripheral.get('base_address', 'Unknown')
        accesses = peripheral['accesses']
        
        report_lines.append("")
        report_lines.append(f"PERIPHERAL: {peripheral_name}")
        report_lines.append(f"Base Address: {format_address(base_address)}")
        report_lines.append("-" * 60)
        
        # Group accesses by register
        register_accesses = defaultdict(list)
        for access in accesses:
            register_accesses[access['register_name']].append(access)
        
        for register_name, reg_accesses in register_accesses.items():
            report_lines.append(f"\nRegister: {register_name}")
            report_lines.append(f"Address: {format_address(reg_accesses[0]['address'])}")
            report_lines.append(f"Total Accesses: {len(reg_accesses)}")
            
            # Access details
            for i, access in enumerate(reg_accesses, 1):
                report_lines.append(f"\n  Access #{i}:")
                report_lines.append(f"    Type: {access['access_type']}")
                report_lines.append(f"    Data Size: {access['data_size']} bits")
                report_lines.append(f"    Bits Modified: {format_bits(access['bits_modified'])}")
                report_lines.append(f"    Function: {access['source_location']['function']}")
                report_lines.append(f"    File: {access['source_location']['file']}")
                report_lines.append(f"    Line: {access['source_location']['line']}")
                report_lines.append(f"    Purpose: {access['purpose']}")
    
    # Security analysis
    report_lines.append("\n\nSECURITY ANALYSIS")
    report_lines.append("-" * 40)
    
    secure_accesses = 0
    non_secure_accesses = 0
    
    for peripheral in data['peripheral_accesses']:
        peripheral_name = peripheral['peripheral_name']
        if 'XSPI0' in peripheral_name and 'NS' not in peripheral_name:
            secure_accesses += len(peripheral['accesses'])
        else:
            non_secure_accesses += len(peripheral['accesses'])
    
    report_lines.append(f"Secure Peripheral Accesses: {secure_accesses}")
    report_lines.append(f"Non-Secure Peripheral Accesses: {non_secure_accesses}")
    
    if secure_accesses > 0:
        report_lines.append("\nSECURITY RECOMMENDATION:")
        report_lines.append("- Review secure peripheral access patterns")
        report_lines.append("- Ensure proper TrustZone configuration")
        report_lines.append("- Validate access permissions for secure resources")
    
    # Performance analysis
    report_lines.append("\n\nPERFORMANCE ANALYSIS")
    report_lines.append("-" * 40)
    
    # Count read-modify-write operations (potentially inefficient)
    rmw_count = sum(1 for peripheral in data['peripheral_accesses'] 
                   for access in peripheral['accesses'] 
                   if access['access_type'] == 'read-modify-write')
    
    report_lines.append(f"Read-Modify-Write Operations: {rmw_count}")
    if rmw_count > 0:
        report_lines.append("PERFORMANCE RECOMMENDATIONS:")
        report_lines.append("- Consider batching register updates where possible")
        report_lines.append("- Review if atomic operations are necessary")
        report_lines.append("- Optimize critical path register accesses")
    
    # Write report to file or stdout
    report_text = "\n".join(report_lines)
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(report_text)
        print(f"Report written to: {output_file}")
    else:
        print(report_text)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 peripheral_analysis_report.py <json_file> [output_file]")
        sys.exit(1)
    
    json_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    generate_report(json_file, output_file)
