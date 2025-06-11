#!/usr/bin/env python3
"""
MIMXRT700 Chronological Sequence Validation Test Suite

Comprehensive validation framework for the corrected chronological sequence,
including sequence integrity checks, hardware correlation validation, and
execution phase logic verification.

Author: Augment Agent
Date: 2024
"""

import json
import os
import sys
from typing import Dict, List, Any, Optional, Tuple
import unittest
from dataclasses import dataclass

# Add the tools directory to the path
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from mimxrt798s_register_decoder import MIMXRT798SRegisterDecoder

@dataclass
class ValidationResult:
    """Result of a validation test"""
    test_name: str
    passed: bool
    message: str
    details: Optional[Dict[str, Any]] = None

class ChronologicalSequenceValidator:
    """
    Comprehensive validation suite for chronological sequence
    """
    
    def __init__(self, results_dir: str = "peripheral_monitoring/results"):
        """Initialize validator with data sources"""
        self.results_dir = results_dir
        self.decoder = MIMXRT798SRegisterDecoder()
        
        # Load corrected sequence and validation data
        self.corrected_sequence = self._load_corrected_sequence()
        self.hardware_diagnostics = self._load_json("register_access_diagnostic_results.json")
        self.live_monitoring = self._load_json("corrected_monitoring_results.json")
        
        self.validation_results: List[ValidationResult] = []
        
    def _load_corrected_sequence(self) -> List[Dict[str, Any]]:
        """Load the corrected chronological sequence"""
        filepath = os.path.join(self.results_dir, "corrected_chronological_sequence.json")
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
                return data.get("chronological_sequence", [])
        except FileNotFoundError:
            print(f"Warning: corrected_chronological_sequence.json not found")
            return []
        except json.JSONDecodeError as e:
            print(f"Error loading corrected sequence: {e}")
            return []
    
    def _load_json(self, filename: str) -> Dict[str, Any]:
        """Load JSON file from results directory"""
        filepath = os.path.join(self.results_dir, filename)
        try:
            with open(filepath, 'r') as f:
                return json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            return {}
    
    def run_all_validations(self) -> bool:
        """
        Run all validation tests
        
        Returns:
            True if all tests pass
        """
        print("🧪 Running chronological sequence validation tests...")
        print("=" * 60)
        
        # Test 1: Sequence integrity
        self._test_sequence_integrity()
        
        # Test 2: Hardware correlation
        self._test_hardware_correlation()
        
        # Test 3: Execution phase logic
        self._test_execution_phase_logic()
        
        # Test 4: Register accessibility
        self._test_register_accessibility()
        
        # Test 5: Bit change consistency
        self._test_bit_change_consistency()
        
        # Test 6: Known value validation
        self._test_known_values()
        
        # Print results
        self._print_validation_results()
        
        # Return overall success
        return all(result.passed for result in self.validation_results)
    
    def _test_sequence_integrity(self):
        """Test 1: Sequence integrity - no gaps, no duplicates"""
        print("🔍 Testing sequence integrity...")
        
        if not self.corrected_sequence:
            self.validation_results.append(ValidationResult(
                "sequence_integrity", False, "No corrected sequence found"
            ))
            return
        
        # Check sequence numbers
        sequence_numbers = [entry.get("sequence_number", 0) for entry in self.corrected_sequence]
        expected_numbers = list(range(1, len(self.corrected_sequence) + 1))
        
        if sequence_numbers != expected_numbers:
            missing = set(expected_numbers) - set(sequence_numbers)
            duplicates = [x for x in sequence_numbers if sequence_numbers.count(x) > 1]
            
            self.validation_results.append(ValidationResult(
                "sequence_integrity", False,
                f"Sequence number issues: missing={missing}, duplicates={duplicates}",
                {"missing": list(missing), "duplicates": duplicates}
            ))
        else:
            self.validation_results.append(ValidationResult(
                "sequence_integrity", True,
                f"Sequence integrity verified: {len(self.corrected_sequence)} entries, no gaps or duplicates"
            ))
    
    def _test_hardware_correlation(self):
        """Test 2: Hardware correlation validation"""
        print("🔍 Testing hardware correlation...")
        
        mismatches = []
        verified_count = 0
        
        for entry in self.corrected_sequence:
            if entry.get("hardware_verified", False):
                verified_count += 1
                
                # Check if captured value matches expected value
                j_link_value = entry.get("j_link_captured_value")
                expected_value = entry.get("value_after")
                
                if j_link_value and expected_value:
                    if j_link_value.lower() != expected_value.lower():
                        mismatches.append({
                            "address": entry.get("address"),
                            "register": f"{entry.get('peripheral_name')}.{entry.get('register_name')}",
                            "expected": expected_value,
                            "captured": j_link_value
                        })
        
        if mismatches:
            self.validation_results.append(ValidationResult(
                "hardware_correlation", False,
                f"Hardware correlation mismatches found: {len(mismatches)} out of {verified_count} verified",
                {"mismatches": mismatches}
            ))
        else:
            self.validation_results.append(ValidationResult(
                "hardware_correlation", True,
                f"Hardware correlation verified: {verified_count} registers match captured values"
            ))
    
    def _test_execution_phase_logic(self):
        """Test 3: Execution phase logic"""
        print("🔍 Testing execution phase logic...")
        
        phase_order = {"board_init": 0, "driver_init": 1, "runtime": 2}
        phase_violations = []
        
        last_phase_num = -1
        for i, entry in enumerate(self.corrected_sequence):
            phase = entry.get("execution_phase", "runtime")
            phase_num = phase_order.get(phase, 3)
            
            if phase_num < last_phase_num:
                phase_violations.append({
                    "sequence_number": entry.get("sequence_number"),
                    "current_phase": phase,
                    "previous_phase": list(phase_order.keys())[last_phase_num] if last_phase_num < 3 else "unknown"
                })
            
            last_phase_num = max(last_phase_num, phase_num)
        
        if phase_violations:
            self.validation_results.append(ValidationResult(
                "execution_phase_logic", False,
                f"Execution phase order violations: {len(phase_violations)}",
                {"violations": phase_violations}
            ))
        else:
            self.validation_results.append(ValidationResult(
                "execution_phase_logic", True,
                "Execution phase ordering is correct (board_init → driver_init → runtime)"
            ))
    
    def _test_register_accessibility(self):
        """Test 4: Register accessibility based on clock gating"""
        print("🔍 Testing register accessibility...")
        
        clock_gated_accessed = []
        
        for entry in self.corrected_sequence:
            clock_status = entry.get("clock_gate_status", "unknown")
            access_type = entry.get("access_type", "")
            
            # Check if we're trying to access a clock-gated register
            if clock_status == "disabled" and access_type in ["volatile_read", "volatile_write"]:
                clock_gated_accessed.append({
                    "address": entry.get("address"),
                    "register": f"{entry.get('peripheral_name')}.{entry.get('register_name')}",
                    "access_type": access_type,
                    "sequence_number": entry.get("sequence_number")
                })
        
        if clock_gated_accessed:
            self.validation_results.append(ValidationResult(
                "register_accessibility", False,
                f"Clock-gated register accesses detected: {len(clock_gated_accessed)}",
                {"clock_gated_accesses": clock_gated_accessed}
            ))
        else:
            self.validation_results.append(ValidationResult(
                "register_accessibility", True,
                "All register accesses respect clock gating constraints"
            ))
    
    def _test_bit_change_consistency(self):
        """Test 5: Bit change consistency"""
        print("🔍 Testing bit change consistency...")
        
        inconsistent_changes = []
        
        for entry in self.corrected_sequence:
            value_before_str = entry.get("value_before", "0x00000000")
            value_after_str = entry.get("value_after", "0x00000000")
            bits_modified = entry.get("bits_modified", [])
            
            try:
                value_before = int(value_before_str, 16)
                value_after = int(value_after_str, 16)
                
                # Calculate actual XOR
                actual_xor = value_before ^ value_after
                
                # If no change, should have no bits_modified
                if actual_xor == 0 and bits_modified:
                    inconsistent_changes.append({
                        "address": entry.get("address"),
                        "issue": "No value change but bits_modified not empty",
                        "bits_modified_count": len(bits_modified)
                    })
                elif actual_xor != 0 and not bits_modified:
                    inconsistent_changes.append({
                        "address": entry.get("address"),
                        "issue": "Value changed but bits_modified is empty",
                        "xor_value": f"0x{actual_xor:08x}"
                    })
                    
            except ValueError:
                inconsistent_changes.append({
                    "address": entry.get("address"),
                    "issue": "Invalid hex values",
                    "value_before": value_before_str,
                    "value_after": value_after_str
                })
        
        if inconsistent_changes:
            self.validation_results.append(ValidationResult(
                "bit_change_consistency", False,
                f"Bit change inconsistencies found: {len(inconsistent_changes)}",
                {"inconsistencies": inconsistent_changes}
            ))
        else:
            self.validation_results.append(ValidationResult(
                "bit_change_consistency", True,
                "Bit change analysis is consistent with value changes"
            ))
    
    def _test_known_values(self):
        """Test 6: Known value validation"""
        print("🔍 Testing known register values...")
        
        # Known register values from hardware specifications
        known_values = {
            "0x40001434": "0x00000002",  # CLKCTL0_CLKSEL
            "0x40411000": "0x072f01dc",  # XSPI2_MCR (case insensitive)
            "0x4000407c": "0x00000041",  # IOPCTL0_PIO0_31
            "0x40020030": "0x0000001c",  # SYSCON0_AHBMATPRIO
        }
        
        value_mismatches = []
        verified_known_values = 0
        
        for entry in self.corrected_sequence:
            address = entry.get("address", "").lower()
            value_after = entry.get("value_after", "").lower()
            
            if address in known_values:
                expected = known_values[address].lower()
                if value_after != expected:
                    value_mismatches.append({
                        "address": address,
                        "register": f"{entry.get('peripheral_name')}.{entry.get('register_name')}",
                        "expected": expected,
                        "actual": value_after
                    })
                else:
                    verified_known_values += 1
        
        if value_mismatches:
            self.validation_results.append(ValidationResult(
                "known_values", False,
                f"Known value mismatches: {len(value_mismatches)} out of {len(known_values)}",
                {"mismatches": value_mismatches}
            ))
        else:
            self.validation_results.append(ValidationResult(
                "known_values", True,
                f"All known register values verified: {verified_known_values}/{len(known_values)}"
            ))
    
    def _print_validation_results(self):
        """Print validation results summary"""
        print("\n📋 Validation Results Summary:")
        print("=" * 60)
        
        passed_count = sum(1 for result in self.validation_results if result.passed)
        total_count = len(self.validation_results)
        
        for result in self.validation_results:
            status = "✅ PASS" if result.passed else "❌ FAIL"
            print(f"{status} {result.test_name}: {result.message}")
            
            if not result.passed and result.details:
                # Print first few details for failed tests
                if isinstance(result.details, dict):
                    for key, value in result.details.items():
                        if isinstance(value, list) and len(value) > 3:
                            print(f"      {key}: {value[:3]}... ({len(value)} total)")
                        else:
                            print(f"      {key}: {value}")
        
        print(f"\n🎯 Overall Result: {passed_count}/{total_count} tests passed")
        
        if passed_count == total_count:
            print("🎉 All validation tests PASSED!")
        else:
            print("⚠️  Some validation tests FAILED - review details above")
    
    def save_validation_results(self, output_file: str = "validation_test_results.json") -> bool:
        """Save detailed validation results to JSON file"""
        output_path = os.path.join(self.results_dir, output_file)
        
        results_data = {
            "validation_timestamp": "2024-01-01T00:00:00",  # Will be updated when run
            "total_tests": len(self.validation_results),
            "passed_tests": sum(1 for r in self.validation_results if r.passed),
            "failed_tests": sum(1 for r in self.validation_results if not r.passed),
            "test_results": [
                {
                    "test_name": result.test_name,
                    "passed": result.passed,
                    "message": result.message,
                    "details": result.details
                }
                for result in self.validation_results
            ]
        }
        
        try:
            with open(output_path, 'w') as f:
                json.dump(results_data, f, indent=2)
            print(f"✅ Validation results saved to {output_path}")
            return True
        except Exception as e:
            print(f"❌ Error saving validation results: {e}")
            return False


def test_first_register_access_accuracy():
    """Test that the first register access matches the actual hardware boot sequence."""
    print("\n🔍 Testing first register access accuracy...")

    # Load the corrected chronological sequence
    with open('peripheral_monitoring/results/corrected_chronological_sequence.json', 'r') as f:
        data = json.load(f)

    first_entry = data['chronological_sequence'][0]

    # Expected first register access based on BOARD_InitHardware() sequence:
    # 1. BOARD_ConfigMPU() -> ARM_MPU_Enable() -> MPU_CTRL register
    expected_first_access = {
        'address': '0xe000ed94',  # MPU_CTRL register
        'peripheral_name': 'MPU',
        'register_name': 'CTRL',
        'function_name': 'BOARD_ConfigMPU',
        'execution_phase': 'board_init'
    }

    print(f"📋 First register access analysis:")
    print(f"   Address: {first_entry['address']}")
    print(f"   Peripheral: {first_entry['peripheral_name']}")
    print(f"   Register: {first_entry['register_name']}")
    print(f"   Function: {first_entry['function_name']}")
    print(f"   Value Before: {first_entry['value_before']}")
    print(f"   Value After: {first_entry['value_after']}")
    print(f"   Value Written: {first_entry['value_written']}")
    print(f"   Source: {first_entry['source_file_path']}:{first_entry['line_number']}")

    # Verify the first access matches expectations
    try:
        assert first_entry['address'].lower() == expected_first_access['address'].lower(), \
            f"First register address mismatch: expected {expected_first_access['address']}, got {first_entry['address']}"

        assert first_entry['peripheral_name'] == expected_first_access['peripheral_name'], \
            f"First peripheral mismatch: expected {expected_first_access['peripheral_name']}, got {first_entry['peripheral_name']}"

        assert first_entry['register_name'] == expected_first_access['register_name'], \
            f"First register name mismatch: expected {expected_first_access['register_name']}, got {first_entry['register_name']}"

        assert first_entry['function_name'] == expected_first_access['function_name'], \
            f"First function mismatch: expected {expected_first_access['function_name']}, got {first_entry['function_name']}"

        assert first_entry['execution_phase'] == expected_first_access['execution_phase'], \
            f"First execution phase mismatch: expected {expected_first_access['execution_phase']}, got {first_entry['execution_phase']}"

        # Additional validation: Check if the MPU_CTRL write is meaningful
        # ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk) should write 0x5, not 0x0
        if first_entry['value_written'] == '0x00000000':
            print("⚠️  WARNING: MPU_CTRL write value is 0x00000000, which suggests MPU is being disabled")
            print("    Expected: 0x00000005 (PRIVDEFENA_Msk | HFNMIENA_Msk)")
            print("    This may indicate a sequencing error or incorrect value capture")
            print("    Checking if this is actually ARM_MPU_Disable() instead of ARM_MPU_Enable()")
            return False

        print("✅ First register access verification PASSED")
        return True

    except AssertionError as e:
        print(f"❌ First register access verification FAILED: {e}")
        return False


def main():
    """Main function for running validation tests"""
    import argparse

    parser = argparse.ArgumentParser(description="Validate corrected chronological sequence")
    parser.add_argument("--results-dir", default="peripheral_monitoring/results",
                       help="Directory containing results files")
    parser.add_argument("--save-results", action="store_true",
                       help="Save detailed validation results to JSON")
    parser.add_argument("--test-first-access", action="store_true",
                       help="Run additional test for first register access accuracy")

    args = parser.parse_args()

    print("🧪 MIMXRT700 Chronological Sequence Validation Test Suite")
    print("=" * 80)
    print()

    # Initialize validator
    validator = ChronologicalSequenceValidator(args.results_dir)

    # Run all validations
    success = validator.run_all_validations()

    # Run additional first access test if requested
    if args.test_first_access:
        first_access_success = test_first_register_access_accuracy()
        success = success and first_access_success

    # Save results if requested
    if args.save_results:
        validator.save_validation_results()

    # Exit with appropriate code
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
