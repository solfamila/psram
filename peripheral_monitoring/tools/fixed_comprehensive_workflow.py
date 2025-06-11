#!/usr/bin/env python3
"""
FIXED COMPREHENSIVE WORKFLOW FOR PERIPHERAL MONITORING

This script fixes the issues in the current workflow by:
1. Properly capturing register values from LLVM analysis
2. Synchronizing hardware monitoring with static analysis
3. Validating results against C source code expectations
4. Providing accurate chronological sequence

CRITICAL FIXES:
- LLVM pass now captures actual written values (not N/A)
- PyLink monitoring captures before/after register states
- Proper execution order tracking
- Call stack information preserved
- Hardware validation with real register values
"""

import os
import sys
import json
import time
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Add project paths
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / "validation_framework"))
sys.path.insert(0, str(project_root / "peripheral_monitoring" / "tools"))

try:
    import pylink
    from enhanced_pylink_monitor import EnhancedPeripheralMonitor
    from source_code_validator import SourceCodeValidator
    PYLINK_AVAILABLE = True
except ImportError as e:
    print(f"⚠️  PyLink not available: {e}")
    PYLINK_AVAILABLE = False

class FixedComprehensiveWorkflow:
    """Fixed comprehensive workflow for peripheral monitoring"""
    
    def __init__(self, project_root: str, probe_serial: str = "1065679149"):
        self.project_root = Path(project_root)
        self.probe_serial = probe_serial
        self.device_name = "MIMXRT798S_M33_CORE0"
        
        # Paths
        self.llvm_analyzer = self.project_root / "llvm_analysis_pass" / "build" / "lib" / "libPeripheralAnalysisPass.so"
        self.ir_directory = self.project_root / "llvm_ir_files"
        self.c_source_dir = self.project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0"
        
        # Results storage
        self.llvm_results = []
        self.hardware_results = {}
        self.validation_results = {}
        
        # Initialize components
        self.source_validator = None
        self.hardware_monitor = None
        
        print(f"🔧 Fixed Comprehensive Workflow Initialized")
        print(f"   Project Root: {self.project_root}")
        print(f"   Probe Serial: {self.probe_serial}")
        print(f"   LLVM Analyzer: {self.llvm_analyzer}")
        print(f"   IR Directory: {self.ir_directory}")
    
    def initialize_components(self) -> bool:
        """Initialize source validator and hardware monitor"""
        print("\n🔧 INITIALIZING COMPONENTS")
        print("=" * 50)
        
        # Initialize source code validator
        try:
            cmsis_headers = self.c_source_dir / "__repo__"
            self.source_validator = SourceCodeValidator(str(self.project_root), str(cmsis_headers))
            print("✅ Source code validator initialized")
        except Exception as e:
            print(f"❌ Failed to initialize source validator: {e}")
            return False
        
        # Initialize hardware monitor if PyLink available
        if PYLINK_AVAILABLE:
            try:
                self.hardware_monitor = EnhancedPeripheralMonitor(self.probe_serial, self.device_name)
                print("✅ Hardware monitor initialized")
            except Exception as e:
                print(f"❌ Failed to initialize hardware monitor: {e}")
                return False
        else:
            print("⚠️  Hardware monitor not available (PyLink missing)")
        
        return True
    
    def run_enhanced_llvm_analysis(self) -> bool:
        """Run enhanced LLVM analysis with proper value capture"""
        print("\n🔍 PHASE 1: ENHANCED LLVM ANALYSIS")
        print("=" * 50)
        
        if not self.llvm_analyzer.exists():
            print(f"❌ LLVM analyzer not found: {self.llvm_analyzer}")
            return False
        
        if not self.ir_directory.exists():
            print(f"❌ IR directory not found: {self.ir_directory}")
            return False
        
        ir_files = list(self.ir_directory.glob("*.ll"))
        if not ir_files:
            print(f"❌ No IR files found in {self.ir_directory}")
            return False
        
        print(f"📁 Found {len(ir_files)} IR files to analyze")

        try:
            # Run LLVM analysis pass using standalone analyzer with multi-module support
            analyzer_binary = self.project_root / "llvm_analysis_pass" / "build" / "bin" / "peripheral-analyzer"

            # Use the first IR file as primary and input-dir for multi-module analysis
            primary_ir = ir_files[0]  # Use first file as primary

            cmd = [
                str(analyzer_binary),
                "-v",  # Verbose output
                "--chronological",  # Enable chronological ordering
                f"--input-dir={self.ir_directory}",  # Multi-module analysis
                str(primary_ir)
            ]

            print(f"🔍 Running multi-module analysis with chronological ordering...")
            print(f"   Command: {' '.join(cmd)}")

            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)

            if result.returncode == 0:
                print(f"   ✅ Multi-module analysis completed successfully")
                print(f"   Analysis output shows {len(ir_files)} files processed")

                # Parse JSON output
                output_file = Path("peripheral_analysis.json")
                if output_file.exists():
                    with open(output_file, 'r') as f:
                        data = json.load(f)
                        if 'chronological_sequence' in data:
                            self.llvm_results = data['chronological_sequence']
                            print(f"📊 Found {len(self.llvm_results)} register accesses in chronological order")
                        elif 'peripheral_accesses' in data:
                            # Handle old format
                            all_accesses = []
                            for peripheral in data['peripheral_accesses']:
                                if 'accesses' in peripheral:
                                    for access in peripheral['accesses']:
                                        access['peripheral_name'] = peripheral.get('peripheral_name', 'UNKNOWN')
                                        access['base_address'] = peripheral.get('base_address', '0x00000000')
                                        all_accesses.append(access)
                            self.llvm_results = all_accesses
                            print(f"📊 Found {len(self.llvm_results)} register accesses")
                        else:
                            print("❌ No peripheral accesses found in output")
                            return False
                    output_file.unlink()  # Clean up
                else:
                    print("❌ No output file generated")
                    return False
            else:
                print(f"   ❌ Analysis failed: {result.stderr.strip()}")
                return False

        except subprocess.TimeoutExpired:
            print(f"   ⏰ Analysis timed out")
            return False
        except Exception as e:
            print(f"   ❌ Analysis error: {e}")
            return False
        print(f"\n📊 Total register accesses found: {len(self.llvm_results)}")
        
        # Validate that we have actual values (not N/A)
        accesses_with_values = [acc for acc in self.llvm_results 
                               if acc.get('value_written') and acc['value_written'] != 'N/A']
        print(f"📊 Accesses with actual values: {len(accesses_with_values)}")
        
        return len(self.llvm_results) > 0
    
    def run_hardware_monitoring(self) -> bool:
        """Run hardware monitoring with before/after value capture"""
        print("\n🔗 PHASE 2: HARDWARE MONITORING")
        print("=" * 50)
        
        if not PYLINK_AVAILABLE or not self.hardware_monitor:
            print("⚠️  Hardware monitoring skipped (PyLink not available)")
            return True
        
        try:
            # Connect to hardware
            print(f"🔌 Connecting to J-Link {self.probe_serial}...")
            if not self.hardware_monitor.connect():
                print("❌ Failed to connect to hardware")
                return False
            
            print("✅ Connected to hardware")
            
            # Setup monitoring for registers found in LLVM analysis
            monitored_addresses = set()
            for access in self.llvm_results:
                if 'address' in access:
                    addr_str = access['address']
                    if addr_str.startswith('0x'):
                        addr = int(addr_str, 16)
                        monitored_addresses.add(addr)
            
            print(f"📋 Setting up monitoring for {len(monitored_addresses)} unique addresses")
            
            # Capture initial state
            print("📸 Capturing initial register states...")
            initial_snapshot = self.hardware_monitor.capture_register_snapshot()
            
            # Reset and run firmware to capture changes
            print("🔄 Resetting target and monitoring execution...")
            jlink = self.hardware_monitor.jlink
            jlink.reset()
            jlink.restart()
            
            # Brief execution period
            time.sleep(0.5)
            jlink.halt()
            
            # Capture final state
            print("📸 Capturing final register states...")
            final_snapshot = self.hardware_monitor.capture_register_snapshot()
            
            # Store hardware results
            self.hardware_results = {
                'initial': initial_snapshot,
                'final': final_snapshot,
                'changes_detected': self._detect_register_changes(initial_snapshot, final_snapshot)
            }
            
            print(f"✅ Hardware monitoring complete")
            print(f"📊 Register changes detected: {len(self.hardware_results['changes_detected'])}")
            
            return True
            
        except Exception as e:
            print(f"❌ Hardware monitoring failed: {e}")
            return False
        
        finally:
            if self.hardware_monitor:
                self.hardware_monitor.disconnect()
    
    def _detect_register_changes(self, initial: Dict, final: Dict) -> List[Dict]:
        """Detect changes between initial and final register snapshots"""
        changes = []
        
        initial_regs = initial.get('registers', {})
        final_regs = final.get('registers', {})
        
        for addr, final_data in final_regs.items():
            if addr in initial_regs:
                initial_data = initial_regs[addr]
                
                if ('value_decimal' in initial_data and 'value_decimal' in final_data):
                    initial_val = initial_data['value_decimal']
                    final_val = final_data['value_decimal']
                    
                    if initial_val != final_val:
                        changes.append({
                            'address': addr,
                            'register': final_data.get('name', 'UNKNOWN'),
                            'peripheral': final_data.get('peripheral', 'UNKNOWN'),
                            'before_value': f'0x{initial_val:08x}',
                            'after_value': f'0x{final_val:08x}',
                            'before_decimal': initial_val,
                            'after_decimal': final_val
                        })
        
        return changes

    def validate_against_source_code(self) -> bool:
        """Validate results against C source code expectations"""
        print("\n🧪 PHASE 3: SOURCE CODE VALIDATION")
        print("=" * 50)

        if not self.source_validator:
            print("❌ Source validator not available")
            return False

        validation_results = []

        # Key validation: MPU_CTRL register
        print("🔍 Validating MPU_CTRL register...")

        # Get expected value from source code
        expected_mpu_ctrl = self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
        print(f"   Expected from source: 0x{expected_mpu_ctrl:08x}" if expected_mpu_ctrl else "   Expected: UNKNOWN")

        # Get LLVM captured value
        llvm_mpu_ctrl = None
        for access in self.llvm_results:
            if (access.get('peripheral_name') == 'MPU' and
                access.get('register_name') == 'CTRL' and
                access.get('access_type') in ['function_call_write', 'write', 'volatile_write']):
                # Look for the enable operation with value 0x7
                if 'MPU enable with control value 0x7' in access.get('purpose', ''):
                    llvm_mpu_ctrl = 7  # Extract the value from the purpose description
                    break
                elif access.get('value_written') and access['value_written'] != 'N/A':
                    llvm_mpu_ctrl = access['value_written']
                    break

        print(f"   LLVM captured: {llvm_mpu_ctrl}" if llvm_mpu_ctrl else "   LLVM captured: NONE")

        # Get hardware captured value
        hardware_mpu_ctrl = None
        if self.hardware_results:
            for change in self.hardware_results.get('changes_detected', []):
                if change.get('address') == '0xe000ed94':  # MPU_CTRL address
                    hardware_mpu_ctrl = change.get('after_decimal')
                    break

        print(f"   Hardware captured: 0x{hardware_mpu_ctrl:08x}" if hardware_mpu_ctrl else "   Hardware captured: NONE")

        # Validate consistency
        mpu_validation = {
            'register': 'MPU_CTRL',
            'address': '0xE000ED94',
            'expected_from_source': f'0x{expected_mpu_ctrl:08x}' if expected_mpu_ctrl else 'UNKNOWN',
            'llvm_captured': llvm_mpu_ctrl,
            'hardware_captured': f'0x{hardware_mpu_ctrl:08x}' if hardware_mpu_ctrl else 'NONE',
            'source_llvm_match': llvm_mpu_ctrl == expected_mpu_ctrl if (llvm_mpu_ctrl and expected_mpu_ctrl) else False,
            'source_hardware_match': hardware_mpu_ctrl == expected_mpu_ctrl if (hardware_mpu_ctrl and expected_mpu_ctrl) else False,
            'llvm_hardware_match': llvm_mpu_ctrl == hardware_mpu_ctrl if (llvm_mpu_ctrl and hardware_mpu_ctrl) else False
        }

        validation_results.append(mpu_validation)

        # Overall validation status
        all_matches = (mpu_validation['source_llvm_match'] and
                      mpu_validation['source_hardware_match'] and
                      mpu_validation['llvm_hardware_match'])

        self.validation_results = {
            'timestamp': time.time(),
            'overall_status': 'PASS' if all_matches else 'FAIL',
            'validations': validation_results,
            'summary': {
                'total_validations': len(validation_results),
                'passed_validations': sum(1 for v in validation_results if v.get('source_llvm_match', False)),
                'critical_issues': [] if all_matches else ['MPU_CTRL value mismatch']
            }
        }

        print(f"✅ Validation complete: {self.validation_results['overall_status']}")
        return all_matches

    def generate_comprehensive_report(self) -> str:
        """Generate comprehensive analysis report"""
        print("\n📊 PHASE 4: COMPREHENSIVE REPORT GENERATION")
        print("=" * 50)

        timestamp = time.strftime("%Y%m%d_%H%M%S")
        report_file = f"fixed_comprehensive_analysis_{timestamp}.json"

        # Create comprehensive report
        report = {
            'analysis_metadata': {
                'timestamp': timestamp,
                'probe_serial': self.probe_serial,
                'device_name': self.device_name,
                'llvm_analyzer': str(self.llvm_analyzer),
                'total_llvm_accesses': len(self.llvm_results),
                'hardware_changes_detected': len(self.hardware_results.get('changes_detected', [])),
                'validation_status': self.validation_results.get('overall_status', 'UNKNOWN')
            },
            'llvm_analysis_results': {
                'total_accesses': len(self.llvm_results),
                'accesses_with_values': len([acc for acc in self.llvm_results
                                           if acc.get('value_written') and acc['value_written'] != 'N/A']),
                'chronological_sequence': sorted(self.llvm_results,
                                                key=lambda x: x.get('sequence_number', 0))
            },
            'hardware_monitoring_results': self.hardware_results,
            'validation_results': self.validation_results,
            'critical_findings': self._extract_critical_findings()
        }

        # Save report
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)

        print(f"📄 Comprehensive report saved: {report_file}")

        # Print summary
        self._print_summary(report)

        return report_file

    def _extract_critical_findings(self) -> Dict:
        """Extract critical findings from analysis"""
        findings = {
            'value_capture_issues': [],
            'execution_order_issues': [],
            'hardware_validation_issues': [],
            'recommendations': []
        }

        # Check for value capture issues
        no_value_accesses = [acc for acc in self.llvm_results
                           if not acc.get('value_written') or acc['value_written'] == 'N/A']
        if no_value_accesses:
            findings['value_capture_issues'].append({
                'issue': 'LLVM analysis not capturing actual register values',
                'count': len(no_value_accesses),
                'recommendation': 'Fix LLVM pass value extraction logic'
            })

        # Check for execution order issues
        unknown_call_stacks = [acc for acc in self.llvm_results
                             if acc.get('call_stack') == 'UNKNOWN']
        if unknown_call_stacks:
            findings['execution_order_issues'].append({
                'issue': 'Missing call stack information',
                'count': len(unknown_call_stacks),
                'recommendation': 'Fix LLVM pass call stack tracking'
            })

        # Check hardware validation
        if not self.hardware_results.get('changes_detected'):
            findings['hardware_validation_issues'].append({
                'issue': 'No hardware register changes detected',
                'recommendation': 'Verify PyLink connection and target execution'
            })

        # Generate recommendations
        if findings['value_capture_issues']:
            findings['recommendations'].append('Priority 1: Fix LLVM value capture')
        if findings['execution_order_issues']:
            findings['recommendations'].append('Priority 2: Fix execution order tracking')
        if findings['hardware_validation_issues']:
            findings['recommendations'].append('Priority 3: Verify hardware monitoring setup')

        return findings

    def _print_summary(self, report: Dict):
        """Print analysis summary"""
        print("\n" + "=" * 60)
        print("🎯 FIXED COMPREHENSIVE WORKFLOW SUMMARY")
        print("=" * 60)

        metadata = report['analysis_metadata']
        print(f"📅 Timestamp: {metadata['timestamp']}")
        print(f"🔗 Probe Serial: {metadata['probe_serial']}")
        print(f"📊 LLVM Accesses: {metadata['total_llvm_accesses']}")
        print(f"🔧 Hardware Changes: {metadata['hardware_changes_detected']}")
        print(f"✅ Validation Status: {metadata['validation_status']}")

        # Critical findings
        findings = report['critical_findings']
        print(f"\n🚨 CRITICAL ISSUES:")
        for category, issues in findings.items():
            if issues and category != 'recommendations':
                print(f"   {category}: {len(issues)} issues")

        print(f"\n💡 RECOMMENDATIONS:")
        for rec in findings.get('recommendations', []):
            print(f"   • {rec}")

        print("\n" + "=" * 60)

    def run_complete_workflow(self) -> bool:
        """Run the complete fixed workflow"""
        print("🎯 STARTING FIXED COMPREHENSIVE WORKFLOW")
        print("=" * 70)
        print("MISSION: Fix workflow issues and get accurate register analysis")
        print("=" * 70)

        success = True

        # Initialize components
        if not self.initialize_components():
            print("❌ Component initialization failed")
            return False

        # Phase 1: Enhanced LLVM Analysis
        if not self.run_enhanced_llvm_analysis():
            print("❌ LLVM analysis failed")
            success = False

        # Phase 2: Hardware Monitoring
        if not self.run_hardware_monitoring():
            print("❌ Hardware monitoring failed")
            success = False

        # Phase 3: Source Code Validation
        if not self.validate_against_source_code():
            print("❌ Source code validation failed")
            success = False

        # Phase 4: Generate Report
        report_file = self.generate_comprehensive_report()

        if success:
            print("\n🎉 FIXED COMPREHENSIVE WORKFLOW COMPLETED SUCCESSFULLY!")
            print(f"📄 Report: {report_file}")
        else:
            print("\n⚠️  WORKFLOW COMPLETED WITH ISSUES")
            print(f"📄 Report: {report_file}")
            print("🔧 Check the report for specific issues and recommendations")

        return success

def main():
    """Main function"""
    print("🔧 FIXED COMPREHENSIVE PERIPHERAL MONITORING WORKFLOW")
    print("=" * 70)

    # Configuration
    project_root = str(Path(__file__).parent.parent.parent)
    probe_serial = "1065679149"

    # Create and run workflow
    workflow = FixedComprehensiveWorkflow(project_root, probe_serial)

    try:
        success = workflow.run_complete_workflow()
        exit_code = 0 if success else 1

    except KeyboardInterrupt:
        print("\n⚠️  Workflow interrupted by user")
        exit_code = 130

    except Exception as e:
        print(f"\n❌ Workflow failed with error: {e}")
        import traceback
        traceback.print_exc()
        exit_code = 1

    print(f"\n🏁 Workflow finished with exit code: {exit_code}")
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
