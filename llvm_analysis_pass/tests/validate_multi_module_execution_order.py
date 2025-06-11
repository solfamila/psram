#!/usr/bin/env python3
"""
Multi-Module Execution Order Validation Test
Tests that the LLVM analysis pass correctly handles multi-module analysis
and produces the expected execution order.
"""

import json
import os
import sys

class MultiModuleExecutionOrderValidator:
    """
    Validates multi-module analysis execution order
    """
    
    def __init__(self):
        self.single_module_file = "single_module_test.json"
        self.multi_module_file = "multi_module_test.json"
        self.validation_results = {}
        
    def load_analysis_results(self):
        """Load both single and multi-module analysis results"""
        try:
            with open(self.single_module_file, 'r') as f:
                self.single_module_data = json.load(f)
            
            with open(self.multi_module_file, 'r') as f:
                self.multi_module_data = json.load(f)
                
            return True
            
        except Exception as e:
            print(f"❌ Failed to load analysis results: {e}")
            return False
    
    def count_accesses_by_peripheral(self, data):
        """Count register accesses by peripheral"""
        counts = {}
        for peripheral in data['peripheral_accesses']:
            peripheral_name = peripheral['peripheral_name']
            access_count = len(peripheral['accesses'])
            counts[peripheral_name] = access_count
        return counts
    
    def test_multi_module_coverage(self):
        """Test that multi-module analysis finds more accesses than single module"""
        print("🔍 TEST 1: Multi-Module Coverage")
        
        single_counts = self.count_accesses_by_peripheral(self.single_module_data)
        multi_counts = self.count_accesses_by_peripheral(self.multi_module_data)
        
        single_total = sum(single_counts.values())
        multi_total = sum(multi_counts.values())
        
        print(f"   Single module total accesses: {single_total}")
        print(f"   Multi-module total accesses: {multi_total}")
        
        if multi_total > single_total:
            print(f"   ✅ PASS: Multi-module found {multi_total - single_total} additional accesses")
            self.validation_results['coverage'] = True
        else:
            print(f"   ❌ FAIL: Multi-module should find more accesses than single module")
            self.validation_results['coverage'] = False
        
        # Check specific peripherals
        print(f"\n   📊 Peripheral breakdown:")
        all_peripherals = set(single_counts.keys()) | set(multi_counts.keys())
        for peripheral in sorted(all_peripherals):
            single_count = single_counts.get(peripheral, 0)
            multi_count = multi_counts.get(peripheral, 0)
            status = "✅" if multi_count >= single_count else "❌"
            print(f"      {status} {peripheral:8s}: {single_count:2d} → {multi_count:2d}")
    
    def test_critical_mpu_sequence(self):
        """Test that critical BOARD_ConfigMPU sequence is detected"""
        print(f"\n🔍 TEST 2: Critical BOARD_ConfigMPU Sequence Detection")
        
        # Extract all MPU accesses from multi-module analysis
        mpu_accesses = []
        for peripheral in self.multi_module_data['peripheral_accesses']:
            if peripheral['peripheral_name'] == 'MPU':
                for access in peripheral['accesses']:
                    mpu_accesses.append(access)
        
        print(f"   Found {len(mpu_accesses)} MPU register accesses")
        
        # Expected critical functions from C source code
        expected_functions = [
            'ARM_MPU_Disable',      # Line 228
            'ARM_MPU_SetMemAttr',   # Lines 231, 233, 236, 239 (4 calls)
            'ARM_MPU_SetRegion',    # Lines 242, 245, 253 (3 calls)
            'ARM_MPU_Enable'        # Line 262
        ]
        
        found_functions = set()
        for access in mpu_accesses:
            purpose = access.get('purpose', '').lower()
            if 'mpu disable' in purpose:
                found_functions.add('ARM_MPU_Disable')
            elif 'mpu memory attribute' in purpose:
                found_functions.add('ARM_MPU_SetMemAttr')
            elif 'mpu region' in purpose:
                found_functions.add('ARM_MPU_SetRegion')
            elif 'mpu enable' in purpose:
                found_functions.add('ARM_MPU_Enable')
        
        print(f"   📋 Critical MPU function detection:")
        all_found = True
        for func in expected_functions:
            if func in found_functions:
                print(f"      ✅ {func}")
            else:
                print(f"      ❌ {func} - NOT FOUND")
                all_found = False
        
        if all_found:
            print(f"   ✅ PASS: All critical MPU functions detected")
            self.validation_results['mpu_sequence'] = True
        else:
            print(f"   ❌ FAIL: Missing critical MPU functions")
            self.validation_results['mpu_sequence'] = False
    
    def test_cache_sequence(self):
        """Test that XCACHE sequence is detected"""
        print(f"\n🔍 TEST 3: XCACHE Sequence Detection")
        
        # Extract all XCACHE accesses
        cache_accesses = []
        for peripheral in self.multi_module_data['peripheral_accesses']:
            if peripheral['peripheral_name'] == 'XCACHE0':
                for access in peripheral['accesses']:
                    cache_accesses.append(access)
        
        print(f"   Found {len(cache_accesses)} XCACHE register accesses")
        
        # Check for cache disable/enable sequence
        cache_operations = []
        for access in cache_accesses:
            purpose = access.get('purpose', '').lower()
            if 'cache disable' in purpose:
                cache_operations.append('disable')
            elif 'cache enable' in purpose:
                cache_operations.append('enable')
        
        print(f"   📋 Cache operations: {cache_operations}")
        
        # Expected: disable operations before enable operations
        disable_count = cache_operations.count('disable')
        enable_count = cache_operations.count('enable')
        
        if disable_count >= 2 and enable_count >= 2:
            print(f"   ✅ PASS: Found {disable_count} disable and {enable_count} enable operations")
            self.validation_results['cache_sequence'] = True
        else:
            print(f"   ❌ FAIL: Expected at least 2 disable and 2 enable operations")
            self.validation_results['cache_sequence'] = False
    
    def test_execution_order_logic(self):
        """Test the logical execution order"""
        print(f"\n🔍 TEST 4: Execution Order Logic")
        
        # Get all accesses in order
        all_accesses = []
        for peripheral in self.multi_module_data['peripheral_accesses']:
            for access in peripheral['accesses']:
                all_accesses.append(access)
        
        # Check that BOARD_ConfigMPU functions appear before main application logic
        mpu_positions = []
        main_positions = []
        
        for i, access in enumerate(all_accesses):
            func = access.get('source_location', {}).get('function', '')
            if 'BOARD_ConfigMPU' in func:
                mpu_positions.append(i)
            elif 'main' in func or 'XSPI_' in func:
                main_positions.append(i)
        
        print(f"   MPU function positions: {mpu_positions[:5]}{'...' if len(mpu_positions) > 5 else ''}")
        print(f"   Main/XSPI positions: {main_positions[:5]}{'...' if len(main_positions) > 5 else ''}")
        
        # Basic logic check: some initialization should happen before main application
        if mpu_positions and main_positions:
            earliest_mpu = min(mpu_positions)
            earliest_main = min(main_positions) if main_positions else float('inf')
            
            if earliest_mpu < earliest_main:
                print(f"   ✅ PASS: MPU initialization (pos {earliest_mpu}) before main logic (pos {earliest_main})")
                self.validation_results['execution_order'] = True
            else:
                print(f"   ⚠️  WARNING: MPU initialization may not be in optimal order")
                self.validation_results['execution_order'] = False
        else:
            print(f"   ⚠️  WARNING: Could not determine execution order relationship")
            self.validation_results['execution_order'] = False
    
    def generate_summary_report(self):
        """Generate final validation summary"""
        print(f"\n🎯 MULTI-MODULE VALIDATION SUMMARY")
        print("=" * 50)
        
        total_tests = len(self.validation_results)
        passed_tests = sum(1 for result in self.validation_results.values() if result)
        
        print(f"Tests passed: {passed_tests}/{total_tests}")
        print(f"Success rate: {passed_tests/total_tests*100:.1f}%")
        
        print(f"\n📋 Detailed Results:")
        for test_name, result in self.validation_results.items():
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"   {status} {test_name.replace('_', ' ').title()}")
        
        if passed_tests == total_tests:
            print(f"\n🏆 ALL MULTI-MODULE TESTS PASSED!")
            print(f"The LLVM analysis pass correctly handles multi-module analysis.")
            return True
        else:
            print(f"\n⚠️  SOME TESTS FAILED")
            print(f"Multi-module analysis needs improvement.")
            return False
    
    def run_validation(self):
        """Run the complete multi-module validation"""
        print("🧪 MULTI-MODULE EXECUTION ORDER VALIDATION")
        print("=" * 60)
        print("MISSION: Validate that multi-module LLVM analysis works correctly")
        print("=" * 60)
        
        if not self.load_analysis_results():
            return False
        
        # Run all validation tests
        self.test_multi_module_coverage()
        self.test_critical_mpu_sequence()
        self.test_cache_sequence()
        self.test_execution_order_logic()
        
        # Generate final report
        return self.generate_summary_report()

def main():
    validator = MultiModuleExecutionOrderValidator()
    success = validator.run_validation()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
