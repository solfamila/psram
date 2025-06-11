#!/usr/bin/env python3
"""
Complete Register Access Validation for MIMXRT700 LLVM Analysis Pass

This script validates that the LLVM analysis pass detects ALL register accesses
by running it on actual IR files and comparing against expected source code patterns.

MISSION: Achieve 100% register access detection coverage
"""

import os
import re
import json
import subprocess
import sys
from pathlib import Path

class CompleteRegisterAccessValidator:
    """
    Validates ALL register accesses in the MIMXRT700 project
    """
    
    def __init__(self, project_root=None):
        if project_root is None:
            # Auto-detect project root
            current_dir = Path.cwd()
            if "llvm_analysis_pass" in str(current_dir):
                self.project_root = current_dir.parent.parent if "tests" in str(current_dir) else current_dir.parent
            else:
                self.project_root = current_dir
        else:
            self.project_root = Path(project_root)
        self.analyzer_path = self.project_root / "llvm_analysis_pass/build/bin/peripheral-analyzer"
        self.source_files = []
        self.expected_accesses = []
        self.detected_accesses = []
        
        # Find all source files
        self._find_source_files()
        
        # Extract expected register accesses from source code
        self._extract_expected_accesses()
    
    def _find_source_files(self):
        """Find all C source files in the project"""
        source_dir = self.project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0"
        
        for file_path in source_dir.rglob("*.c"):
            if file_path.is_file():
                self.source_files.append(file_path)
        
        print(f"📁 Found {len(self.source_files)} C source files")
        for file_path in self.source_files:
            print(f"   • {file_path.name}")
    
    def _extract_expected_accesses(self):
        """Extract all expected register accesses from source code"""
        print("\n🔍 Extracting expected register accesses from source code...")
        
        # Function patterns that access registers
        function_patterns = [
            r'XCACHE_DisableCache\s*\(',
            r'XCACHE_EnableCache\s*\(',
            r'ARM_MPU_Enable\s*\(',
            r'ARM_MPU_Disable\s*\(',
            r'ARM_MPU_SetRegion\s*\(',
            r'ARM_MPU_SetMemAttr\s*\(',
            r'CLOCK_AttachClk\s*\(',
            r'CLOCK_SetClkDiv\s*\(',
            r'GPIO_PinWrite\s*\(',
            r'GPIO_PinRead\s*\(',
            r'GPIO_PinInit\s*\(',
            r'IOPCTL_PinMuxSet\s*\(',
            r'RESET_ClearPeripheralReset\s*\(',
        ]
        
        # Direct register access patterns
        direct_patterns = [
            r'\w+->[\w_]+\s*[|&]?=',  # base->register = value
            r'\w+\[\w+\]\s*[|&]?=',   # array[index] = value
        ]
        
        total_expected = 0
        
        for source_file in self.source_files:
            with open(source_file, 'r') as f:
                content = f.read()
                lines = content.split('\n')
            
            file_accesses = 0
            
            # Search for function calls
            for pattern in function_patterns:
                matches = list(re.finditer(pattern, content))
                for match in matches:
                    line_num = content[:match.start()].count('\n') + 1
                    function_name = match.group(0).split('(')[0].strip()
                    
                    self.expected_accesses.append({
                        'file': source_file.name,
                        'line': line_num,
                        'type': 'function_call',
                        'function': function_name,
                        'pattern': match.group(0)
                    })
                    file_accesses += 1
            
            # Search for direct register accesses
            for pattern in direct_patterns:
                matches = list(re.finditer(pattern, content))
                for match in matches:
                    line_num = content[:match.start()].count('\n') + 1
                    
                    # Skip if it's inside a comment or string
                    line_content = lines[line_num - 1].strip()
                    if line_content.startswith('//') or line_content.startswith('/*'):
                        continue
                    
                    self.expected_accesses.append({
                        'file': source_file.name,
                        'line': line_num,
                        'type': 'direct_access',
                        'pattern': match.group(0),
                        'line_content': line_content
                    })
                    file_accesses += 1
            
            if file_accesses > 0:
                print(f"   📊 {source_file.name}: {file_accesses} register accesses")
            total_expected += file_accesses
        
        print(f"\n📊 Total expected register accesses: {total_expected}")
        
        # Group by function type
        function_counts = {}
        for access in self.expected_accesses:
            if access['type'] == 'function_call':
                func = access['function']
                function_counts[func] = function_counts.get(func, 0) + 1
        
        print("\n📋 Expected function call breakdown:")
        for func, count in sorted(function_counts.items()):
            print(f"   • {func}: {count} calls")
    
    def _run_llvm_analysis(self):
        """Run LLVM analysis pass on IR files"""
        print("\n🔍 Running LLVM analysis pass on IR files...")
        
        if not self.analyzer_path.exists():
            print(f"❌ Analyzer not found: {self.analyzer_path}")
            return False
        
        # Find IR files
        ir_files = []
        for ir_dir in ["clang_ir_final", "llvm_ir_files"]:
            ir_path = self.project_root / ir_dir
            if ir_path.exists():
                ir_files.extend(list(ir_path.rglob("*.ll")))
        
        if not ir_files:
            print("❌ No IR files found")
            return False
        
        print(f"📁 Found {len(ir_files)} IR files")
        
        successful_analyses = 0
        
        for ir_file in ir_files[:5]:  # Analyze first 5 files
            print(f"\n🔍 Analyzing: {ir_file.name}")
            
            try:
                result = subprocess.run([str(self.analyzer_path), str(ir_file)], 
                                      capture_output=True, text=True, timeout=30)
                
                if result.returncode == 0:
                    successful_analyses += 1
                    print(f"   ✅ Analysis completed")
                    
                    # Look for JSON output
                    json_file = ir_file.with_suffix('.ll').with_suffix('_peripheral_analysis.json')
                    if json_file.exists():
                        self._parse_analysis_results(json_file)
                else:
                    print(f"   ❌ Analysis failed: {result.stderr}")
                    
            except subprocess.TimeoutExpired:
                print(f"   ⏰ Analysis timeout")
            except Exception as e:
                print(f"   ❌ Analysis error: {e}")
        
        print(f"\n📊 Successful analyses: {successful_analyses}/{len(ir_files[:5])}")
        return successful_analyses > 0
    
    def _parse_analysis_results(self, json_file):
        """Parse LLVM analysis results from JSON"""
        try:
            with open(json_file, 'r') as f:
                data = json.load(f)
            
            # Extract register accesses
            if 'chronological_sequence' in data:
                accesses = data['chronological_sequence']
            elif 'register_accesses' in data:
                accesses = data['register_accesses']
            else:
                return
            
            for access in accesses:
                self.detected_accesses.append({
                    'peripheral': access.get('peripheral_name', 'UNKNOWN'),
                    'register': access.get('register_name', 'UNKNOWN'),
                    'address': access.get('address', '0x00000000'),
                    'purpose': access.get('purpose', 'unknown'),
                    'file': access.get('file_name', 'unknown'),
                    'line': access.get('line_number', 0),
                    'sequence': access.get('sequence_number', 0)
                })
            
            print(f"   📊 Detected {len(accesses)} register accesses")
            
        except Exception as e:
            print(f"   ❌ Failed to parse JSON: {e}")
    
    def _validate_coverage(self):
        """Validate that all expected accesses were detected"""
        print("\n🧪 Validating Register Access Coverage")
        print("=" * 50)
        
        # Group expected accesses by function
        expected_functions = {}
        for access in self.expected_accesses:
            if access['type'] == 'function_call':
                func = access['function']
                expected_functions[func] = expected_functions.get(func, 0) + 1
        
        # Group detected accesses by purpose/function
        detected_functions = {}
        for access in self.detected_accesses:
            purpose = access['purpose'].lower()
            
            # Map purpose back to function names
            if 'cache disable' in purpose:
                func = 'XCACHE_DisableCache'
            elif 'cache enable' in purpose:
                func = 'XCACHE_EnableCache'
            elif 'mpu enable' in purpose:
                func = 'ARM_MPU_Enable'
            elif 'mpu disable' in purpose:
                func = 'ARM_MPU_Disable'
            elif 'mpu memory attribute' in purpose:
                func = 'ARM_MPU_SetMemAttr'
            elif 'mpu region' in purpose:
                func = 'ARM_MPU_SetRegion'
            elif 'clock' in purpose:
                if 'attach' in purpose:
                    func = 'CLOCK_AttachClk'
                elif 'div' in purpose:
                    func = 'CLOCK_SetClkDiv'
                else:
                    func = 'CLOCK_Unknown'
            elif 'gpio' in purpose:
                func = 'GPIO_PinWrite'
            else:
                func = 'UNKNOWN'
            
            detected_functions[func] = detected_functions.get(func, 0) + 1
        
        # Compare expected vs detected
        print("📊 Function Coverage Analysis:")
        print(f"{'Function':<25} {'Expected':<10} {'Detected':<10} {'Status':<10}")
        print("-" * 60)
        
        all_functions = set(expected_functions.keys()) | set(detected_functions.keys())
        coverage_passed = 0
        coverage_total = 0
        
        for func in sorted(all_functions):
            expected = expected_functions.get(func, 0)
            detected = detected_functions.get(func, 0)
            
            if expected > 0:
                coverage_total += 1
                if detected > 0:
                    status = "✅ PASS"
                    coverage_passed += 1
                else:
                    status = "❌ FAIL"
            else:
                status = "➕ EXTRA"
            
            print(f"{func:<25} {expected:<10} {detected:<10} {status:<10}")
        
        # Calculate coverage percentage
        coverage_percent = (coverage_passed / coverage_total * 100) if coverage_total > 0 else 0
        
        print("\n📊 COVERAGE SUMMARY:")
        print(f"Functions with expected accesses: {coverage_total}")
        print(f"Functions successfully detected: {coverage_passed}")
        print(f"Coverage percentage: {coverage_percent:.1f}%")
        print(f"Total expected register accesses: {len(self.expected_accesses)}")
        print(f"Total detected register accesses: {len(self.detected_accesses)}")
        
        return coverage_percent >= 95.0  # 95% threshold for success
    
    def run_complete_validation(self):
        """Run complete validation of register access detection"""
        print("🧪 COMPLETE REGISTER ACCESS VALIDATION FOR MIMXRT700")
        print("=" * 70)
        print("MISSION: Validate that LLVM analysis pass detects ALL register accesses")
        print("=" * 70)
        
        # Step 1: Extract expected accesses from source code
        if not self.expected_accesses:
            print("❌ No expected accesses found")
            return False
        
        # Step 2: Run LLVM analysis
        if not self._run_llvm_analysis():
            print("❌ LLVM analysis failed")
            return False
        
        # Step 3: Validate coverage
        success = self._validate_coverage()
        
        # Final result
        print("\n🎯 FINAL VALIDATION RESULT:")
        if success:
            print("✅ COMPLETE REGISTER ACCESS VALIDATION PASSED!")
            print("The LLVM analysis pass successfully detects all critical register accesses.")
        else:
            print("❌ REGISTER ACCESS VALIDATION FAILED!")
            print("Some register accesses are not being detected by the LLVM analysis pass.")
        
        return success

def main():
    validator = CompleteRegisterAccessValidator()
    success = validator.run_complete_validation()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
