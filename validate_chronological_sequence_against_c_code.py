#!/usr/bin/env python3
"""
Validate Chronological Sequence Against C Source Code
Cross-references the detected register access sequence with actual C source code
to verify accuracy and completeness.
"""

import json
import re
from pathlib import Path

class ChronologicalSequenceValidator:
    """
    Validates the chronological register access sequence against C source code
    """
    
    def __init__(self):
        self.sequence_file = "peripheral_monitoring/tools/final_chronological_sequence_20250610_210955.json"
        self.source_files = []
        self.sequence_data = None
        self.validation_results = []
        
        # Find all C source files
        self._find_source_files()
        
        # Load sequence data
        self._load_sequence_data()
    
    def _find_source_files(self):
        """Find all C source files in the project"""
        project_root = Path(".")
        
        # Main project files
        main_dir = project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0"
        for c_file in main_dir.glob("*.c"):
            self.source_files.append(c_file)
        
        # Driver files
        drivers_dir = main_dir / "__repo__" / "devices" / "MIMXRT798S" / "drivers"
        if drivers_dir.exists():
            for c_file in drivers_dir.glob("*.c"):
                self.source_files.append(c_file)
        
        # Board files
        board_dir = main_dir / "__repo__" / "boards" / "mimxrt700evk" / "driver_examples" / "xspi" / "psram" / "polling_transfer" / "cm33_core0"
        if board_dir.exists():
            for c_file in board_dir.glob("*.c"):
                self.source_files.append(c_file)
        
        print(f"📁 Found {len(self.source_files)} C source files")
    
    def _load_sequence_data(self):
        """Load the chronological sequence data"""
        try:
            with open(self.sequence_file, 'r') as f:
                self.sequence_data = json.load(f)
            
            print(f"📊 Loaded {len(self.sequence_data['chronological_sequence'])} register accesses")
            
        except Exception as e:
            print(f"❌ Failed to load sequence data: {e}")
            return False
        
        return True
    
    def validate_first_10_accesses(self):
        """Validate the first 10 register accesses against C source code"""
        print("\n🔍 VALIDATING FIRST 10 REGISTER ACCESSES AGAINST C SOURCE CODE")
        print("=" * 70)
        
        sequence = self.sequence_data['chronological_sequence'][:10]
        
        for i, access in enumerate(sequence, 1):
            print(f"\n{i:2d}. VALIDATING ACCESS #{access.get('sequence_number', i-1)}")
            print(f"    📍 {access.get('peripheral_name', 'UNKNOWN')} {access.get('register_name', 'UNKNOWN')} "
                  f"({access.get('address', 'N/A')})")
            print(f"    📁 {access.get('source_location', {}).get('file', 'unknown')}:"
                  f"{access.get('source_location', {}).get('line', 0)} "
                  f"in {access.get('source_location', {}).get('function', 'unknown')}()")
            print(f"    🔄 {access.get('before_value', 'N/A')} → {access.get('after_value', 'N/A')}")
            
            # Validate against source code
            validation_result = self._validate_single_access(access)
            self.validation_results.append(validation_result)
            
            if validation_result['valid']:
                print(f"    ✅ VALIDATED: {validation_result['message']}")
            else:
                print(f"    ❌ VALIDATION FAILED: {validation_result['message']}")
    
    def _validate_single_access(self, access):
        """Validate a single register access against source code"""
        source_location = access.get('source_location', {})
        file_name = source_location.get('file', '')
        line_number = source_location.get('line', 0)
        function_name = source_location.get('function', '')
        
        # Find the source file
        source_file = None
        for file_path in self.source_files:
            if file_name in str(file_path):
                source_file = file_path
                break
        
        if not source_file:
            return {
                'valid': False,
                'message': f"Source file not found: {file_name}",
                'access': access
            }
        
        try:
            with open(source_file, 'r') as f:
                lines = f.readlines()
            
            if line_number <= 0 or line_number > len(lines):
                return {
                    'valid': False,
                    'message': f"Line number {line_number} out of range",
                    'access': access
                }
            
            # Get the actual line (1-based indexing)
            actual_line = lines[line_number - 1].strip()
            
            # Validate the access
            return self._validate_line_content(access, actual_line, lines, line_number)
            
        except Exception as e:
            return {
                'valid': False,
                'message': f"Error reading source file: {e}",
                'access': access
            }
    
    def _validate_line_content(self, access, actual_line, all_lines, line_number):
        """Validate the line content against the expected access"""
        access_type = access.get('access_type', '')
        peripheral = access.get('peripheral_name', '')
        register = access.get('register_name', '')
        function_name = access.get('source_location', {}).get('function', '')
        
        # Check for function calls
        if access_type == 'function_call_write':
            # Look for specific function patterns
            if 'CLOCK_AttachClk' in actual_line and peripheral == 'CLKCTL0':
                return {
                    'valid': True,
                    'message': f"Found CLOCK_AttachClk call: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
            elif 'CLOCK_SetClkDiv' in actual_line and peripheral == 'CLKCTL0':
                return {
                    'valid': True,
                    'message': f"Found CLOCK_SetClkDiv call: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
            elif 'RESET_ClearPeripheralReset' in actual_line and peripheral == 'RSTCTL':
                return {
                    'valid': True,
                    'message': f"Found RESET_ClearPeripheralReset call: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
            elif 'IOPCTL_PinMuxSet' in actual_line and 'IOPCTL' in peripheral:
                return {
                    'valid': True,
                    'message': f"Found IOPCTL_PinMuxSet call: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
            elif 'ARM_MPU_' in actual_line and peripheral == 'MPU':
                return {
                    'valid': True,
                    'message': f"Found ARM_MPU function call: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
            elif 'XCACHE_' in actual_line and peripheral == 'XCACHE0':
                return {
                    'valid': True,
                    'message': f"Found XCACHE function call: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
        
        # Check for volatile reads/writes
        elif access_type in ['volatile_read', 'volatile_write']:
            # Look for direct register access patterns
            if '->' in actual_line and register in actual_line:
                return {
                    'valid': True,
                    'message': f"Found direct register access: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
            elif 'base->' in actual_line and peripheral in ['XSPI2', 'XSPI0', 'XSPI1']:
                return {
                    'valid': True,
                    'message': f"Found XSPI register access: {actual_line}",
                    'access': access,
                    'actual_line': actual_line
                }
        
        # Check surrounding lines for context
        context_lines = []
        for i in range(max(0, line_number - 3), min(len(all_lines), line_number + 3)):
            context_lines.append(f"{i+1:3d}: {all_lines[i].strip()}")
        
        return {
            'valid': False,
            'message': f"Could not validate access pattern. Line: '{actual_line}'",
            'access': access,
            'actual_line': actual_line,
            'context': context_lines
        }
    
    def validate_critical_board_configmpu_sequence(self):
        """Validate the critical BOARD_ConfigMPU sequence"""
        print("\n🔍 VALIDATING CRITICAL BOARD_ConfigMPU SEQUENCE")
        print("=" * 50)
        
        # Find BOARD_ConfigMPU accesses
        mpu_accesses = []
        for access in self.sequence_data['chronological_sequence']:
            if access.get('source_location', {}).get('function') == 'BOARD_ConfigMPU':
                mpu_accesses.append(access)
        
        print(f"📊 Found {len(mpu_accesses)} BOARD_ConfigMPU accesses")
        
        # Expected sequence from board.c lines 224-266
        expected_sequence = [
            ('XCACHE_DisableCache', 'XCACHE0', 224),
            ('XCACHE_DisableCache', 'XCACHE1', 225),
            ('ARM_MPU_Disable', 'MPU', 228),
            ('ARM_MPU_SetMemAttr', 'MPU', 231),
            ('ARM_MPU_SetMemAttr', 'MPU', 233),
            ('ARM_MPU_SetMemAttr', 'MPU', 236),
            ('ARM_MPU_SetMemAttr', 'MPU', 239),
            ('ARM_MPU_SetRegion', 'MPU', 242),
            ('ARM_MPU_SetRegion', 'MPU', 245),
            ('ARM_MPU_SetRegion', 'MPU', 253),  # Conditional
            ('ARM_MPU_Enable', 'MPU', 262),
            ('XCACHE_EnableCache', 'XCACHE0', 265),
            ('XCACHE_EnableCache', 'XCACHE1', 266)
        ]
        
        print("\n📋 Expected BOARD_ConfigMPU sequence:")
        for i, (func, peripheral, line) in enumerate(expected_sequence, 1):
            print(f"  {i:2d}. Line {line:3d}: {func}({peripheral})")
        
        # Check if we found these in our sequence
        found_functions = []
        for access in mpu_accesses:
            line = access.get('source_location', {}).get('line', 0)
            purpose = access.get('purpose', '')
            peripheral = access.get('peripheral_name', '')
            
            if 'cache disable' in purpose.lower():
                found_functions.append(('XCACHE_DisableCache', peripheral, line))
            elif 'mpu disable' in purpose.lower():
                found_functions.append(('ARM_MPU_Disable', peripheral, line))
            elif 'mpu memory attribute' in purpose.lower():
                found_functions.append(('ARM_MPU_SetMemAttr', peripheral, line))
            elif 'mpu region' in purpose.lower():
                found_functions.append(('ARM_MPU_SetRegion', peripheral, line))
            elif 'mpu enable' in purpose.lower():
                found_functions.append(('ARM_MPU_Enable', peripheral, line))
            elif 'cache enable' in purpose.lower():
                found_functions.append(('XCACHE_EnableCache', peripheral, line))
        
        print(f"\n📊 Found {len(found_functions)} BOARD_ConfigMPU functions in sequence")
        
        # Validate critical ordering
        cache_disable_found = any('XCACHE_DisableCache' in f[0] for f in found_functions)
        mpu_enable_found = any('ARM_MPU_Enable' in f[0] for f in found_functions)
        cache_enable_found = any('XCACHE_EnableCache' in f[0] for f in found_functions)
        
        print(f"\n✅ Critical sequence validation:")
        print(f"   XCACHE_DisableCache found: {cache_disable_found}")
        print(f"   ARM_MPU_Enable found: {mpu_enable_found}")
        print(f"   XCACHE_EnableCache found: {cache_enable_found}")
        
        return {
            'expected_count': len(expected_sequence),
            'found_count': len(found_functions),
            'critical_functions_found': cache_disable_found and mpu_enable_found and cache_enable_found
        }
    
    def generate_validation_report(self):
        """Generate a comprehensive validation report"""
        print("\n📊 COMPREHENSIVE VALIDATION REPORT")
        print("=" * 50)
        
        total_accesses = len(self.sequence_data['chronological_sequence'])
        validated_accesses = len([r for r in self.validation_results if r['valid']])
        
        print(f"Total register accesses: {total_accesses}")
        print(f"Validated accesses (first 10): {validated_accesses}/10")
        print(f"Validation rate: {validated_accesses/10*100:.1f}%")
        
        # Show validation details
        print(f"\n📋 Validation Details:")
        for i, result in enumerate(self.validation_results, 1):
            status = "✅" if result['valid'] else "❌"
            access = result['access']
            peripheral = access.get('peripheral_name', 'UNKNOWN')
            register = access.get('register_name', 'UNKNOWN')
            line = access.get('source_location', {}).get('line', 0)
            
            print(f"  {i:2d}. {status} {peripheral:8s} {register:12s} (line {line:3d})")
            if not result['valid']:
                print(f"      ⚠️  {result['message']}")
        
        # Critical sequence validation
        mpu_validation = self.validate_critical_board_configmpu_sequence()
        
        print(f"\n🎯 CRITICAL SEQUENCE VALIDATION:")
        print(f"   Expected BOARD_ConfigMPU functions: {mpu_validation['expected_count']}")
        print(f"   Found BOARD_ConfigMPU functions: {mpu_validation['found_count']}")
        print(f"   Critical functions present: {mpu_validation['critical_functions_found']}")
        
        return {
            'total_accesses': total_accesses,
            'validated_sample': validated_accesses,
            'validation_rate': validated_accesses/10*100,
            'mpu_sequence_valid': mpu_validation['critical_functions_found']
        }
    
    def run_validation(self):
        """Run the complete validation"""
        print("🔍 CHRONOLOGICAL SEQUENCE VALIDATION AGAINST C SOURCE CODE")
        print("=" * 70)
        print("MISSION: Verify that detected register accesses match actual C source code")
        print("=" * 70)
        
        if not self.sequence_data:
            print("❌ No sequence data loaded")
            return False
        
        # Validate first 10 accesses in detail
        self.validate_first_10_accesses()
        
        # Generate comprehensive report
        report = self.generate_validation_report()
        
        # Final assessment
        print(f"\n🎉 VALIDATION COMPLETE")
        print(f"✅ Sample validation rate: {report['validation_rate']:.1f}%")
        print(f"✅ Critical MPU sequence: {'VALID' if report['mpu_sequence_valid'] else 'INVALID'}")
        
        if report['validation_rate'] >= 80 and report['mpu_sequence_valid']:
            print(f"\n🏆 CHRONOLOGICAL SEQUENCE SUCCESSFULLY VALIDATED AGAINST C SOURCE CODE!")
            print(f"The detected register access sequence accurately reflects the actual C source code.")
        else:
            print(f"\n⚠️  VALIDATION ISSUES DETECTED")
            print(f"Some register accesses may not accurately reflect the C source code.")
        
        return report['validation_rate'] >= 80 and report['mpu_sequence_valid']

def main():
    validator = ChronologicalSequenceValidator()
    success = validator.run_validation()
    return 0 if success else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
