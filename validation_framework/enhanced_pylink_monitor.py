#!/usr/bin/env python3
"""
Enhanced PyLink Hardware Monitor for Validation Framework

This module provides real-time hardware register monitoring with
validation against source code expectations and LLVM analysis results.

Critical Focus: Capture actual MPU_CTRL value and verify it's 0x00000007
"""

import os
import time
import json
import logging
from typing import Dict, List, Optional, Tuple
from pathlib import Path

try:
    import pylink
except ImportError:
    print("❌ PyLink not available. Install with: pip install pylink-square")
    pylink = None

from source_code_validator import SourceCodeValidator

class EnhancedPeripheralMonitor:
    """
    Enhanced peripheral monitor with real-time validation
    """
    
    def __init__(self, jlink_serial: str, device_name: str):
        self.jlink_serial = jlink_serial
        self.device_name = device_name
        self.jlink = None
        self.source_validator = None
        self.validation_results = []
        self.monitored_registers = {}
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Initialize source code validator
        project_root = "."
        cmsis_headers = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
        self.source_validator = SourceCodeValidator(project_root, cmsis_headers)
        
        # Define critical registers to monitor
        self._setup_critical_registers()
    
    def _setup_critical_registers(self):
        """Setup critical registers for monitoring"""
        self.monitored_registers = {
            0xE000ED94: {
                'name': 'MPU_CTRL',
                'peripheral': 'MPU',
                'expected_operations': {
                    'ARM_MPU_Enable': self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
                },
                'critical': True
            },
            0x40001434: {
                'name': 'CLKCTL0_CLKSEL',
                'peripheral': 'CLKCTL0',
                'critical': False
            },
            0x40411000: {
                'name': 'XSPI2_MCR',
                'peripheral': 'XSPI2',
                'critical': False
            }
        }
        
        self.logger.info(f"Setup monitoring for {len(self.monitored_registers)} critical registers")
    
    def connect(self) -> bool:
        """Connect to J-Link probe and target"""
        if pylink is None:
            self.logger.error("PyLink not available")
            return False
        
        try:
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=self.jlink_serial)
            
            self.logger.info(f"Connected to J-Link probe: {self.jlink_serial}")
            
            # Connect to target
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect(self.device_name)
            
            self.logger.info(f"Connected to target: {self.device_name}")
            
            # Halt target for initial setup
            self.jlink.halt()
            
            return True
            
        except Exception as e:
            self.logger.error(f"Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from J-Link probe"""
        if self.jlink:
            try:
                self.jlink.close()
                self.logger.info("Disconnected from J-Link probe")
            except Exception as e:
                self.logger.error(f"Error during disconnect: {e}")
    
    def capture_register_snapshot(self) -> Dict:
        """Capture current values of all monitored registers"""
        if not self.jlink:
            self.logger.error("J-Link not connected")
            return {}
        
        snapshot = {
            'timestamp': time.time(),
            'registers': {}
        }
        
        for address, reg_info in self.monitored_registers.items():
            try:
                # Read register value
                value = self.jlink.memory_read32(address, 1)[0]
                
                snapshot['registers'][f'0x{address:08x}'] = {
                    'name': reg_info['name'],
                    'peripheral': reg_info['peripheral'],
                    'value': f'0x{value:08x}',
                    'value_decimal': value,
                    'critical': reg_info.get('critical', False)
                }
                
                self.logger.debug(f"Read {reg_info['name']}: 0x{value:08x}")
                
            except Exception as e:
                self.logger.warning(f"Failed to read register {reg_info['name']} at 0x{address:08x}: {e}")
                snapshot['registers'][f'0x{address:08x}'] = {
                    'name': reg_info['name'],
                    'peripheral': reg_info['peripheral'],
                    'value': 'READ_ERROR',
                    'error': str(e)
                }
        
        return snapshot
    
    def validate_mpu_ctrl_register(self) -> Dict:
        """Specific validation for MPU_CTRL register"""
        self.logger.info("Validating MPU_CTRL register...")
        
        mpu_ctrl_address = 0xE000ED94
        expected_value = self.source_validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
        
        if not self.jlink:
            return {
                'status': 'ERROR',
                'message': 'J-Link not connected'
            }
        
        try:
            # Read current MPU_CTRL value
            current_value = self.jlink.memory_read32(mpu_ctrl_address, 1)[0]
            
            validation_result = {
                'register': 'MPU_CTRL',
                'address': f'0x{mpu_ctrl_address:08x}',
                'expected_value': f'0x{expected_value:08x}' if expected_value else 'UNKNOWN',
                'current_value': f'0x{current_value:08x}',
                'validation_passed': current_value == expected_value if expected_value else False,
                'timestamp': time.time()
            }
            
            if expected_value and current_value != expected_value:
                validation_result['error_details'] = {
                    'expected_decimal': expected_value,
                    'current_decimal': current_value,
                    'difference': current_value - expected_value,
                    'bit_analysis': self._analyze_bit_differences(expected_value, current_value)
                }
            
            self.validation_results.append(validation_result)
            
            self.logger.info(f"MPU_CTRL validation: {validation_result['validation_passed']}")
            self.logger.info(f"  Expected: {validation_result['expected_value']}")
            self.logger.info(f"  Current:  {validation_result['current_value']}")
            
            return validation_result
            
        except Exception as e:
            self.logger.error(f"Failed to validate MPU_CTRL: {e}")
            return {
                'status': 'ERROR',
                'message': f'Failed to read MPU_CTRL: {e}'
            }
    
    def _analyze_bit_differences(self, expected: int, actual: int) -> Dict:
        """Analyze bit-level differences between expected and actual values"""
        xor_result = expected ^ actual
        
        bit_analysis = {
            'xor_result': f'0x{xor_result:08x}',
            'different_bits': [],
            'expected_bits': f'{expected:032b}',
            'actual_bits': f'{actual:032b}'
        }
        
        for bit_pos in range(32):
            if (xor_result >> bit_pos) & 1:
                expected_bit = (expected >> bit_pos) & 1
                actual_bit = (actual >> bit_pos) & 1
                
                bit_analysis['different_bits'].append({
                    'position': bit_pos,
                    'expected': expected_bit,
                    'actual': actual_bit
                })
        
        return bit_analysis
    
    def monitor_register_writes(self, duration: float = 30.0) -> List[Dict]:
        """Monitor register writes for specified duration"""
        self.logger.info(f"Monitoring register writes for {duration} seconds...")
        
        if not self.jlink:
            self.logger.error("J-Link not connected")
            return []
        
        start_time = time.time()
        monitoring_results = []
        
        # Take initial snapshot
        initial_snapshot = self.capture_register_snapshot()
        monitoring_results.append({
            'type': 'initial_snapshot',
            'data': initial_snapshot
        })
        
        # Monitor for changes
        previous_values = {}
        for addr_str, reg_data in initial_snapshot['registers'].items():
            if 'value_decimal' in reg_data:
                previous_values[addr_str] = reg_data['value_decimal']
        
        while (time.time() - start_time) < duration:
            try:
                # Take current snapshot
                current_snapshot = self.capture_register_snapshot()
                
                # Check for changes
                for addr_str, reg_data in current_snapshot['registers'].items():
                    if 'value_decimal' in reg_data:
                        current_value = reg_data['value_decimal']
                        previous_value = previous_values.get(addr_str, 0)
                        
                        if current_value != previous_value:
                            change_event = {
                                'type': 'register_change',
                                'timestamp': current_snapshot['timestamp'],
                                'register': reg_data['name'],
                                'address': addr_str,
                                'previous_value': f'0x{previous_value:08x}',
                                'new_value': f'0x{current_value:08x}',
                                'peripheral': reg_data['peripheral']
                            }
                            
                            monitoring_results.append(change_event)
                            self.logger.info(f"Register change detected: {reg_data['name']} "
                                           f"{change_event['previous_value']} → {change_event['new_value']}")
                            
                            previous_values[addr_str] = current_value
                
                # Sleep briefly before next check
                time.sleep(0.1)
                
            except Exception as e:
                self.logger.error(f"Error during monitoring: {e}")
                break
        
        # Take final snapshot
        final_snapshot = self.capture_register_snapshot()
        monitoring_results.append({
            'type': 'final_snapshot',
            'data': final_snapshot
        })
        
        self.logger.info(f"Monitoring complete. Captured {len(monitoring_results)} events")
        return monitoring_results
    
    def run_comprehensive_validation(self) -> Dict:
        """Run comprehensive validation of all critical registers"""
        self.logger.info("Running comprehensive register validation...")
        
        validation_summary = {
            'timestamp': time.time(),
            'total_registers': len(self.monitored_registers),
            'validations': [],
            'critical_issues': [],
            'overall_status': 'UNKNOWN'
        }
        
        # Validate each critical register
        for address, reg_info in self.monitored_registers.items():
            if reg_info.get('critical', False):
                if reg_info['name'] == 'MPU_CTRL':
                    validation = self.validate_mpu_ctrl_register()
                    validation_summary['validations'].append(validation)
                    
                    if not validation.get('validation_passed', False):
                        validation_summary['critical_issues'].append({
                            'register': validation['register'],
                            'issue': 'Value mismatch with source code expectation',
                            'details': validation.get('error_details', {})
                        })
        
        # Determine overall status
        failed_validations = len(validation_summary['critical_issues'])
        if failed_validations == 0:
            validation_summary['overall_status'] = 'PASS'
        else:
            validation_summary['overall_status'] = 'FAIL'
        
        self.logger.info(f"Validation complete. Status: {validation_summary['overall_status']}")
        self.logger.info(f"Critical issues: {failed_validations}")
        
        return validation_summary
    
    def export_validation_results(self, output_file: str):
        """Export validation results to JSON file"""
        self.logger.info(f"Exporting validation results to {output_file}")
        
        export_data = {
            'validation_timestamp': time.time(),
            'jlink_serial': self.jlink_serial,
            'device_name': self.device_name,
            'monitored_registers': self.monitored_registers,
            'validation_results': self.validation_results
        }
        
        with open(output_file, 'w') as f:
            json.dump(export_data, f, indent=2)

def main():
    """Main function for testing the enhanced PyLink monitor"""
    print("🔍 Enhanced PyLink Monitor for Validation Framework")
    print("=" * 60)
    
    # Configuration
    JLINK_SERIAL = "1065679149"
    DEVICE_NAME = "MIMXRT798S_M33_CORE0"
    
    if pylink is None:
        print("❌ PyLink not available. Please install pylink-square")
        return
    
    # Initialize monitor
    monitor = EnhancedPeripheralMonitor(JLINK_SERIAL, DEVICE_NAME)
    
    try:
        # Connect to hardware
        print(f"\n🔌 Connecting to J-Link {JLINK_SERIAL}...")
        if not monitor.connect():
            print("❌ Failed to connect to hardware")
            return
        
        print("✅ Connected to hardware")
        
        # Run comprehensive validation
        print("\n🧪 Running Comprehensive Validation...")
        validation_results = monitor.run_comprehensive_validation()
        
        print(f"Overall Status: {validation_results['overall_status']}")
        print(f"Total Registers: {validation_results['total_registers']}")
        print(f"Critical Issues: {len(validation_results['critical_issues'])}")
        
        for issue in validation_results['critical_issues']:
            print(f"  ❌ {issue['register']}: {issue['issue']}")
        
        # Export results
        monitor.export_validation_results("validation_framework/hardware_validation_results.json")
        
        print("\n✅ Enhanced PyLink Monitor Test Complete")
        
    except Exception as e:
        print(f"❌ Error during monitoring: {e}")
    
    finally:
        monitor.disconnect()

if __name__ == "__main__":
    main()
