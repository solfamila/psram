#!/usr/bin/env python3
"""
Generate Complete Enhanced Peripheral Analysis
Combines all enhanced LLVM analysis results to create a comprehensive peripheral access report
"""

import json
import os
import sys
from pathlib import Path

def load_json_file(filepath):
    """Load JSON file and return the data"""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Warning: File {filepath} not found")
        return None
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON file {filepath}: {e}")
        return None

def combine_analysis_results():
    """Combine all enhanced analysis results"""
    
    # List of analysis files to combine
    analysis_files = [
        "enhanced_board_analysis.json",
        "enhanced_pin_mux_analysis.json",
        "enhanced_clock_analysis.json",
        "enhanced_main_analysis.json",
        "enhanced_hardware_init_analysis.json",
        "enhanced_xspi_driver_analysis.json",
        "enhanced_fsl_clock_analysis.json"
    ]
    
    all_accesses = []
    total_files_processed = 0
    
    print("Combining enhanced peripheral analysis results...")
    
    for filename in analysis_files:
        if os.path.exists(filename):
            print(f"Processing {filename}...")
            data = load_json_file(filename)
            if data and 'chronological_sequence' in data:
                accesses = data['chronological_sequence']
                print(f"  Found {len(accesses)} accesses")
                all_accesses.extend(accesses)
                total_files_processed += 1
            else:
                print(f"  No valid data found in {filename}")
        else:
            print(f"  File {filename} not found, skipping...")
    
    print(f"\nProcessed {total_files_processed} files")
    print(f"Total peripheral accesses found: {len(all_accesses)}")
    
    # Sort by sequence number to maintain chronological order
    all_accesses.sort(key=lambda x: x.get('sequence_number', 0))
    
    # Renumber sequence numbers to be consecutive
    for i, access in enumerate(all_accesses):
        access['sequence_number'] = i
    
    # Count accesses by execution phase
    phase_counts = {}
    peripheral_counts = {}
    
    for access in all_accesses:
        phase = access.get('execution_phase', 'unknown')
        peripheral = access.get('peripheral_name', 'unknown')
        
        phase_counts[phase] = phase_counts.get(phase, 0) + 1
        peripheral_counts[peripheral] = peripheral_counts.get(peripheral, 0) + 1
    
    # Create comprehensive report
    report = {
        "analysis_type": "complete_enhanced_peripheral_access_sequence",
        "description": f"Complete peripheral register accesses in chronological execution order - {len(all_accesses)} total accesses",
        "total_accesses": len(all_accesses),
        "files_analyzed": total_files_processed,
        "execution_phase_summary": phase_counts,
        "peripheral_summary": peripheral_counts,
        "chronological_sequence": all_accesses
    }
    
    return report

def main():
    """Main function"""
    print("Enhanced Peripheral Analysis Report Generator")
    print("=" * 50)
    
    # Generate combined report
    report = combine_analysis_results()
    
    # Write to output file
    output_file = "complete_enhanced_peripheral_analysis.json"
    
    try:
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\nComplete enhanced analysis saved to: {output_file}")
        print(f"Total peripheral accesses: {report['total_accesses']}")
        print(f"Execution phases: {list(report['execution_phase_summary'].keys())}")
        print(f"Peripherals accessed: {list(report['peripheral_summary'].keys())}")
        
        # Print summary statistics
        print("\nExecution Phase Summary:")
        for phase, count in report['execution_phase_summary'].items():
            print(f"  {phase}: {count} accesses")
        
        print("\nPeripheral Summary:")
        for peripheral, count in sorted(report['peripheral_summary'].items()):
            print(f"  {peripheral}: {count} accesses")
            
    except Exception as e:
        print(f"Error writing output file: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
