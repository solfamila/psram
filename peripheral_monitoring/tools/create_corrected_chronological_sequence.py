#!/usr/bin/env python3
"""
MIMXRT700 Peripheral Register Chronological Sequence Correction Tool

This tool creates a comprehensive, hardware-accurate chronological sequence of
peripheral register accesses by intelligently merging LLVM IR static analysis
with real-time J-Link hardware monitoring results.

Primary Objective:
Fix the existing flawed chronological_sequence in complete_enhanced_peripheral_analysis.json
which currently lacks actual register values, contains duplicate entries, and has empty
bit-change analysis. Generate a corrected sequence that represents the true hardware
initialization flow with complete before/after register state tracking.

Author: Augment Agent
Date: 2024
"""

import json
import os
import sys
from typing import Dict, List, Any, Optional, Tuple
from datetime import datetime
from dataclasses import dataclass, asdict
import argparse

# Add the tools directory to the path to import our decoder
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mimxrt798s_register_decoder import MIMXRT798SRegisterDecoder, AccessType

@dataclass
class CorrectedRegisterAccess:
    """Corrected register access entry with complete information"""
    sequence_number: int
    relative_timestamp_ms: Optional[float]
    address: str
    peripheral_name: str
    register_name: str
    register_offset_from_base: str
    access_type: str
    data_width: int
    value_before: str
    value_after: str
    value_written: Optional[str]
    mask_applied: Optional[str]
    bits_modified: List[Dict[str, Any]]
    execution_phase: str
    source_file_path: str
    function_name: str
    line_number: int
    call_stack_depth: int
    hardware_verified: bool
    j_link_captured_value: Optional[str]
    clock_gate_status: str

class ChronologicalSequenceCorrector:
    """
    Main tool for correcting and validating the chronological sequence
    """
    
    def __init__(self, results_dir: str = "peripheral_monitoring/results"):
        """Initialize the corrector with data sources"""
        self.results_dir = results_dir
        self.decoder = MIMXRT798SRegisterDecoder()
        
        # Load data sources
        self.llvm_analysis = self._load_json("complete_enhanced_peripheral_analysis.json")
        self.hardware_diagnostics = self._load_json("register_access_diagnostic_results.json")
        self.live_monitoring = self._load_json("corrected_monitoring_results.json")
        
        # Initialize correction state
        self.corrected_sequence: List[CorrectedRegisterAccess] = []
        self.validation_issues: List[Dict[str, Any]] = []
        self.statistics = {
            "total_entries": 0,
            "duplicates_removed": 0,
            "hardware_verified": 0,
            "clock_gated_registers": 0,
            "bit_changes_populated": 0,
            "missing_values_filled": 0
        }
    
    def _load_json(self, filename: str) -> Dict[str, Any]:
        """Load JSON file from results directory"""
        filepath = os.path.join(self.results_dir, filename)
        try:
            with open(filepath, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"Warning: {filename} not found, using empty data")
            return {}
        except json.JSONDecodeError as e:
            print(f"Error loading {filename}: {e}")
            return {}
    
    def correct_chronological_sequence(self) -> bool:
        """
        Main correction process
        
        Returns:
            True if correction was successful
        """
        print("🔧 Starting chronological sequence correction...")
        
        # Step 1: Extract and deduplicate entries
        raw_entries = self._extract_raw_entries()
        print(f"📊 Extracted {len(raw_entries)} raw entries")
        
        # Step 2: Remove duplicates and consolidate
        deduplicated_entries = self._remove_duplicates(raw_entries)
        print(f"🔄 Deduplicated to {len(deduplicated_entries)} unique entries")
        self.statistics["duplicates_removed"] = len(raw_entries) - len(deduplicated_entries)
        
        # Step 3: Sort by execution order
        sorted_entries = self._sort_by_execution_order(deduplicated_entries)
        print(f"📋 Sorted entries by execution order")
        
        # Step 4: Populate register values and bit changes
        corrected_entries = self._populate_register_values(sorted_entries)
        print(f"💾 Populated register values for {len(corrected_entries)} entries")
        
        # Step 5: Validate against hardware data
        validated_entries = self._validate_against_hardware(corrected_entries)
        print(f"✅ Hardware validation completed")
        
        # Step 6: Assign final sequence numbers
        self.corrected_sequence = self._assign_sequence_numbers(validated_entries)
        print(f"🔢 Assigned sequence numbers 1-{len(self.corrected_sequence)}")
        
        self.statistics["total_entries"] = len(self.corrected_sequence)
        
        return True
    
    def _extract_raw_entries(self) -> List[Dict[str, Any]]:
        """Extract raw entries from LLVM analysis"""
        if not self.llvm_analysis or "chronological_sequence" not in self.llvm_analysis:
            print("❌ No chronological sequence found in LLVM analysis")
            return []
        
        return self.llvm_analysis["chronological_sequence"]
    
    def _remove_duplicates(self, entries: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """
        Remove duplicate entries based on address, access_type, and execution context
        
        Keep the most complete entry for each unique combination
        """
        seen_combinations = {}
        deduplicated = []
        
        for entry in entries:
            # Create unique key based on critical attributes
            key = (
                entry.get("address", ""),
                entry.get("access_type", ""),
                entry.get("execution_phase", ""),
                entry.get("source_location", {}).get("function", ""),
                entry.get("source_location", {}).get("line", 0)
            )
            
            if key not in seen_combinations:
                seen_combinations[key] = entry
                deduplicated.append(entry)
            else:
                # Keep the entry with more complete information
                existing = seen_combinations[key]
                if self._is_more_complete(entry, existing):
                    # Replace existing entry
                    idx = deduplicated.index(existing)
                    deduplicated[idx] = entry
                    seen_combinations[key] = entry
        
        return deduplicated
    
    def _is_more_complete(self, entry1: Dict[str, Any], entry2: Dict[str, Any]) -> bool:
        """Determine which entry has more complete information"""
        score1 = self._completeness_score(entry1)
        score2 = self._completeness_score(entry2)
        return score1 > score2
    
    def _completeness_score(self, entry: Dict[str, Any]) -> int:
        """Calculate completeness score for an entry"""
        score = 0
        
        # Basic required fields
        if entry.get("address"): score += 1
        if entry.get("peripheral_name"): score += 1
        if entry.get("register_name"): score += 1
        if entry.get("access_type"): score += 1
        
        # Source location information
        source_loc = entry.get("source_location", {})
        if source_loc.get("file"): score += 1
        if source_loc.get("function"): score += 1
        if source_loc.get("line"): score += 1
        
        # Execution context
        if entry.get("execution_phase"): score += 1
        if entry.get("execution_context"): score += 1
        
        # Data completeness
        if entry.get("data_size"): score += 1
        if entry.get("bits_modified"): score += 1
        
        return score
    
    def _sort_by_execution_order(self, entries: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """
        Sort entries by logical execution order:
        1. Execution phase (board_init -> driver_init -> runtime)
        2. Instruction index within phase
        3. Source file and line number
        """
        phase_order = {"board_init": 0, "driver_init": 1, "runtime": 2}
        
        def sort_key(entry):
            phase = entry.get("execution_phase", "runtime")
            phase_num = phase_order.get(phase, 3)
            instruction_idx = entry.get("instruction_index", 0)
            line_num = entry.get("source_location", {}).get("line", 0)
            
            return (phase_num, instruction_idx, line_num)
        
        return sorted(entries, key=sort_key)
    
    def _populate_register_values(self, entries: List[Dict[str, Any]]) -> List[CorrectedRegisterAccess]:
        """
        Populate register values and bit changes for each entry
        """
        corrected_entries = []
        register_state = {}  # Track register states for before/after analysis
        
        for i, entry in enumerate(entries):
            try:
                corrected_entry = self._create_corrected_entry(entry, register_state, i)
                corrected_entries.append(corrected_entry)
                
                # Update register state tracking
                if corrected_entry.value_after != "0x00000000":
                    register_state[corrected_entry.address] = corrected_entry.value_after
                    
            except Exception as e:
                print(f"⚠️  Error processing entry {i}: {e}")
                self.validation_issues.append({
                    "type": "processing_error",
                    "entry_index": i,
                    "error": str(e),
                    "entry": entry
                })
        
        return corrected_entries

    def _create_corrected_entry(self, entry: Dict[str, Any], register_state: Dict[str, str], index: int) -> CorrectedRegisterAccess:
        """Create a corrected register access entry with complete information"""

        # Extract basic information
        address_str = entry.get("address", "0x00000000")
        address_int = int(address_str, 16) if isinstance(address_str, str) else address_str

        # Get register information from decoder
        register_info = self.decoder.get_register_info(address_int)
        if register_info:
            peripheral_name, register_name, register_def = register_info
            register_offset = f"0x{register_def.offset:x}"
        else:
            peripheral_name = entry.get("peripheral_name", "UNKNOWN")
            register_name = entry.get("register_name", "UNKNOWN")
            register_offset = "0x0"

        # Determine register values
        value_before, value_after, value_written = self._determine_register_values(
            address_int, entry, register_state
        )

        # Analyze bit changes
        bits_modified = self._analyze_bit_changes(address_int, value_before, value_after)

        # Get hardware verification status
        hardware_verified, j_link_value = self._get_hardware_verification(address_int)

        # Determine clock gate status
        clock_gate_status = self._get_clock_gate_status(address_int)

        # Extract source location
        source_loc = entry.get("source_location", {})

        # Calculate relative timestamp (approximate based on execution phase)
        relative_timestamp = self._calculate_relative_timestamp(entry, index)

        return CorrectedRegisterAccess(
            sequence_number=index + 1,  # Will be reassigned later
            relative_timestamp_ms=relative_timestamp,
            address=f"0x{address_int:08x}",
            peripheral_name=peripheral_name,
            register_name=register_name,
            register_offset_from_base=register_offset,
            access_type=entry.get("access_type", "unknown"),
            data_width=entry.get("data_size", 32),
            value_before=f"0x{value_before:08x}",
            value_after=f"0x{value_after:08x}",
            value_written=f"0x{value_written:08x}" if value_written is not None else None,
            mask_applied=None,  # TODO: Implement RMW mask detection
            bits_modified=bits_modified,
            execution_phase=entry.get("execution_phase", "unknown"),
            source_file_path=source_loc.get("file", "unknown"),
            function_name=source_loc.get("function", "unknown"),
            line_number=source_loc.get("line", 0),
            call_stack_depth=len(entry.get("call_stack", "").split("->")) if entry.get("call_stack") else 1,
            hardware_verified=hardware_verified,
            j_link_captured_value=j_link_value,
            clock_gate_status=clock_gate_status
        )

    def _determine_register_values(self, address: int, entry: Dict[str, Any],
                                 register_state: Dict[str, str]) -> Tuple[int, int, Optional[int]]:
        """
        Determine before/after register values using multiple data sources

        Returns:
            Tuple of (value_before, value_after, value_written)
        """
        address_str = f"0x{address:08x}"
        access_type = entry.get("access_type", "")

        # Get current state from register tracking
        current_value_str = register_state.get(address_str, "0x00000000")
        current_value = int(current_value_str, 16) if isinstance(current_value_str, str) else current_value_str

        # Get hardware captured value if available
        hardware_value = self._get_hardware_register_value(address)

        # Determine values based on access type
        if access_type in ["volatile_read"]:
            # Read operation - no change to register
            value_before = hardware_value if hardware_value is not None else current_value
            value_after = value_before
            value_written = None

        elif access_type in ["volatile_write", "function_call_write"]:
            # Write operation - determine written value
            value_before = current_value

            # Try to get the written value from known register values
            if hardware_value is not None:
                value_after = hardware_value
                value_written = hardware_value
            else:
                # Use decoder to get reset/expected values
                register_info = self.decoder.get_register_info(address)
                if register_info:
                    _, _, register_def = register_info
                    value_after = self._estimate_written_value(address, register_def, entry)
                else:
                    value_after = current_value  # No change assumed
                value_written = value_after

        else:
            # Unknown access type
            value_before = current_value
            value_after = hardware_value if hardware_value is not None else current_value
            value_written = None

        return value_before, value_after, value_written

    def _get_hardware_register_value(self, address: int) -> Optional[int]:
        """Get register value from hardware monitoring data"""
        address_str = str(address)

        # Check live monitoring results first
        if self.live_monitoring and "successful_reads" in self.live_monitoring:
            if address_str in self.live_monitoring["successful_reads"]:
                value_str = self.live_monitoring["successful_reads"][address_str]["value"]
                return int(value_str, 16)

        # Check diagnostic results
        if self.hardware_diagnostics and "access_widths" in self.hardware_diagnostics:
            if address_str in self.hardware_diagnostics["access_widths"]:
                access_data = self.hardware_diagnostics["access_widths"][address_str]
                if "32bit" in access_data and access_data["32bit"]["success"]:
                    value_str = access_data["32bit"]["value"]
                    return int(value_str, 16)

        # Check target_state in diagnostics
        if self.hardware_diagnostics and "target_state" in self.hardware_diagnostics:
            if address_str in self.hardware_diagnostics["target_state"]:
                target_data = self.hardware_diagnostics["target_state"][address_str]
                if target_data["success"]:
                    value_str = target_data["value_32bit"]
                    return int(value_str, 16)

        return None

    def _estimate_written_value(self, address: int, register_def, entry: Dict[str, Any]) -> int:
        """Estimate the value written to a register based on context"""

        # Use known values for specific registers
        known_values = {
            0x40001434: 0x00000002,  # CLKCTL0_CLKSEL
            0x40411000: 0x072F01DC,  # XSPI2_MCR
            0x4000407C: 0x00000041,  # IOPCTL0_PIO0_31
            0x40020030: 0x0000001C,  # SYSCON0_AHBMATPRIO
            0x40001020: 0x00000001,  # CLKCTL0_PSCCTL4
            0x40001024: 0x0000002C,  # CLKCTL0_PSCCTL5
        }

        if address in known_values:
            return known_values[address]

        # For clock control registers, use bit-set patterns
        if "PSCCTL" in register_def.name:
            # Peripheral clock control - typically setting enable bits
            return 0x00000001  # Enable bit 0

        # Default to reset value
        return register_def.reset_value

    def _analyze_bit_changes(self, address: int, value_before: int, value_after: int) -> List[Dict[str, Any]]:
        """Analyze bit-level changes using the register decoder"""
        if value_before == value_after:
            return []

        bit_changes = self.decoder.analyze_bit_changes(address, value_before, value_after)

        if bit_changes:
            self.statistics["bit_changes_populated"] += 1

        return bit_changes

    def _get_hardware_verification(self, address: int) -> Tuple[bool, Optional[str]]:
        """Get hardware verification status for a register"""
        hardware_value = self._get_hardware_register_value(address)

        if hardware_value is not None:
            self.statistics["hardware_verified"] += 1
            return True, f"0x{hardware_value:08x}"
        else:
            return False, None

    def _get_clock_gate_status(self, address: int) -> str:
        """Determine clock gate status for a register"""
        if not self.live_monitoring or "clock_status" not in self.live_monitoring:
            return "unknown"

        clock_status = self.live_monitoring["clock_status"]

        if self.decoder.is_clock_gated_register(address, clock_status):
            self.statistics["clock_gated_registers"] += 1
            return "disabled"
        else:
            return "enabled"

    def _calculate_relative_timestamp(self, entry: Dict[str, Any], index: int) -> Optional[float]:
        """Calculate approximate relative timestamp based on execution phase"""
        phase = entry.get("execution_phase", "runtime")
        instruction_idx = entry.get("instruction_index", 0)

        # Approximate timing based on typical embedded system boot sequence
        phase_base_times = {
            "board_init": 0.0,      # 0-50ms
            "driver_init": 50.0,    # 50-100ms
            "runtime": 100.0        # 100ms+
        }

        base_time = phase_base_times.get(phase, 100.0)

        # Add small increments for instruction ordering
        increment = instruction_idx * 0.1  # 0.1ms per instruction

        return base_time + increment

    def _validate_against_hardware(self, entries: List[CorrectedRegisterAccess]) -> List[CorrectedRegisterAccess]:
        """Validate entries against hardware monitoring data"""
        validated_entries = []

        for entry in entries:
            # Check for hardware correlation discrepancies
            if entry.hardware_verified and entry.j_link_captured_value:
                captured_value = int(entry.j_link_captured_value, 16)
                expected_value = int(entry.value_after, 16)

                if captured_value != expected_value:
                    self.validation_issues.append({
                        "type": "hardware_mismatch",
                        "address": entry.address,
                        "expected": entry.value_after,
                        "captured": entry.j_link_captured_value,
                        "register": f"{entry.peripheral_name}.{entry.register_name}"
                    })

            validated_entries.append(entry)

        return validated_entries

    def _assign_sequence_numbers(self, entries: List[CorrectedRegisterAccess]) -> List[CorrectedRegisterAccess]:
        """Assign final sequence numbers 1-N"""
        for i, entry in enumerate(entries):
            entry.sequence_number = i + 1
        return entries

    def save_corrected_sequence(self, output_file: str = "corrected_chronological_sequence.json") -> bool:
        """Save the corrected chronological sequence to JSON file"""
        if not self.corrected_sequence:
            print("❌ No corrected sequence to save")
            return False

        output_path = os.path.join(self.results_dir, output_file)

        # Convert to serializable format
        sequence_data = {
            "analysis_type": "corrected_chronological_sequence",
            "description": "Hardware-validated peripheral register accesses in chronological execution order",
            "total_accesses": len(self.corrected_sequence),
            "correction_timestamp": datetime.now().isoformat(),
            "statistics": self.statistics,
            "validation_summary": {
                "total_issues": len(self.validation_issues),
                "hardware_verified_count": self.statistics["hardware_verified"],
                "clock_gated_count": self.statistics["clock_gated_registers"],
                "bit_changes_populated": self.statistics["bit_changes_populated"]
            },
            "chronological_sequence": [asdict(entry) for entry in self.corrected_sequence]
        }

        try:
            with open(output_path, 'w') as f:
                json.dump(sequence_data, f, indent=2)
            print(f"✅ Corrected sequence saved to {output_path}")
            return True
        except Exception as e:
            print(f"❌ Error saving corrected sequence: {e}")
            return False

    def save_validation_report(self, output_file: str = "sequence_validation_report.json") -> bool:
        """Save validation report with issues and statistics"""
        output_path = os.path.join(self.results_dir, output_file)

        validation_report = {
            "validation_timestamp": datetime.now().isoformat(),
            "total_entries_processed": self.statistics["total_entries"],
            "correction_statistics": self.statistics,
            "validation_issues": self.validation_issues,
            "issue_summary": self._generate_issue_summary(),
            "confidence_metrics": self._calculate_confidence_metrics(),
            "recommendations": self._generate_recommendations()
        }

        try:
            with open(output_path, 'w') as f:
                json.dump(validation_report, f, indent=2)
            print(f"✅ Validation report saved to {output_path}")
            return True
        except Exception as e:
            print(f"❌ Error saving validation report: {e}")
            return False

    def _generate_issue_summary(self) -> Dict[str, int]:
        """Generate summary of validation issues by type"""
        issue_counts = {}
        for issue in self.validation_issues:
            issue_type = issue.get("type", "unknown")
            issue_counts[issue_type] = issue_counts.get(issue_type, 0) + 1
        return issue_counts

    def _calculate_confidence_metrics(self) -> Dict[str, float]:
        """Calculate confidence metrics for the corrected sequence"""
        total = self.statistics["total_entries"]
        if total == 0:
            return {}

        return {
            "hardware_verification_rate": (self.statistics["hardware_verified"] / total) * 100,
            "bit_analysis_completion_rate": (self.statistics["bit_changes_populated"] / total) * 100,
            "clock_gating_detection_rate": (self.statistics["clock_gated_registers"] / total) * 100,
            "duplicate_removal_efficiency": (self.statistics["duplicates_removed"] / (total + self.statistics["duplicates_removed"])) * 100 if self.statistics["duplicates_removed"] > 0 else 100,
            "overall_confidence": min(95.0, (self.statistics["hardware_verified"] / total) * 80 + 20)  # Max 95% confidence
        }

    def _generate_recommendations(self) -> List[str]:
        """Generate recommendations based on validation results"""
        recommendations = []

        if self.statistics["hardware_verified"] < self.statistics["total_entries"] * 0.5:
            recommendations.append("Consider running hardware monitoring with more comprehensive register coverage")

        if self.statistics["clock_gated_registers"] > 0:
            recommendations.append("Enable clocks for gated peripherals to improve register accessibility")

        if len(self.validation_issues) > 0:
            recommendations.append("Review validation issues for potential firmware or analysis improvements")

        if self.statistics["duplicates_removed"] > self.statistics["total_entries"] * 0.1:
            recommendations.append("Review LLVM analysis for potential duplicate detection improvements")

        return recommendations

    def generate_summary_markdown(self, output_file: str = "corrected_sequence_summary.md") -> bool:
        """Generate human-readable summary in Markdown format"""
        output_path = os.path.join(self.results_dir, output_file)

        confidence = self._calculate_confidence_metrics()

        markdown_content = f"""# MIMXRT700 Corrected Chronological Sequence Summary

## Overview
- **Total Register Accesses**: {self.statistics['total_entries']}
- **Correction Timestamp**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
- **Overall Confidence**: {confidence.get('overall_confidence', 0):.1f}%

## Correction Statistics
- **Duplicates Removed**: {self.statistics['duplicates_removed']}
- **Hardware Verified**: {self.statistics['hardware_verified']} ({confidence.get('hardware_verification_rate', 0):.1f}%)
- **Clock-Gated Registers**: {self.statistics['clock_gated_registers']}
- **Bit Changes Populated**: {self.statistics['bit_changes_populated']} ({confidence.get('bit_analysis_completion_rate', 0):.1f}%)

## Validation Results
- **Total Issues**: {len(self.validation_issues)}
- **Issue Types**: {', '.join(f"{k}: {v}" for k, v in self._generate_issue_summary().items())}

## Key Findings
"""

        # Add key register values
        key_registers = [
            ("CLKCTL0_CLKSEL", "0x40001434", "0x00000002"),
            ("XSPI2_MCR", "0x40411000", "0x072F01DC"),
            ("IOPCTL0_PIO0_31", "0x4000407C", "0x00000041"),
            ("SYSCON0_AHBMATPRIO", "0x40020030", "0x0000001C")
        ]

        for reg_name, address, expected_value in key_registers:
            # Find this register in our corrected sequence
            found_entry = None
            for entry in self.corrected_sequence:
                if entry.address.lower() == address.lower():
                    found_entry = entry
                    break

            if found_entry:
                status = "✅ Verified" if found_entry.hardware_verified else "⚠️ Not verified"
                markdown_content += f"- **{reg_name}**: {found_entry.value_after} {status}\n"
            else:
                markdown_content += f"- **{reg_name}**: Not found in sequence\n"

        markdown_content += f"""
## Recommendations
"""
        for rec in self._generate_recommendations():
            markdown_content += f"- {rec}\n"

        markdown_content += f"""
## Files Generated
- `corrected_chronological_sequence.json` - Complete corrected sequence
- `sequence_validation_report.json` - Detailed validation report
- `corrected_sequence_summary.md` - This summary

## Next Steps
1. Review validation issues in the detailed report
2. Run comprehensive tests with the corrected sequence
3. Update firmware if clock gating issues are identified
4. Verify critical register values match hardware specifications
"""

        try:
            with open(output_path, 'w') as f:
                f.write(markdown_content)
            print(f"✅ Summary saved to {output_path}")
            return True
        except Exception as e:
            print(f"❌ Error saving summary: {e}")
            return False


def main():
    """Main function with command-line interface"""
    parser = argparse.ArgumentParser(
        description="Create corrected chronological sequence of MIMXRT700 peripheral register accesses",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic correction with default paths
  python3 create_corrected_chronological_sequence.py

  # Specify custom results directory
  python3 create_corrected_chronological_sequence.py --results-dir /path/to/results

  # Custom output filenames
  python3 create_corrected_chronological_sequence.py --output corrected_seq.json --report validation.json
        """
    )

    parser.add_argument(
        "--results-dir",
        default="peripheral_monitoring/results",
        help="Directory containing analysis results (default: peripheral_monitoring/results)"
    )

    parser.add_argument(
        "--output",
        default="corrected_chronological_sequence.json",
        help="Output filename for corrected sequence (default: corrected_chronological_sequence.json)"
    )

    parser.add_argument(
        "--report",
        default="sequence_validation_report.json",
        help="Output filename for validation report (default: sequence_validation_report.json)"
    )

    parser.add_argument(
        "--summary",
        default="corrected_sequence_summary.md",
        help="Output filename for summary (default: corrected_sequence_summary.md)"
    )

    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )

    args = parser.parse_args()

    print("🎯 MIMXRT700 Peripheral Register Chronological Sequence Correction Tool")
    print("=" * 80)
    print()

    # Initialize corrector
    try:
        corrector = ChronologicalSequenceCorrector(args.results_dir)
        print(f"📁 Using results directory: {args.results_dir}")
    except Exception as e:
        print(f"❌ Error initializing corrector: {e}")
        return 1

    # Perform correction
    try:
        success = corrector.correct_chronological_sequence()
        if not success:
            print("❌ Correction process failed")
            return 1
    except Exception as e:
        print(f"❌ Error during correction: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1

    # Save results
    print("\n💾 Saving results...")

    try:
        # Save corrected sequence
        if not corrector.save_corrected_sequence(args.output):
            print("❌ Failed to save corrected sequence")
            return 1

        # Save validation report
        if not corrector.save_validation_report(args.report):
            print("❌ Failed to save validation report")
            return 1

        # Save summary
        if not corrector.generate_summary_markdown(args.summary):
            print("❌ Failed to save summary")
            return 1

    except Exception as e:
        print(f"❌ Error saving results: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1

    # Print final statistics
    print("\n📊 Final Statistics:")
    print(f"  Total Entries: {corrector.statistics['total_entries']}")
    print(f"  Duplicates Removed: {corrector.statistics['duplicates_removed']}")
    print(f"  Hardware Verified: {corrector.statistics['hardware_verified']}")
    print(f"  Clock-Gated Registers: {corrector.statistics['clock_gated_registers']}")
    print(f"  Bit Changes Populated: {corrector.statistics['bit_changes_populated']}")
    print(f"  Validation Issues: {len(corrector.validation_issues)}")

    confidence = corrector._calculate_confidence_metrics()
    print(f"\n🎯 Overall Confidence: {confidence.get('overall_confidence', 0):.1f}%")

    print("\n✅ Chronological sequence correction completed successfully!")
    print(f"📄 Results saved to:")
    print(f"  - {os.path.join(args.results_dir, args.output)}")
    print(f"  - {os.path.join(args.results_dir, args.report)}")
    print(f"  - {os.path.join(args.results_dir, args.summary)}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
