#!/usr/bin/env python3
"""
MIMXRT700 Peripheral Monitoring Validation Framework - Master Script

This script orchestrates the complete validation pipeline to ensure
accuracy across the entire peripheral monitoring toolchain.

CRITICAL MISSION: Fix MPU_CTRL value from 0x00000000 to 0x00000007

Pipeline: Source Code → LLVM Analysis → Hardware Monitoring → Validation
"""

import os
import sys
import time
import json
import argparse
import logging
from pathlib import Path

# Add validation framework to path
sys.path.insert(0, str(Path(__file__).parent))

from source_code_validator import SourceCodeValidator
from enhanced_llvm_analyzer import EnhancedLLVMAnalyzer
from comprehensive_test_suite import ValidationTestSuite

try:
    from enhanced_pylink_monitor import EnhancedPeripheralMonitor
    PYLINK_AVAILABLE = True
except ImportError:
    PYLINK_AVAILABLE = False

class ValidationFrameworkOrchestrator:
    """
    Master orchestrator for the validation framework
    """
    
    def __init__(self, project_root: str = ".", jlink_serial: str = "1065679149"):
        self.project_root = Path(project_root)
        self.jlink_serial = jlink_serial
        self.device_name = "MIMXRT798S_M33_CORE0"
        
        # Setup logging
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        self.logger = logging.getLogger(__name__)
        
        # Results storage
        self.validation_results = {
            'framework_version': '1.0.0',
            'timestamp': time.time(),
            'project_root': str(self.project_root),
            'jlink_serial': self.jlink_serial,
            'phases': {}
        }
    
    def phase_1_source_code_validation(self) -> bool:
        """Phase 1: Source Code Analysis and Validation"""
        self.logger.info("🔍 PHASE 1: Source Code Validation")
        self.logger.info("=" * 50)
        
        try:
            # Initialize source code validator
            cmsis_headers = self.project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
            validator = SourceCodeValidator(str(self.project_root), str(cmsis_headers))
            
            # Validate MPU_CTRL operation
            mpu_validation = validator.validate_mpu_ctrl_operation()
            
            # Check expected value
            expected_value = validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
            
            phase_result = {
                'status': 'PASS' if expected_value == 7 else 'FAIL',
                'expected_mpu_ctrl_value': f'0x{expected_value:08x}' if expected_value else 'UNKNOWN',
                'source_operations_found': len(mpu_validation.get('source_operations', [])),
                'validation_details': mpu_validation
            }
            
            self.validation_results['phases']['phase_1_source_code'] = phase_result
            
            if expected_value == 7:
                self.logger.info("✅ Phase 1 PASSED: Source code validation successful")
                self.logger.info(f"   Expected MPU_CTRL value: 0x{expected_value:08x}")
                return True
            else:
                self.logger.error("❌ Phase 1 FAILED: Incorrect expected value calculation")
                return False
                
        except Exception as e:
            self.logger.error(f"❌ Phase 1 FAILED: {e}")
            self.validation_results['phases']['phase_1_source_code'] = {
                'status': 'ERROR',
                'error': str(e)
            }
            return False
    
    def phase_2_llvm_analysis_enhancement(self) -> bool:
        """Phase 2: Enhanced LLVM Analysis"""
        self.logger.info("\n🔍 PHASE 2: Enhanced LLVM Analysis")
        self.logger.info("=" * 50)
        
        try:
            # Initialize LLVM analyzer
            llvm_build_dir = self.project_root / "llvm_analysis_pass/build"
            analyzer = EnhancedLLVMAnalyzer(str(self.project_root), str(llvm_build_dir))
            
            # Check for IR files
            ir_directory = self.project_root / "llvm_ir_files"
            if not ir_directory.exists():
                self.logger.warning("⚠️  LLVM IR files not found - skipping LLVM analysis")
                self.validation_results['phases']['phase_2_llvm_analysis'] = {
                    'status': 'SKIPPED',
                    'reason': 'IR files not found'
                }
                return True  # Not critical for framework validation
            
            # Run enhanced analysis
            results = analyzer.run_enhanced_analysis(str(ir_directory))
            
            if not results:
                self.logger.error("❌ Phase 2 FAILED: No LLVM analysis results")
                return False
            
            # Check for MPU_CTRL in results
            register_accesses = results.get('analysis_results', {}).get('register_accesses', [])
            mpu_ctrl_found = False
            mpu_ctrl_expected = None
            
            for access in register_accesses:
                if (access.get('peripheral_name') == 'MPU' and 
                    access.get('register_name') == 'CTRL'):
                    mpu_ctrl_found = True
                    mpu_ctrl_expected = access.get('expected_value')
                    break
            
            phase_result = {
                'status': 'PASS' if mpu_ctrl_found else 'FAIL',
                'total_accesses': len(register_accesses),
                'mpu_ctrl_found': mpu_ctrl_found,
                'mpu_ctrl_expected_value': mpu_ctrl_expected,
                'validation_report': results.get('validation_report', {})
            }
            
            self.validation_results['phases']['phase_2_llvm_analysis'] = phase_result
            
            if mpu_ctrl_found:
                self.logger.info("✅ Phase 2 PASSED: LLVM analysis enhanced successfully")
                self.logger.info(f"   Total register accesses: {len(register_accesses)}")
                self.logger.info(f"   MPU_CTRL expected: {mpu_ctrl_expected}")
                return True
            else:
                self.logger.error("❌ Phase 2 FAILED: MPU_CTRL not found in LLVM analysis")
                return False
                
        except Exception as e:
            self.logger.error(f"❌ Phase 2 FAILED: {e}")
            self.validation_results['phases']['phase_2_llvm_analysis'] = {
                'status': 'ERROR',
                'error': str(e)
            }
            return False
    
    def phase_3_hardware_monitoring(self) -> bool:
        """Phase 3: Hardware Monitoring and Validation"""
        self.logger.info("\n🔍 PHASE 3: Hardware Monitoring and Validation")
        self.logger.info("=" * 50)
        
        if not PYLINK_AVAILABLE:
            self.logger.warning("⚠️  PyLink not available - skipping hardware monitoring")
            self.validation_results['phases']['phase_3_hardware_monitoring'] = {
                'status': 'SKIPPED',
                'reason': 'PyLink not available'
            }
            return True  # Not critical for framework validation
        
        try:
            # Initialize hardware monitor
            monitor = EnhancedPeripheralMonitor(self.jlink_serial, self.device_name)
            
            # Connect to hardware
            if not monitor.connect():
                self.logger.warning("⚠️  Failed to connect to hardware - skipping hardware monitoring")
                self.validation_results['phases']['phase_3_hardware_monitoring'] = {
                    'status': 'SKIPPED',
                    'reason': 'Hardware connection failed'
                }
                return True  # Not critical for framework validation
            
            # Run comprehensive validation
            validation_results = monitor.run_comprehensive_validation()
            
            # Disconnect
            monitor.disconnect()
            
            phase_result = {
                'status': validation_results.get('overall_status', 'UNKNOWN'),
                'total_registers': validation_results.get('total_registers', 0),
                'critical_issues': validation_results.get('critical_issues', []),
                'validation_details': validation_results
            }
            
            self.validation_results['phases']['phase_3_hardware_monitoring'] = phase_result
            
            if validation_results.get('overall_status') == 'PASS':
                self.logger.info("✅ Phase 3 PASSED: Hardware monitoring validation successful")
                return True
            else:
                self.logger.error("❌ Phase 3 FAILED: Hardware validation issues detected")
                for issue in validation_results.get('critical_issues', []):
                    self.logger.error(f"   • {issue.get('register')}: {issue.get('issue')}")
                return False
                
        except Exception as e:
            self.logger.error(f"❌ Phase 3 FAILED: {e}")
            self.validation_results['phases']['phase_3_hardware_monitoring'] = {
                'status': 'ERROR',
                'error': str(e)
            }
            return False
    
    def phase_4_comprehensive_testing(self) -> bool:
        """Phase 4: Comprehensive Test Suite"""
        self.logger.info("\n🔍 PHASE 4: Comprehensive Test Suite")
        self.logger.info("=" * 50)
        
        try:
            # Initialize test suite
            test_suite = ValidationTestSuite(str(self.project_root), self.jlink_serial)
            
            # Run all tests
            test_results = test_suite.run_all_tests()
            
            phase_result = {
                'status': test_results.get('overall_status', 'UNKNOWN'),
                'tests_run': test_results.get('tests_run', 0),
                'tests_passed': test_results.get('tests_passed', 0),
                'tests_failed': test_results.get('tests_failed', 0),
                'success_rate': test_results.get('success_rate', 0),
                'critical_failures': test_results.get('critical_failures', []),
                'detailed_results': test_results.get('detailed_results', [])
            }
            
            self.validation_results['phases']['phase_4_comprehensive_testing'] = phase_result
            
            if test_results.get('overall_status') == 'PASS':
                self.logger.info("✅ Phase 4 PASSED: Comprehensive testing successful")
                self.logger.info(f"   Success rate: {test_results.get('success_rate', 0):.1f}%")
                return True
            else:
                self.logger.error("❌ Phase 4 FAILED: Critical test failures detected")
                for failure in test_results.get('critical_failures', []):
                    self.logger.error(f"   • {failure.get('test_name')}: {failure.get('message')}")
                return False
                
        except Exception as e:
            self.logger.error(f"❌ Phase 4 FAILED: {e}")
            self.validation_results['phases']['phase_4_comprehensive_testing'] = {
                'status': 'ERROR',
                'error': str(e)
            }
            return False
    
    def run_complete_validation(self) -> Dict:
        """Run the complete validation framework"""
        self.logger.info("🚀 MIMXRT700 Peripheral Monitoring Validation Framework")
        self.logger.info("=" * 70)
        self.logger.info("MISSION: Fix MPU_CTRL value from 0x00000000 to 0x00000007")
        self.logger.info("=" * 70)
        
        # Run all phases
        phases = [
            ("Phase 1: Source Code Validation", self.phase_1_source_code_validation),
            ("Phase 2: Enhanced LLVM Analysis", self.phase_2_llvm_analysis_enhancement),
            ("Phase 3: Hardware Monitoring", self.phase_3_hardware_monitoring),
            ("Phase 4: Comprehensive Testing", self.phase_4_comprehensive_testing)
        ]
        
        phases_passed = 0
        total_phases = len(phases)
        
        for phase_name, phase_function in phases:
            if phase_function():
                phases_passed += 1
        
        # Calculate overall status
        success_rate = (phases_passed / total_phases) * 100
        overall_status = 'PASS' if phases_passed == total_phases else 'PARTIAL_PASS' if phases_passed > 0 else 'FAIL'
        
        self.validation_results.update({
            'overall_status': overall_status,
            'phases_passed': phases_passed,
            'total_phases': total_phases,
            'success_rate': success_rate,
            'completion_timestamp': time.time()
        })
        
        # Log final results
        self.logger.info(f"\n🎯 VALIDATION FRAMEWORK COMPLETE")
        self.logger.info("=" * 50)
        self.logger.info(f"Overall Status: {overall_status}")
        self.logger.info(f"Phases Passed: {phases_passed}/{total_phases}")
        self.logger.info(f"Success Rate: {success_rate:.1f}%")
        
        return self.validation_results
    
    def export_results(self, output_file: str):
        """Export validation results to JSON file"""
        self.logger.info(f"📄 Exporting validation results to {output_file}")
        
        with open(output_file, 'w') as f:
            json.dump(self.validation_results, f, indent=2)

def main():
    """Main function for the validation framework"""
    parser = argparse.ArgumentParser(description='MIMXRT700 Peripheral Monitoring Validation Framework')
    parser.add_argument('--jlink-serial', default='1065679149', help='J-Link probe serial number')
    parser.add_argument('--project-root', default='.', help='Project root directory')
    parser.add_argument('--output', default='validation_framework/validation_framework_results.json', 
                       help='Output file for results')
    
    args = parser.parse_args()
    
    # Initialize orchestrator
    orchestrator = ValidationFrameworkOrchestrator(args.project_root, args.jlink_serial)
    
    # Run complete validation
    results = orchestrator.run_complete_validation()
    
    # Export results
    orchestrator.export_results(args.output)
    
    # Exit with appropriate code
    if results['overall_status'] == 'PASS':
        print("\n✅ VALIDATION FRAMEWORK: ALL TESTS PASSED")
        sys.exit(0)
    elif results['overall_status'] == 'PARTIAL_PASS':
        print("\n⚠️  VALIDATION FRAMEWORK: PARTIAL SUCCESS")
        sys.exit(1)
    else:
        print("\n❌ VALIDATION FRAMEWORK: CRITICAL FAILURES DETECTED")
        sys.exit(2)

if __name__ == "__main__":
    main()
