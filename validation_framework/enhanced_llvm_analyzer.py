#!/usr/bin/env python3
"""
Enhanced LLVM Analysis Integration for Validation Framework

This module provides enhanced analysis of LLVM IR to extract actual
register values and validate them against source code expectations.

Critical Focus: Fix MPU_CTRL value capture from 0x00000000 to 0x00000007
"""

import os
import json
import logging
import subprocess
from typing import Dict, List, Optional, Tuple
from pathlib import Path
from source_code_validator import SourceCodeValidator

class EnhancedLLVMAnalyzer:
    """
    Enhanced LLVM analysis with accurate value capture and validation
    """
    
    def __init__(self, project_root: str, llvm_build_dir: str):
        self.project_root = Path(project_root)
        self.llvm_build_dir = Path(llvm_build_dir)
        self.source_validator = None
        self.captured_values = {}
        self.analysis_results = {}
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Initialize source code validator
        cmsis_headers = self.project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
        self.source_validator = SourceCodeValidator(str(self.project_root), str(cmsis_headers))
    
    def analyze_llvm_ir_files(self, ir_files: List[str]) -> Dict:
        """Analyze LLVM IR files for register access patterns"""
        self.logger.info(f"Analyzing {len(ir_files)} LLVM IR files...")
        
        all_results = []
        
        for ir_file in ir_files:
            ir_path = Path(ir_file)
            if not ir_path.exists():
                self.logger.warning(f"IR file not found: {ir_file}")
                continue
            
            self.logger.info(f"Processing IR file: {ir_file}")
            file_results = self._analyze_single_ir_file(ir_path)
            all_results.extend(file_results)
        
        # Sort by sequence number for chronological order
        all_results.sort(key=lambda x: x.get('sequence_number', 0))
        
        return {
            'total_accesses': len(all_results),
            'ir_files_processed': len(ir_files),
            'register_accesses': all_results
        }
    
    def _analyze_single_ir_file(self, ir_file: Path) -> List[Dict]:
        """Analyze a single LLVM IR file for register accesses"""
        self.logger.info(f"Analyzing IR file: {ir_file}")
        
        try:
            with open(ir_file, 'r') as f:
                ir_content = f.read()
        except Exception as e:
            self.logger.error(f"Failed to read IR file {ir_file}: {e}")
            return []
        
        register_accesses = []
        
        # Look for store instructions to peripheral addresses
        store_patterns = self._find_store_instructions(ir_content)
        for pattern in store_patterns:
            access = self._analyze_store_instruction(pattern, ir_content)
            if access:
                register_accesses.append(access)
        
        # Look for function calls that affect registers
        call_patterns = self._find_function_calls(ir_content)
        for pattern in call_patterns:
            access = self._analyze_function_call(pattern, ir_content)
            if access:
                register_accesses.append(access)
        
        return register_accesses
    
    def _find_store_instructions(self, ir_content: str) -> List[Dict]:
        """Find store instructions in LLVM IR"""
        import re
        
        # Pattern for store instructions with immediate values
        store_pattern = r'store\s+i(\d+)\s+(\d+|0x[0-9a-fA-F]+),\s+i\d+\*\s+inttoptr\s+\(i\d+\s+(0x[0-9a-fA-F]+)\s+to\s+i\d+\*\)'
        
        matches = []
        for match in re.finditer(store_pattern, ir_content):
            width = int(match.group(1))
            value_str = match.group(2)
            address_str = match.group(3)
            
            # Convert value to integer
            if value_str.startswith('0x'):
                value = int(value_str, 16)
            else:
                value = int(value_str)
            
            # Convert address to integer
            address = int(address_str, 16)
            
            matches.append({
                'type': 'store',
                'width': width,
                'value': value,
                'address': address,
                'raw_match': match.group(0)
            })
        
        return matches
    
    def _find_function_calls(self, ir_content: str) -> List[Dict]:
        """Find function calls that might affect registers"""
        import re
        
        # Pattern for function calls
        call_pattern = r'call\s+[^@]*@([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)'
        
        matches = []
        for match in re.finditer(call_pattern, ir_content):
            function_name = match.group(1)
            
            # Focus on functions that affect registers
            if any(keyword in function_name for keyword in ['MPU', 'Enable', 'Config', 'Init']):
                matches.append({
                    'type': 'function_call',
                    'function_name': function_name,
                    'raw_match': match.group(0)
                })
        
        return matches
    
    def _analyze_store_instruction(self, pattern: Dict, ir_content: str) -> Optional[Dict]:
        """Analyze a store instruction pattern"""
        address = pattern['address']
        value = pattern['value']
        
        # Check if this is a peripheral register address
        if not self._is_peripheral_address(address):
            return None
        
        # Identify the peripheral and register
        peripheral_name, register_name = self._identify_peripheral_register(address)
        
        if not peripheral_name:
            return None
        
        access = {
            'access_type': 'store_instruction',
            'address': f'0x{address:08x}',
            'peripheral_name': peripheral_name,
            'register_name': register_name,
            'value_written': f'0x{value:08x}',
            'data_width': pattern['width'],
            'llvm_instruction': pattern['raw_match']
        }
        
        # Validate against expected values
        if peripheral_name == 'MPU' and register_name == 'CTRL':
            expected_value = self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
            if expected_value is not None:
                access['expected_value'] = f'0x{expected_value:08x}'
                access['validation_passed'] = (value == expected_value)
                
                if not access['validation_passed']:
                    self.logger.warning(f"MPU_CTRL value mismatch: expected 0x{expected_value:08x}, found 0x{value:08x}")
        
        return access
    
    def _analyze_function_call(self, pattern: Dict, ir_content: str) -> Optional[Dict]:
        """Analyze a function call pattern"""
        function_name = pattern['function_name']
        
        # Handle ARM_MPU_Enable specifically
        if 'ARM_MPU_Enable' in function_name or 'MPU_Enable' in function_name:
            # This function writes to MPU_CTRL register
            expected_value = self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
            
            access = {
                'access_type': 'function_call',
                'address': '0xe000ed94',  # MPU_CTRL address
                'peripheral_name': 'MPU',
                'register_name': 'CTRL',
                'function_called': function_name,
                'expected_value': f'0x{expected_value:08x}' if expected_value else 'UNKNOWN',
                'llvm_instruction': pattern['raw_match']
            }
            
            return access
        
        return None
    
    def _is_peripheral_address(self, address: int) -> bool:
        """Check if address is in peripheral space"""
        # ARM Cortex-M peripheral address ranges
        peripheral_ranges = [
            (0x40000000, 0x5FFFFFFF),  # Peripheral space
            (0xE0000000, 0xE00FFFFF),  # Private peripheral bus
        ]
        
        for start, end in peripheral_ranges:
            if start <= address <= end:
                return True
        
        return False
    
    def _identify_peripheral_register(self, address: int) -> Tuple[str, str]:
        """Identify peripheral and register name from address"""
        # Known register addresses
        register_map = {
            0xE000ED94: ('MPU', 'CTRL'),
            0xE000ED98: ('MPU', 'RNR'),
            0xE000ED9C: ('MPU', 'RBAR'),
            0xE000EDA0: ('MPU', 'RLAR'),
        }
        
        if address in register_map:
            return register_map[address]
        
        # Try to identify based on address ranges
        if 0x40000000 <= address <= 0x4FFFFFFF:
            return ('UNKNOWN_PERIPHERAL', f'REG_0x{address:08x}')
        elif 0xE000E000 <= address <= 0xE000EFFF:
            return ('SYSTEM_CONTROL', f'REG_0x{address:08x}')
        
        return ('UNKNOWN', f'REG_0x{address:08x}')
    
    def run_enhanced_analysis(self, ir_directory: str) -> Dict:
        """Run enhanced analysis on all IR files in directory"""
        self.logger.info(f"Running enhanced analysis on IR directory: {ir_directory}")
        
        ir_dir = Path(ir_directory)
        if not ir_dir.exists():
            self.logger.error(f"IR directory not found: {ir_directory}")
            return {}
        
        # Find all .ll files
        ir_files = list(ir_dir.glob("*.ll"))
        if not ir_files:
            self.logger.warning(f"No .ll files found in {ir_directory}")
            return {}
        
        self.logger.info(f"Found {len(ir_files)} IR files")
        
        # Analyze all IR files
        analysis_results = self.analyze_llvm_ir_files([str(f) for f in ir_files])
        
        # Generate validation report
        validation_report = self._generate_validation_report(analysis_results)
        
        return {
            'analysis_results': analysis_results,
            'validation_report': validation_report
        }
    
    def _generate_validation_report(self, analysis_results: Dict) -> Dict:
        """Generate validation report comparing LLVM results with expectations"""
        register_accesses = analysis_results.get('register_accesses', [])
        
        validation_summary = {
            'total_accesses': len(register_accesses),
            'validated_accesses': 0,
            'failed_validations': 0,
            'critical_issues': []
        }
        
        for access in register_accesses:
            if 'expected_value' in access and 'value_written' in access:
                validation_summary['validated_accesses'] += 1
                
                if not access.get('validation_passed', True):
                    validation_summary['failed_validations'] += 1
                    
                    # Check for critical MPU_CTRL issue
                    if (access.get('peripheral_name') == 'MPU' and 
                        access.get('register_name') == 'CTRL'):
                        validation_summary['critical_issues'].append({
                            'register': 'MPU_CTRL',
                            'expected': access.get('expected_value'),
                            'captured': access.get('value_written'),
                            'issue': 'Critical MPU configuration mismatch'
                        })
        
        return validation_summary
    
    def export_enhanced_results(self, output_file: str, results: Dict):
        """Export enhanced analysis results to JSON"""
        self.logger.info(f"Exporting enhanced results to {output_file}")
        
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2)

def main():
    """Main function for testing the enhanced LLVM analyzer"""
    print("🔍 Enhanced LLVM Analyzer for Validation Framework")
    print("=" * 60)
    
    # Initialize analyzer
    project_root = "."
    llvm_build_dir = "llvm_analysis_pass/build"
    
    analyzer = EnhancedLLVMAnalyzer(project_root, llvm_build_dir)
    
    # Test with existing IR files
    ir_directory = "llvm_ir_files"
    if Path(ir_directory).exists():
        print(f"\n📊 Running Enhanced Analysis on {ir_directory}")
        results = analyzer.run_enhanced_analysis(ir_directory)
        
        if results:
            print(f"Total Accesses Found: {results['analysis_results']['total_accesses']}")
            print(f"Validation Summary: {results['validation_report']}")
            
            # Export results
            analyzer.export_enhanced_results("validation_framework/enhanced_llvm_analysis.json", results)
        else:
            print("❌ No analysis results generated")
    else:
        print(f"⚠️  IR directory not found: {ir_directory}")
        print("   Please run LLVM analysis pass first to generate IR files")
    
    print("\n✅ Enhanced LLVM Analyzer Test Complete")

if __name__ == "__main__":
    main()
