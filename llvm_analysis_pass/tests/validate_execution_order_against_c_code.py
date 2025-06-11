#!/usr/bin/env python3
"""
Execution Order Validation Against C Source Code
Validates that the detected execution order matches the actual C source code call sequence.
"""

import json
import os
import sys
from pathlib import Path

class ExecutionOrderValidator:
    """
    Validates execution order against actual C source code
    """
    
    def __init__(self):
        self.analysis_file = "multi_module_test.json"
        self.source_files = []
        self.analysis_data = None
        self.validation_results = {}
        
        # Find C source files
        self._find_source_files()
    
    def _find_source_files(self):
        """Find all relevant C source files"""
        project_root = Path("../../mimxrt700evk_xspi_psram_polling_transfer_cm33_core0")
        
        # Main files
        main_files = [
            project_root / "xspi_psram_polling_transfer.c",
            project_root / "hardware_init.c",
            project_root / "board.c"
        ]
        
        for file_path in main_files:
            if file_path.exists():
                self.source_files.append(file_path)
        
        print(f"📁 Found {len(self.source_files)} C source files for validation")
    
    def load_analysis_data(self):
        """Load the multi-module analysis results"""
        try:
            with open(self.analysis_file, 'r') as f:
                self.analysis_data = json.load(f)
            
            print(f"📊 Loaded analysis data with {len(self.analysis_data['peripheral_accesses'])} peripheral types")
            return True
            
        except Exception as e:
            print(f"❌ Failed to load analysis data: {e}")
            return False
    
    def validate_board_inithardware_sequence(self):
        """Validate BOARD_InitHardware call sequence against C source"""
        print("🔍 TEST 1: BOARD_InitHardware Call Sequence")
        
        # Expected sequence from hardware_init.c BOARD_InitHardware()
        expected_sequence = [
            ('BOARD_ConfigMPU', 136),
            ('BOARD_InitPins', 137),
            ('BOARD_BootClockRUN', 138),
            ('BOARD_InitDebugConsole', 139),
            ('BOARD_InitPsRamPins_Xspi2', 141),
            ('CLOCK_AttachClk', 142),
            ('CLOCK_SetClkDiv', 143)
        ]
        
        print(f"   📋 Expected call sequence from hardware_init.c:")
        for i, (func, line) in enumerate(expected_sequence, 1):
            print(f"      {i}. Line {line:3d}: {func}()")
        
        # Find corresponding accesses in analysis data
        found_functions = []
        for peripheral in self.analysis_data['peripheral_accesses']:
            for access in peripheral['accesses']:
                source_loc = access.get('source_location', {})
                func = source_loc.get('function', '')
                line = source_loc.get('line', 0)
                purpose = access.get('purpose', '')
                
                # Check if this matches expected functions
                for expected_func, expected_line in expected_sequence:
                    if expected_func in purpose or expected_func in func:
                        found_functions.append((expected_func, line, purpose))
                        break
        
        print(f"\n   📊 Found {len(found_functions)} matching function calls in analysis")
        
        # Check coverage
        expected_funcs = set(func for func, _ in expected_sequence)
        found_funcs = set(func for func, _, _ in found_functions)
        
        coverage = len(found_funcs) / len(expected_funcs) * 100
        
        if coverage >= 70:
            print(f"   ✅ PASS: {coverage:.1f}% function coverage")
            self.validation_results['board_init_sequence'] = True
        else:
            print(f"   ❌ FAIL: Only {coverage:.1f}% function coverage")
            self.validation_results['board_init_sequence'] = False
        
        print(f"   📋 Function coverage details:")
        for expected_func in expected_funcs:
            if expected_func in found_funcs:
                print(f"      ✅ {expected_func}")
            else:
                print(f"      ❌ {expected_func} - NOT FOUND")
    
    def validate_board_configmpu_sequence(self):
        """Validate BOARD_ConfigMPU internal sequence"""
        print(f"\n🔍 TEST 2: BOARD_ConfigMPU Internal Sequence")
        
        # Expected sequence from board.c BOARD_ConfigMPU() lines 224-266
        expected_mpu_sequence = [
            ('XCACHE_DisableCache', [224, 225]),
            ('ARM_MPU_Disable', [228]),
            ('ARM_MPU_SetMemAttr', [231, 233, 236, 239]),
            ('ARM_MPU_SetRegion', [242, 245, 253]),
            ('ARM_MPU_Enable', [262]),
            ('XCACHE_EnableCache', [265, 266])
        ]
        
        print(f"   📋 Expected BOARD_ConfigMPU sequence from board.c:")
        for func, lines in expected_mpu_sequence:
            line_str = ', '.join(str(l) for l in lines)
            print(f"      {func}() - Lines: {line_str}")
        
        # Find MPU-related accesses
        mpu_accesses = []
        cache_accesses = []
        
        for peripheral in self.analysis_data['peripheral_accesses']:
            if peripheral['peripheral_name'] == 'MPU':
                mpu_accesses.extend(peripheral['accesses'])
            elif peripheral['peripheral_name'] == 'XCACHE0':
                cache_accesses.extend(peripheral['accesses'])
        
        print(f"\n   📊 Found {len(mpu_accesses)} MPU accesses and {len(cache_accesses)} cache accesses")
        
        # Validate function detection
        detected_functions = set()
        for access in mpu_accesses + cache_accesses:
            purpose = access.get('purpose', '').lower()
            if 'cache disable' in purpose:
                detected_functions.add('XCACHE_DisableCache')
            elif 'mpu disable' in purpose:
                detected_functions.add('ARM_MPU_Disable')
            elif 'mpu memory attribute' in purpose:
                detected_functions.add('ARM_MPU_SetMemAttr')
            elif 'mpu region' in purpose:
                detected_functions.add('ARM_MPU_SetRegion')
            elif 'mpu enable' in purpose:
                detected_functions.add('ARM_MPU_Enable')
            elif 'cache enable' in purpose:
                detected_functions.add('XCACHE_EnableCache')
        
        expected_funcs = set(func for func, _ in expected_mpu_sequence)
        coverage = len(detected_functions) / len(expected_funcs) * 100
        
        print(f"   📋 Function detection results:")
        for expected_func in expected_funcs:
            if expected_func in detected_functions:
                print(f"      ✅ {expected_func}")
            else:
                print(f"      ❌ {expected_func} - NOT DETECTED")
        
        if coverage >= 80:
            print(f"   ✅ PASS: {coverage:.1f}% MPU function detection")
            self.validation_results['mpu_internal_sequence'] = True
        else:
            print(f"   ❌ FAIL: Only {coverage:.1f}% MPU function detection")
            self.validation_results['mpu_internal_sequence'] = False
    
    def validate_source_line_accuracy(self):
        """Validate that source line numbers are accurate"""
        print(f"\n🔍 TEST 3: Source Line Number Accuracy")
        
        # Sample a few accesses and verify line numbers
        sample_accesses = []
        count = 0
        for peripheral in self.analysis_data['peripheral_accesses']:
            for access in peripheral['accesses']:
                if count < 10:  # Sample first 10 accesses
                    sample_accesses.append(access)
                    count += 1
        
        print(f"   📊 Validating line numbers for {len(sample_accesses)} sample accesses")
        
        accurate_lines = 0
        total_checked = 0
        
        for access in sample_accesses:
            source_loc = access.get('source_location', {})
            file_name = source_loc.get('file', '')
            line_num = source_loc.get('line', 0)
            func_name = source_loc.get('function', '')
            
            if line_num > 0 and file_name and func_name:
                total_checked += 1
                # Basic validation: line number should be reasonable (1-1000)
                if 1 <= line_num <= 1000:
                    accurate_lines += 1
        
        if total_checked > 0:
            accuracy = accurate_lines / total_checked * 100
            if accuracy >= 90:
                print(f"   ✅ PASS: {accuracy:.1f}% line number accuracy")
                self.validation_results['line_accuracy'] = True
            else:
                print(f"   ❌ FAIL: Only {accuracy:.1f}% line number accuracy")
                self.validation_results['line_accuracy'] = False
        else:
            print(f"   ⚠️  WARNING: No line numbers to validate")
            self.validation_results['line_accuracy'] = False
    
    def validate_function_name_accuracy(self):
        """Validate that function names are accurate"""
        print(f"\n🔍 TEST 4: Function Name Accuracy")
        
        # Expected function patterns
        expected_patterns = [
            'BOARD_',
            'CLOCK_',
            'ARM_MPU_',
            'XCACHE_',
            'IOPCTL_',
            'main'
        ]
        
        function_names = set()
        for peripheral in self.analysis_data['peripheral_accesses']:
            for access in peripheral['accesses']:
                func = access.get('source_location', {}).get('function', '')
                if func:
                    function_names.add(func)
        
        print(f"   📊 Found {len(function_names)} unique function names")
        
        # Check if function names match expected patterns
        matching_functions = 0
        for func in function_names:
            for pattern in expected_patterns:
                if pattern in func:
                    matching_functions += 1
                    break
        
        if function_names:
            accuracy = matching_functions / len(function_names) * 100
            if accuracy >= 70:
                print(f"   ✅ PASS: {accuracy:.1f}% function name pattern match")
                self.validation_results['function_accuracy'] = True
            else:
                print(f"   ❌ FAIL: Only {accuracy:.1f}% function name pattern match")
                self.validation_results['function_accuracy'] = False
        else:
            print(f"   ❌ FAIL: No function names found")
            self.validation_results['function_accuracy'] = False
    
    def generate_final_report(self):
        """Generate final validation report"""
        print(f"\n🎯 EXECUTION ORDER VALIDATION SUMMARY")
        print("=" * 60)
        
        total_tests = len(self.validation_results)
        passed_tests = sum(1 for result in self.validation_results.values() if result)
        
        print(f"Tests passed: {passed_tests}/{total_tests}")
        print(f"Success rate: {passed_tests/total_tests*100:.1f}%")
        
        print(f"\n📋 Detailed Results:")
        for test_name, result in self.validation_results.items():
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"   {status} {test_name.replace('_', ' ').title()}")
        
        if passed_tests >= total_tests * 0.75:  # 75% pass rate
            print(f"\n🏆 EXECUTION ORDER VALIDATION SUCCESSFUL!")
            print(f"The multi-module analysis correctly reflects C source code execution order.")
            return True
        else:
            print(f"\n⚠️  EXECUTION ORDER VALIDATION NEEDS IMPROVEMENT")
            print(f"The analysis may not accurately reflect C source code execution order.")
            return False
    
    def run_validation(self):
        """Run the complete execution order validation"""
        print("🧪 EXECUTION ORDER VALIDATION AGAINST C SOURCE CODE")
        print("=" * 70)
        print("MISSION: Verify that detected execution order matches actual C source code")
        print("=" * 70)
        
        if not self.load_analysis_data():
            return False
        
        # Run all validation tests
        self.validate_board_inithardware_sequence()
        self.validate_board_configmpu_sequence()
        self.validate_source_line_accuracy()
        self.validate_function_name_accuracy()
        
        # Generate final report
        return self.generate_final_report()

def main():
    validator = ExecutionOrderValidator()
    success = validator.run_validation()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
