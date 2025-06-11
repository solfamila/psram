#!/usr/bin/env python3
"""
MIMXRT700 Source Code Validation Framework

This module provides comprehensive source code analysis and validation
for peripheral register operations, ensuring accuracy between source
code expectations and captured hardware values.

Critical Issue: MPU_CTRL register shows 0x00000000 but should be 0x00000007
"""

import os
import re
import json
import logging
from typing import Dict, List, Tuple, Optional
from pathlib import Path

class SourceCodeValidator:
    """
    Validates source code expectations against captured register values
    """
    
    def __init__(self, project_root: str, cmsis_headers_path: str):
        self.project_root = Path(project_root)
        self.cmsis_headers_path = Path(cmsis_headers_path)
        self.register_definitions = {}
        self.expected_values = {}
        self.bit_mask_definitions = {}
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Initialize validation
        self._parse_cmsis_headers()
        self._create_register_database()
    
    def _parse_cmsis_headers(self):
        """Parse CMSIS header files for register bit definitions"""
        self.logger.info("Parsing CMSIS header files...")
        
        # Core CM33 header file
        core_cm33_header = self.cmsis_headers_path / "CMSIS/Core/Include/core_cm33.h"
        if core_cm33_header.exists():
            self._parse_mpu_definitions(core_cm33_header)
        else:
            self.logger.error(f"Core CM33 header not found: {core_cm33_header}")
    
    def _parse_mpu_definitions(self, header_file: Path):
        """Parse MPU register definitions from core_cm33.h"""
        self.logger.info(f"Parsing MPU definitions from {header_file}")
        
        with open(header_file, 'r') as f:
            content = f.read()
        
        # Parse MPU_CTRL bit mask definitions
        mpu_ctrl_patterns = {
            'ENABLE': r'#define\s+MPU_CTRL_ENABLE_Pos\s+(\d+)U',
            'HFNMIENA': r'#define\s+MPU_CTRL_HFNMIENA_Pos\s+(\d+)U',
            'PRIVDEFENA': r'#define\s+MPU_CTRL_PRIVDEFENA_Pos\s+(\d+)U'
        }
        
        mpu_ctrl_bits = {}
        for bit_name, pattern in mpu_ctrl_patterns.items():
            match = re.search(pattern, content)
            if match:
                position = int(match.group(1))
                mask = 1 << position
                mpu_ctrl_bits[bit_name] = {
                    'position': position,
                    'mask': f'0x{mask:08x}'
                }
                self.logger.info(f"Found {bit_name}: position={position}, mask=0x{mask:08x}")
        
        # Store MPU_CTRL register definition
        self.register_definitions['MPU_CTRL'] = {
            'address': '0xe000ed94',
            'bit_fields': mpu_ctrl_bits
        }
    
    def _create_register_database(self):
        """Create comprehensive register definition database"""
        self.logger.info("Creating register definition database...")
        
        # MPU_CTRL specific analysis
        if 'MPU_CTRL' in self.register_definitions:
            mpu_ctrl = self.register_definitions['MPU_CTRL']
            bit_fields = mpu_ctrl['bit_fields']
            
            # Calculate expected value for ARM_MPU_Enable operation
            if all(bit in bit_fields for bit in ['ENABLE', 'HFNMIENA', 'PRIVDEFENA']):
                enable_mask = int(bit_fields['ENABLE']['mask'], 16)
                hfnmiena_mask = int(bit_fields['HFNMIENA']['mask'], 16)
                privdefena_mask = int(bit_fields['PRIVDEFENA']['mask'], 16)
                
                # Input to ARM_MPU_Enable: MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk
                input_mask = privdefena_mask | hfnmiena_mask
                
                # ARM_MPU_Enable adds the ENABLE bit
                final_value = input_mask | enable_mask
                
                mpu_ctrl['expected_operations'] = {
                    'ARM_MPU_Enable': {
                        'input_mask': f'0x{input_mask:08x}',
                        'enable_bit_added': f'0x{enable_mask:08x}',
                        'final_expected': f'0x{final_value:08x}',
                        'calculation': f'({privdefena_mask:#x} | {hfnmiena_mask:#x}) | {enable_mask:#x} = {final_value:#x}'
                    }
                }
                
                self.logger.info(f"MPU_CTRL expected value calculation:")
                self.logger.info(f"  PRIVDEFENA_Msk: {privdefena_mask:#x}")
                self.logger.info(f"  HFNMIENA_Msk: {hfnmiena_mask:#x}")
                self.logger.info(f"  Input mask: {input_mask:#x}")
                self.logger.info(f"  ENABLE_Msk: {enable_mask:#x}")
                self.logger.info(f"  Final expected: {final_value:#x}")
    
    def analyze_source_file(self, file_path: str) -> Dict:
        """Analyze C source file for register operations"""
        self.logger.info(f"Analyzing source file: {file_path}")
        
        source_path = Path(file_path)
        if not source_path.exists():
            self.logger.error(f"Source file not found: {file_path}")
            return {}
        
        with open(source_path, 'r') as f:
            content = f.read()
        
        # Find ARM_MPU_Enable calls
        mpu_enable_pattern = r'ARM_MPU_Enable\s*\(\s*([^)]+)\s*\)'
        matches = re.finditer(mpu_enable_pattern, content)
        
        operations = []
        for match in matches:
            line_number = content[:match.start()].count('\n') + 1
            operation = {
                'function': 'ARM_MPU_Enable',
                'parameters': match.group(1).strip(),
                'line_number': line_number,
                'file_path': str(source_path)
            }
            operations.append(operation)
            self.logger.info(f"Found ARM_MPU_Enable at line {line_number}: {match.group(1)}")
        
        return {
            'file_path': str(source_path),
            'operations': operations
        }
    
    def validate_mpu_ctrl_operation(self, source_file: str = None) -> Dict:
        """Specific validation for MPU_CTRL operation"""
        self.logger.info("Validating MPU_CTRL operation...")
        
        # Use default board.c if not specified
        if source_file is None:
            source_file = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/board.c"
        
        # Analyze source file
        analysis = self.analyze_source_file(source_file)
        
        # Get expected value from register database
        expected_info = self.register_definitions.get('MPU_CTRL', {}).get('expected_operations', {}).get('ARM_MPU_Enable', {})
        
        validation_result = {
            'register': 'MPU_CTRL',
            'address': '0xe000ed94',
            'source_file': source_file,
            'expected_value': expected_info.get('final_expected', 'UNKNOWN'),
            'calculation': expected_info.get('calculation', 'UNKNOWN'),
            'source_operations': analysis.get('operations', []),
            'validation_status': 'PENDING_HARDWARE_VERIFICATION'
        }
        
        self.logger.info(f"MPU_CTRL validation result:")
        self.logger.info(f"  Expected value: {validation_result['expected_value']}")
        self.logger.info(f"  Calculation: {validation_result['calculation']}")
        
        return validation_result
    
    def get_expected_value(self, register_name: str, operation: str) -> Optional[int]:
        """Get expected register value for a specific operation"""
        register_def = self.register_definitions.get(register_name, {})
        operation_def = register_def.get('expected_operations', {}).get(operation, {})
        
        expected_hex = operation_def.get('final_expected')
        if expected_hex:
            return int(expected_hex, 16)
        
        return None
    
    def export_register_database(self, output_file: str):
        """Export register definitions to JSON file"""
        self.logger.info(f"Exporting register database to {output_file}")
        
        with open(output_file, 'w') as f:
            json.dump(self.register_definitions, f, indent=2)
    
    def validate_against_captured_value(self, register_name: str, operation: str, captured_value: int) -> Dict:
        """Validate captured value against expected value"""
        expected_value = self.get_expected_value(register_name, operation)
        
        if expected_value is None:
            return {
                'status': 'ERROR',
                'message': f'No expected value found for {register_name}:{operation}'
            }
        
        validation_passed = captured_value == expected_value
        
        result = {
            'register': register_name,
            'operation': operation,
            'expected_value': f'0x{expected_value:08x}',
            'captured_value': f'0x{captured_value:08x}',
            'validation_passed': validation_passed,
            'status': 'PASS' if validation_passed else 'FAIL'
        }
        
        if not validation_passed:
            result['error_details'] = {
                'expected_decimal': expected_value,
                'captured_decimal': captured_value,
                'difference': captured_value - expected_value,
                'difference_hex': f'0x{(captured_value - expected_value) & 0xFFFFFFFF:08x}'
            }
        
        return result

def main():
    """Main function for testing the source code validator"""
    print("🔍 MIMXRT700 Source Code Validator")
    print("=" * 50)
    
    # Initialize validator
    project_root = "."
    cmsis_headers = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
    
    validator = SourceCodeValidator(project_root, cmsis_headers)
    
    # Validate MPU_CTRL operation
    print("\n📋 MPU_CTRL Validation:")
    mpu_validation = validator.validate_mpu_ctrl_operation()
    
    print(f"Register: {mpu_validation['register']}")
    print(f"Address: {mpu_validation['address']}")
    print(f"Expected Value: {mpu_validation['expected_value']}")
    print(f"Calculation: {mpu_validation['calculation']}")
    print(f"Source Operations: {len(mpu_validation['source_operations'])}")
    
    # Export register database
    validator.export_register_database("validation_framework/register_definitions.json")
    
    # Test validation against known incorrect value
    print("\n🧪 Testing Validation Against Captured Value:")
    test_result = validator.validate_against_captured_value('MPU_CTRL', 'ARM_MPU_Enable', 0x00000000)
    print(f"Validation Status: {test_result['status']}")
    print(f"Expected: {test_result['expected_value']}")
    print(f"Captured: {test_result['captured_value']}")
    print(f"Validation Passed: {test_result['validation_passed']}")
    
    if not test_result['validation_passed']:
        print(f"Error Details: {test_result['error_details']}")
    
    print("\n✅ Source Code Validator Test Complete")

if __name__ == "__main__":
    main()
