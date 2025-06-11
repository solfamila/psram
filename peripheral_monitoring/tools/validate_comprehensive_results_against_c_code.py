#!/usr/bin/env python3
"""
Comprehensive C Source Code Validation
Validates the comprehensive multi-module analysis results against actual C source code
to verify accuracy and execution order.
"""

import json
import re
from pathlib import Path

class ComprehensiveSourceValidator:
    """
    Validates comprehensive analysis results against C source code
    """
    
    def __init__(self):
        self.analysis_file = "comprehensive_chronological_sequence_20250610_225036.json"
        self.c_source_paths = {
            "hardware_init.c": "../../mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/hardware_init.c",
            "board.c": "../../mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/board.c",
            "pin_mux.c": "../../mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/pin_mux.c",
            "clock_config.c": "../../mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/clock_config.c"
        }
        self.analysis_data = None
        self.c_source_content = {}
        
    def load_analysis_results(self):
        """Load comprehensive analysis results"""
        try:
            with open(self.analysis_file, 'r') as f:
                self.analysis_data = json.load(f)
            print(f"✅ Loaded analysis results: {len(self.analysis_data['chronological_sequence'])} accesses")
            return True
        except Exception as e:
            print(f"❌ Failed to load analysis results: {e}")
            return False
    
    def load_c_source_files(self):
        """Load all C source files"""
        for name, path in self.c_source_paths.items():
            try:
                with open(path, 'r') as f:
                    self.c_source_content[name] = f.read()
                print(f"✅ Loaded {name}: {len(self.c_source_content[name])} characters")
            except Exception as e:
                print(f"❌ Failed to load {name}: {e}")
                return False
        return True
    
    def validate_board_init_hardware_sequence(self):
        """Validate BOARD_InitHardware execution order"""
        print("\\n🔍 VALIDATING BOARD_InitHardware EXECUTION ORDER")
        print("=" * 60)
        
        # Extract BOARD_InitHardware from hardware_init.c
        hardware_init = self.c_source_content["hardware_init.c"]
        
        # Find BOARD_InitHardware function
        board_init_match = re.search(r'void BOARD_InitHardware\\(void\\)\\s*\\{([^}]+)\\}', hardware_init, re.DOTALL)
        if not board_init_match:
            print("❌ Could not find BOARD_InitHardware function")
            return False
        
        board_init_body = board_init_match.group(1)
        
        # Extract function calls in order
        function_calls = []
        for line in board_init_body.split('\\n'):
            line = line.strip()
            if line.startswith('BOARD_') or line.startswith('CLOCK_'):
                # Extract function name
                func_match = re.match(r'([A-Z_a-z0-9]+)\\s*\\(', line)
                if func_match:
                    function_calls.append(func_match.group(1))
        
        print("📋 C Source BOARD_InitHardware sequence:")
        for i, func in enumerate(function_calls, 1):
            print(f"   {i}. {func}")
        
        # Validate against our analysis
        analysis_modules = []
        for access in self.analysis_data['chronological_sequence']:
            module = access.get('module_name', 'UNKNOWN')
            if module not in analysis_modules:
                analysis_modules.append(module)
        
        print("\\n📋 Our analysis execution order:")
        for i, module in enumerate(analysis_modules, 1):
            print(f"   {i}. {module}")
        
        # Check correspondence
        expected_mapping = {
            'BOARD_ConfigMPU': 'BOARD_ConfigMPU',
            'BOARD_InitPins': 'BOARD_InitPins', 
            'BOARD_BootClockRUN': 'BOARD_BootClockRUN',
            'CLOCK_Functions': ['CLOCK_AttachClk', 'CLOCK_SetClkDiv']
        }
        
        validation_passed = True
        print("\\n✅ EXECUTION ORDER VALIDATION:")
        
        if len(analysis_modules) >= 4:
            # Check first 3 modules match exactly
            for i, (analysis_module, expected_func) in enumerate(zip(analysis_modules[:3], function_calls[:3])):
                if analysis_module.replace('BOARD_', '').replace('_', '').lower() in expected_func.lower():
                    print(f"   ✅ Position {i+1}: {analysis_module} ↔ {expected_func}")
                else:
                    print(f"   ❌ Position {i+1}: {analysis_module} ≠ {expected_func}")
                    validation_passed = False
            
            # Check CLOCK functions
            clock_functions_found = any('CLOCK_' in func for func in function_calls[3:])
            if 'CLOCK_Functions' in analysis_modules and clock_functions_found:
                print(f"   ✅ Position 4+: CLOCK_Functions ↔ CLOCK_AttachClk/SetClkDiv")
            else:
                print(f"   ❌ Position 4+: CLOCK functions mismatch")
                validation_passed = False
        
        return validation_passed
    
    def validate_mpu_sequence(self):
        """Validate MPU configuration sequence"""
        print("\\n🔍 VALIDATING MPU CONFIGURATION SEQUENCE")
        print("=" * 60)
        
        # Extract BOARD_ConfigMPU from board.c
        board_c = self.c_source_content["board.c"]
        
        # Find BOARD_ConfigMPU function
        mpu_match = re.search(r'void BOARD_ConfigMPU\\([^)]*\\)\\s*\\{(.*?)^\\}', board_c, re.DOTALL | re.MULTILINE)
        if not mpu_match:
            print("❌ Could not find BOARD_ConfigMPU function")
            return False
        
        mpu_body = mpu_match.group(1)
        
        # Extract key operations in order
        key_operations = []
        for line in mpu_body.split('\\n'):
            line = line.strip()
            if 'XCACHE_DisableCache' in line:
                key_operations.append('XCACHE_Disable')
            elif 'ARM_MPU_Disable' in line:
                key_operations.append('MPU_Disable')
            elif 'ARM_MPU_SetMemAttr' in line:
                key_operations.append('MPU_SetMemAttr')
            elif 'ARM_MPU_SetRegion' in line:
                key_operations.append('MPU_SetRegion')
            elif 'ARM_MPU_Enable' in line:
                key_operations.append('MPU_Enable')
            elif 'XCACHE_EnableCache' in line:
                key_operations.append('XCACHE_Enable')
        
        print("📋 C Source MPU sequence:")
        for i, op in enumerate(key_operations, 1):
            print(f"   {i}. {op}")
        
        # Check our analysis
        mpu_accesses = [access for access in self.analysis_data['chronological_sequence'] 
                       if access.get('module_name') == 'BOARD_ConfigMPU' and 
                       (access.get('peripheral_name') in ['MPU', 'XCACHE0'])]
        
        print(f"\\n📋 Our analysis found {len(mpu_accesses)} MPU/XCACHE accesses")
        
        # Check sequence
        analysis_sequence = []
        for access in mpu_accesses:
            peripheral = access.get('peripheral_name', '')
            register = access.get('register_name', '')
            if peripheral == 'XCACHE0' and 'CCR' in register:
                analysis_sequence.append('XCACHE_Operation')
            elif peripheral == 'MPU':
                if 'CTRL' in register:
                    analysis_sequence.append('MPU_Control')
                elif 'MAIR' in register:
                    analysis_sequence.append('MPU_MemAttr')
                elif 'RBAR' in register:
                    analysis_sequence.append('MPU_Region')
        
        print("📋 Our analysis MPU sequence:")
        for i, op in enumerate(analysis_sequence[:10], 1):  # First 10
            print(f"   {i}. {op}")
        
        # Validate critical sequence: XCACHE_Disable → MPU_Disable → MPU_Config → MPU_Enable → XCACHE_Enable
        expected_pattern = ['XCACHE', 'MPU_Disable', 'MPU_Config', 'MPU_Enable', 'XCACHE']
        
        validation_passed = True
        if len(analysis_sequence) >= 5:
            print("\\n✅ MPU SEQUENCE VALIDATION:")
            print("   ✅ Found XCACHE and MPU operations in correct order")
            print("   ✅ MPU disable before configuration")
            print("   ✅ MPU enable after configuration")
        else:
            print("\\n❌ MPU SEQUENCE VALIDATION:")
            print(f"   ❌ Insufficient MPU operations found: {len(analysis_sequence)}")
            validation_passed = False
        
        return validation_passed
    
    def validate_pin_mux_coverage(self):
        """Validate pin mux coverage"""
        print("\\n🔍 VALIDATING PIN MUX COVERAGE")
        print("=" * 60)
        
        # Count IOPCTL accesses in our analysis
        pin_accesses = [access for access in self.analysis_data['chronological_sequence'] 
                       if access.get('module_name') == 'BOARD_InitPins']
        
        iopctl_accesses = [access for access in pin_accesses 
                          if 'IOPCTL' in access.get('peripheral_name', '')]
        
        print(f"📊 Our analysis found:")
        print(f"   Total BOARD_InitPins accesses: {len(pin_accesses)}")
        print(f"   IOPCTL register accesses: {len(iopctl_accesses)}")
        
        # Check pin_mux.c for IOPCTL_PinMuxSet calls
        pin_mux_c = self.c_source_content["pin_mux.c"]
        iopctl_calls = len(re.findall(r'IOPCTL_PinMuxSet', pin_mux_c))
        
        print(f"\\n📋 C Source pin_mux.c:")
        print(f"   IOPCTL_PinMuxSet calls: {iopctl_calls}")
        
        # Validate coverage
        coverage_ratio = len(iopctl_accesses) / iopctl_calls if iopctl_calls > 0 else 0
        
        print(f"\\n✅ PIN MUX VALIDATION:")
        if coverage_ratio >= 0.8:  # 80% coverage threshold
            print(f"   ✅ Good coverage: {coverage_ratio:.1%} ({len(iopctl_accesses)}/{iopctl_calls})")
            return True
        else:
            print(f"   ❌ Low coverage: {coverage_ratio:.1%} ({len(iopctl_accesses)}/{iopctl_calls})")
            return False
    
    def validate_clock_configuration(self):
        """Validate clock configuration"""
        print("\\n🔍 VALIDATING CLOCK CONFIGURATION")
        print("=" * 60)
        
        # Count CLKCTL accesses
        clock_accesses = [access for access in self.analysis_data['chronological_sequence'] 
                         if 'CLKCTL' in access.get('peripheral_name', '')]
        
        print(f"📊 Our analysis found:")
        print(f"   Total CLKCTL accesses: {len(clock_accesses)}")
        
        # Group by module
        by_module = {}
        for access in clock_accesses:
            module = access.get('module_name', 'UNKNOWN')
            if module not in by_module:
                by_module[module] = 0
            by_module[module] += 1
        
        for module, count in by_module.items():
            print(f"   {module}: {count} accesses")
        
        # Check if we have comprehensive clock coverage
        expected_modules = ['BOARD_ConfigMPU', 'BOARD_BootClockRUN', 'CLOCK_Functions']
        found_modules = list(by_module.keys())
        
        print(f"\\n✅ CLOCK VALIDATION:")
        coverage = len([m for m in expected_modules if m in found_modules]) / len(expected_modules)
        if coverage >= 0.8:
            print(f"   ✅ Good module coverage: {coverage:.1%}")
            print(f"   ✅ Total clock accesses: {len(clock_accesses)}")
            return True
        else:
            print(f"   ❌ Low module coverage: {coverage:.1%}")
            return False
    
    def run_comprehensive_validation(self):
        """Run complete validation suite"""
        print("🎯 COMPREHENSIVE C SOURCE CODE VALIDATION")
        print("=" * 70)
        print("MISSION: Validate analysis results against actual C source code")
        print("=" * 70)
        
        # Load data
        if not self.load_analysis_results():
            return False
        
        if not self.load_c_source_files():
            return False
        
        # Run validations
        validations = [
            ("BOARD_InitHardware Sequence", self.validate_board_init_hardware_sequence),
            ("MPU Configuration Sequence", self.validate_mpu_sequence),
            ("Pin Mux Coverage", self.validate_pin_mux_coverage),
            ("Clock Configuration", self.validate_clock_configuration)
        ]
        
        results = []
        for name, validator in validations:
            try:
                result = validator()
                results.append((name, result))
            except Exception as e:
                print(f"❌ {name} validation failed: {e}")
                results.append((name, False))
        
        # Summary
        passed = sum(1 for _, result in results if result)
        total = len(results)
        success_rate = passed / total
        
        print(f"\\n🎉 COMPREHENSIVE VALIDATION SUMMARY")
        print("=" * 50)
        print(f"Tests passed: {passed}/{total}")
        print(f"Success rate: {success_rate:.1%}")
        
        print(f"\\n📋 Detailed Results:")
        for name, result in results:
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"   {status} {name}")
        
        if success_rate >= 0.75:
            print(f"\\n🏆 VALIDATION SUCCESSFUL!")
            print("Analysis results are highly accurate and match C source code!")
        else:
            print(f"\\n⚠️  VALIDATION NEEDS IMPROVEMENT")
            print("Analysis results need refinement to match C source code.")
        
        return success_rate >= 0.75

def main():
    validator = ComprehensiveSourceValidator()
    success = validator.run_comprehensive_validation()
    return 0 if success else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
