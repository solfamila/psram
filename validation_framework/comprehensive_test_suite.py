#!/usr/bin/env python3
"""
Comprehensive Test Suite for MIMXRT700 Peripheral Monitoring Validation

This test suite validates the entire peripheral monitoring pipeline:
1. Source Code Analysis → 2. LLVM IR Analysis → 3. Hardware Monitoring → 4. Validation

Critical Focus: Ensure MPU_CTRL shows correct value 0x00000007 throughout the pipeline
"""

import os
import json
import time
import logging
import unittest
from typing import Dict, List, Optional
from pathlib import Path

# Import validation framework components
from source_code_validator import SourceCodeValidator
from enhanced_llvm_analyzer import EnhancedLLVMAnalyzer

try:
    from enhanced_pylink_monitor import EnhancedPeripheralMonitor
    PYLINK_AVAILABLE = True
except ImportError:
    PYLINK_AVAILABLE = False

class ValidationTestSuite:
    """
    Comprehensive test suite for peripheral monitoring validation
    """
    
    def __init__(self, project_root: str = ".", jlink_serial: str = "1065679149"):
        self.project_root = Path(project_root)
        self.jlink_serial = jlink_serial
        self.device_name = "MIMXRT798S_M33_CORE0"
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Initialize components
        self.source_validator = None
        self.llvm_analyzer = None
        self.hardware_monitor = None
        
        # Test results
        self.test_results = {
            'timestamp': time.time(),
            'tests_run': 0,
            'tests_passed': 0,
            'tests_failed': 0,
            'critical_failures': [],
            'detailed_results': []
        }
        
        self._initialize_components()
    
    def _initialize_components(self):
        """Initialize all validation framework components"""
        self.logger.info("Initializing validation framework components...")
        
        # Initialize source code validator
        cmsis_headers = self.project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
        self.source_validator = SourceCodeValidator(str(self.project_root), str(cmsis_headers))
        
        # Initialize LLVM analyzer
        llvm_build_dir = self.project_root / "llvm_analysis_pass/build"
        self.llvm_analyzer = EnhancedLLVMAnalyzer(str(self.project_root), str(llvm_build_dir))
        
        # Initialize hardware monitor (if PyLink available)
        if PYLINK_AVAILABLE:
            self.hardware_monitor = EnhancedPeripheralMonitor(self.jlink_serial, self.device_name)
        else:
            self.logger.warning("PyLink not available - hardware tests will be skipped")
    
    def run_test(self, test_name: str, test_function) -> bool:
        """Run a single test and record results"""
        self.logger.info(f"Running test: {test_name}")
        self.test_results['tests_run'] += 1
        
        try:
            result = test_function()
            
            if result.get('passed', False):
                self.test_results['tests_passed'] += 1
                self.logger.info(f"✅ {test_name}: PASSED")
            else:
                self.test_results['tests_failed'] += 1
                self.logger.error(f"❌ {test_name}: FAILED - {result.get('message', 'Unknown error')}")
                
                if result.get('critical', False):
                    self.test_results['critical_failures'].append({
                        'test_name': test_name,
                        'message': result.get('message', 'Unknown error'),
                        'details': result.get('details', {})
                    })
            
            self.test_results['detailed_results'].append({
                'test_name': test_name,
                'passed': result.get('passed', False),
                'message': result.get('message', ''),
                'details': result.get('details', {}),
                'timestamp': time.time()
            })
            
            return result.get('passed', False)
            
        except Exception as e:
            self.test_results['tests_failed'] += 1
            self.logger.error(f"❌ {test_name}: EXCEPTION - {e}")
            
            self.test_results['detailed_results'].append({
                'test_name': test_name,
                'passed': False,
                'message': f'Exception: {e}',
                'details': {},
                'timestamp': time.time()
            })
            
            return False
    
    def test_source_code_validation(self) -> Dict:
        """Test #1: Source code validation accuracy"""
        # Test CMSIS header parsing
        if not self.source_validator.register_definitions:
            return {
                'passed': False,
                'message': 'Failed to parse CMSIS headers',
                'critical': True
            }
        
        # Test MPU_CTRL expected value calculation
        expected_value = self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
        if expected_value != 7:  # 0x00000007
            return {
                'passed': False,
                'message': f'MPU_CTRL expected value incorrect: got {expected_value}, expected 7',
                'critical': True,
                'details': {'expected': 7, 'actual': expected_value}
            }
        
        # Test source file analysis
        mpu_validation = self.source_validator.validate_mpu_ctrl_operation()
        if not mpu_validation.get('source_operations'):
            return {
                'passed': False,
                'message': 'Failed to find ARM_MPU_Enable in source code',
                'critical': True
            }
        
        return {
            'passed': True,
            'message': 'Source code validation passed',
            'details': {
                'expected_mpu_ctrl_value': f'0x{expected_value:08x}',
                'source_operations_found': len(mpu_validation.get('source_operations', []))
            }
        }
    
    def test_llvm_analysis_accuracy(self) -> Dict:
        """Test #2: LLVM analysis value extraction accuracy"""
        # Check if IR files exist
        ir_directory = self.project_root / "llvm_ir_files"
        if not ir_directory.exists():
            return {
                'passed': False,
                'message': 'LLVM IR files not found - run LLVM analysis pass first',
                'critical': False
            }
        
        # Run enhanced LLVM analysis
        try:
            results = self.llvm_analyzer.run_enhanced_analysis(str(ir_directory))
            
            if not results:
                return {
                    'passed': False,
                    'message': 'LLVM analysis produced no results',
                    'critical': True
                }
            
            # Check for MPU_CTRL analysis
            register_accesses = results.get('analysis_results', {}).get('register_accesses', [])
            mpu_ctrl_found = False
            
            for access in register_accesses:
                if (access.get('peripheral_name') == 'MPU' and 
                    access.get('register_name') == 'CTRL'):
                    mpu_ctrl_found = True
                    
                    # Check if expected value is correct
                    expected_value = access.get('expected_value')
                    if expected_value != '0x00000007':
                        return {
                            'passed': False,
                            'message': f'LLVM analysis MPU_CTRL expected value incorrect: {expected_value}',
                            'critical': True
                        }
            
            if not mpu_ctrl_found:
                return {
                    'passed': False,
                    'message': 'MPU_CTRL register not found in LLVM analysis',
                    'critical': True
                }
            
            return {
                'passed': True,
                'message': 'LLVM analysis accuracy validated',
                'details': {
                    'total_accesses': len(register_accesses),
                    'mpu_ctrl_found': mpu_ctrl_found
                }
            }
            
        except Exception as e:
            return {
                'passed': False,
                'message': f'LLVM analysis failed: {e}',
                'critical': True
            }
    
    def test_hardware_monitoring_accuracy(self) -> Dict:
        """Test #3: Hardware monitoring accuracy"""
        if not PYLINK_AVAILABLE:
            return {
                'passed': False,
                'message': 'PyLink not available - hardware test skipped',
                'critical': False
            }
        
        try:
            # Connect to hardware
            if not self.hardware_monitor.connect():
                return {
                    'passed': False,
                    'message': 'Failed to connect to hardware',
                    'critical': False
                }
            
            # Validate MPU_CTRL register
            mpu_validation = self.hardware_monitor.validate_mpu_ctrl_register()
            
            if mpu_validation.get('status') == 'ERROR':
                return {
                    'passed': False,
                    'message': f'Hardware validation error: {mpu_validation.get("message")}',
                    'critical': True
                }
            
            # Check if validation passed
            validation_passed = mpu_validation.get('validation_passed', False)
            
            return {
                'passed': validation_passed,
                'message': 'Hardware monitoring accuracy validated' if validation_passed else 'MPU_CTRL value mismatch detected',
                'critical': not validation_passed,
                'details': {
                    'expected_value': mpu_validation.get('expected_value'),
                    'current_value': mpu_validation.get('current_value'),
                    'validation_passed': validation_passed
                }
            }
            
        except Exception as e:
            return {
                'passed': False,
                'message': f'Hardware monitoring test failed: {e}',
                'critical': True
            }
        
        finally:
            if self.hardware_monitor:
                self.hardware_monitor.disconnect()
    
    def test_end_to_end_correlation(self) -> Dict:
        """Test #4: End-to-end pipeline correlation"""
        # Get expected value from source code
        source_expected = self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
        
        # Get LLVM analysis results (if available)
        llvm_expected = None
        ir_directory = self.project_root / "llvm_ir_files"
        if ir_directory.exists():
            try:
                results = self.llvm_analyzer.run_enhanced_analysis(str(ir_directory))
                register_accesses = results.get('analysis_results', {}).get('register_accesses', [])
                
                for access in register_accesses:
                    if (access.get('peripheral_name') == 'MPU' and 
                        access.get('register_name') == 'CTRL'):
                        expected_str = access.get('expected_value', '0x00000000')
                        llvm_expected = int(expected_str, 16)
                        break
            except:
                pass
        
        # Get hardware value (if available)
        hardware_value = None
        if PYLINK_AVAILABLE:
            try:
                if self.hardware_monitor.connect():
                    mpu_validation = self.hardware_monitor.validate_mpu_ctrl_register()
                    current_str = mpu_validation.get('current_value', '0x00000000')
                    hardware_value = int(current_str, 16)
                    self.hardware_monitor.disconnect()
            except:
                pass
        
        # Check correlation
        correlation_results = {
            'source_expected': source_expected,
            'llvm_expected': llvm_expected,
            'hardware_value': hardware_value
        }
        
        # Determine if correlation is successful
        all_values = [v for v in correlation_results.values() if v is not None]
        correlation_passed = len(set(all_values)) <= 1 and source_expected in all_values
        
        return {
            'passed': correlation_passed,
            'message': 'End-to-end correlation validated' if correlation_passed else 'Pipeline correlation mismatch detected',
            'critical': not correlation_passed,
            'details': correlation_results
        }
    
    def run_all_tests(self) -> Dict:
        """Run all validation tests"""
        self.logger.info("🧪 Running Comprehensive Validation Test Suite")
        self.logger.info("=" * 60)
        
        # Define test sequence
        tests = [
            ("Source Code Validation", self.test_source_code_validation),
            ("LLVM Analysis Accuracy", self.test_llvm_analysis_accuracy),
            ("Hardware Monitoring Accuracy", self.test_hardware_monitoring_accuracy),
            ("End-to-End Correlation", self.test_end_to_end_correlation)
        ]
        
        # Run all tests
        for test_name, test_function in tests:
            self.run_test(test_name, test_function)
        
        # Generate summary
        self.test_results['success_rate'] = (
            self.test_results['tests_passed'] / self.test_results['tests_run'] * 100
            if self.test_results['tests_run'] > 0 else 0
        )
        
        self.test_results['overall_status'] = (
            'PASS' if len(self.test_results['critical_failures']) == 0 else 'CRITICAL_FAILURE'
        )
        
        # Log summary
        self.logger.info(f"\n📊 Test Suite Summary:")
        self.logger.info(f"Tests Run: {self.test_results['tests_run']}")
        self.logger.info(f"Tests Passed: {self.test_results['tests_passed']}")
        self.logger.info(f"Tests Failed: {self.test_results['tests_failed']}")
        self.logger.info(f"Success Rate: {self.test_results['success_rate']:.1f}%")
        self.logger.info(f"Overall Status: {self.test_results['overall_status']}")
        self.logger.info(f"Critical Failures: {len(self.test_results['critical_failures'])}")
        
        return self.test_results
    
    def export_test_results(self, output_file: str):
        """Export test results to JSON file"""
        self.logger.info(f"Exporting test results to {output_file}")
        
        with open(output_file, 'w') as f:
            json.dump(self.test_results, f, indent=2)

def main():
    """Main function for running the comprehensive test suite"""
    print("🧪 MIMXRT700 Peripheral Monitoring Validation Test Suite")
    print("=" * 70)
    
    # Initialize test suite
    test_suite = ValidationTestSuite()
    
    # Run all tests
    results = test_suite.run_all_tests()
    
    # Export results
    test_suite.export_test_results("validation_framework/comprehensive_test_results.json")
    
    # Print final status
    print(f"\n🎯 FINAL VALIDATION STATUS: {results['overall_status']}")
    
    if results['critical_failures']:
        print("\n❌ CRITICAL FAILURES DETECTED:")
        for failure in results['critical_failures']:
            print(f"  • {failure['test_name']}: {failure['message']}")
    else:
        print("\n✅ ALL CRITICAL VALIDATIONS PASSED")
    
    print(f"\nSuccess Rate: {results['success_rate']:.1f}%")
    print("Detailed results exported to: validation_framework/comprehensive_test_results.json")

if __name__ == "__main__":
    main()
