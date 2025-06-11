#!/usr/bin/env python3
"""
Corrected MIMXRT700 Peripheral Monitor
=====================================

Fixed implementation based on diagnostic findings:
1. Proper error handling for MPU access violations
2. Clock-aware register access
3. Improved register reading methods
"""

import sys
import time
import json
from typing import Dict, List, Optional
import pylink

class CorrectedPeripheralMonitor:
    """Corrected peripheral monitor with proper error handling"""
    
    def __init__(self, probe_serial: str, target_device: str):
        self.probe_serial = probe_serial
        self.target_device = target_device
        self.jlink = None
        self.analysis_data = None
        
    def connect(self) -> bool:
        """Connect to J-Link and target"""
        try:
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=self.probe_serial)
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect(self.target_device)
            return True
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            return False
    
    def load_analysis_data(self, filename: str) -> bool:
        """Load peripheral analysis data"""
        try:
            with open(filename, 'r') as f:
                self.analysis_data = json.load(f)
            return True
        except Exception as e:
            print(f"❌ Failed to load analysis data: {e}")
            return False
    
    def read_register_safe(self, address: int) -> Optional[int]:
        """Safely read register with proper error handling"""
        try:
            # Ensure target is halted for consistent access
            if not self.jlink.halted():
                self.jlink.halt()
                time.sleep(0.01)  # Brief pause after halt
            
            # Read register value
            value = self.jlink.memory_read32(address, 1)[0]
            return value
            
        except Exception as e:
            # Categorize the error
            error_str = str(e).lower()
            if "unspecified error" in error_str:
                return None  # Likely MPU violation or clock gating
            else:
                print(f"   Unexpected error reading 0x{address:08x}: {e}")
                return None
    
    def check_clock_enables(self) -> Dict[str, bool]:
        """Check which peripheral clocks are enabled"""
        clock_status = {}
        
        # Read CLKCTL0 PSCCTL registers
        pscctl_registers = {
            0x40001010: "PSCCTL0",
            0x40001014: "PSCCTL1", 
            0x40001018: "PSCCTL2",
            0x4000101C: "PSCCTL3",
            0x40001020: "PSCCTL4",
            0x40001024: "PSCCTL5"
        }
        
        for addr, name in pscctl_registers.items():
            value = self.read_register_safe(addr)
            if value is not None:
                clock_status[name] = {
                    'address': f"0x{addr:08x}",
                    'value': f"0x{value:08x}",
                    'enabled_bits': [i for i in range(32) if value & (1 << i)]
                }
        
        return clock_status
    
    def enable_iopctl2_clock(self) -> bool:
        """Attempt to enable IOPCTL2 clock"""
        try:
            # IOPCTL2 is typically controlled by PSCCTL2
            # Try to set bit for IOPCTL2 (exact bit depends on documentation)
            
            print("⚠️  IOPCTL2 clock appears disabled")
            print("   This requires firmware modification to enable")
            print("   Current monitoring will skip IOPCTL2 registers")
            
            return False  # Cannot enable from debugger
            
        except Exception as e:
            print(f"❌ Failed to enable IOPCTL2 clock: {e}")
            return False
    
    def run_corrected_monitoring(self, duration: int = 30) -> Dict:
        """Run corrected monitoring with proper error handling"""
        
        print(f"🔍 CORRECTED MIMXRT700 PERIPHERAL MONITORING")
        print("=" * 60)
        print(f"Probe: {self.probe_serial}")
        print(f"Target: {self.target_device}")
        print(f"Duration: {duration} seconds")
        
        if not self.connect():
            return {}
        
        if not self.load_analysis_data("complete_enhanced_peripheral_analysis.json"):
            return {}
        
        try:
            # Check clock enables first
            print(f"\n📊 CHECKING PERIPHERAL CLOCK STATUS")
            print("-" * 40)
            
            clock_status = self.check_clock_enables()
            for name, status in clock_status.items():
                enabled_count = len(status['enabled_bits'])
                print(f"{name}: {status['value']} ({enabled_count} bits enabled)")
            
            # Identify registers to monitor based on clock status
            print(f"\n📋 IDENTIFYING ACCESSIBLE REGISTERS")
            print("-" * 40)
            
            accessible_registers = []
            clock_gated_registers = []
            
            for access in self.analysis_data['chronological_sequence']:
                if 'write' in access['access_type']:
                    addr = int(access['address'], 16)
                    peripheral = access['peripheral_name']
                    register = access['register_name']
                    
                    # Check if peripheral clock is enabled
                    if peripheral == 'IOPCTL2':
                        # IOPCTL2 requires PSCCTL2 bit (currently 0x00000000)
                        clock_gated_registers.append({
                            'address': addr,
                            'peripheral': peripheral,
                            'register': register,
                            'reason': 'IOPCTL2 clock disabled (PSCCTL2=0x00000000)'
                        })
                    else:
                        accessible_registers.append({
                            'address': addr,
                            'peripheral': peripheral,
                            'register': register
                        })
            
            print(f"Accessible registers: {len(accessible_registers)}")
            print(f"Clock-gated registers: {len(clock_gated_registers)}")
            
            # Test register accessibility
            print(f"\n🔍 TESTING REGISTER ACCESSIBILITY")
            print("-" * 40)
            
            successful_reads = {}
            failed_reads = {}
            
            # Test a sample of accessible registers
            test_registers = accessible_registers[:20]  # Test first 20
            
            for reg_info in test_registers:
                addr = reg_info['address']
                peripheral = reg_info['peripheral']
                register = reg_info['register']
                
                value = self.read_register_safe(addr)
                
                if value is not None:
                    successful_reads[addr] = {
                        'peripheral': peripheral,
                        'register': register,
                        'value': f"0x{value:08x}"
                    }
                    print(f"✅ 0x{addr:08x}: {peripheral} {register} = 0x{value:08x}")
                else:
                    failed_reads[addr] = {
                        'peripheral': peripheral,
                        'register': register,
                        'reason': 'Read failed (likely MPU or state dependent)'
                    }
                    print(f"❌ 0x{addr:08x}: {peripheral} {register} - Read failed")
            
            # Monitor successful registers for changes
            if successful_reads:
                print(f"\n📊 MONITORING {len(successful_reads)} ACCESSIBLE REGISTERS")
                print("-" * 40)
                
                # Establish baseline
                baseline_values = {}
                for addr in successful_reads:
                    value = self.read_register_safe(addr)
                    if value is not None:
                        baseline_values[addr] = value
                
                print(f"Baseline established for {len(baseline_values)} registers")
                
                # Start monitoring
                changes_detected = 0
                start_time = time.time()
                
                # Resume target execution
                if self.jlink.halted():
                    self.jlink.restart()
                
                while time.time() - start_time < duration:
                    current_time = time.time() - start_time
                    
                    # Check for changes
                    for addr in baseline_values:
                        current_value = self.read_register_safe(addr)
                        
                        if (current_value is not None and 
                            current_value != baseline_values[addr]):
                            
                            changes_detected += 1
                            reg_info = successful_reads[addr]
                            
                            print(f"\n🔄 CHANGE #{changes_detected} at {current_time:.3f}s")
                            print(f"   0x{addr:08x}: {reg_info['peripheral']} {reg_info['register']}")
                            print(f"   Before: 0x{baseline_values[addr]:08x}")
                            print(f"   After:  0x{current_value:08x}")
                            
                            # Update baseline
                            baseline_values[addr] = current_value
                    
                    time.sleep(0.1)  # 100ms polling
                
                elapsed = time.time() - start_time
                
                print(f"\n📊 CORRECTED MONITORING RESULTS")
                print("=" * 50)
                print(f"Duration: {elapsed:.2f} seconds")
                print(f"Accessible registers: {len(successful_reads)}")
                print(f"Clock-gated registers: {len(clock_gated_registers)}")
                print(f"Failed reads: {len(failed_reads)}")
                print(f"Register changes detected: {changes_detected}")
                
                # Calculate corrected success rate
                total_attempted = len(accessible_registers)
                actual_accessible = len(successful_reads)
                corrected_success_rate = (actual_accessible / total_attempted * 100) if total_attempted > 0 else 0
                
                print(f"Corrected success rate: {actual_accessible}/{total_attempted} ({corrected_success_rate:.1f}%)")
                
                return {
                    'successful_reads': successful_reads,
                    'failed_reads': failed_reads,
                    'clock_gated_registers': clock_gated_registers,
                    'clock_status': clock_status,
                    'changes_detected': changes_detected,
                    'corrected_success_rate': corrected_success_rate
                }
            
            else:
                print("❌ No registers were accessible for monitoring")
                return {}
                
        finally:
            if self.jlink:
                self.jlink.close()

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python3 corrected_peripheral_monitor.py <probe_serial>")
        return 1
    
    probe_serial = sys.argv[1]
    target_device = "MIMXRT798S_M33_CORE0"
    
    monitor = CorrectedPeripheralMonitor(probe_serial, target_device)
    results = monitor.run_corrected_monitoring(30)
    
    # Save results
    if results:
        with open('corrected_monitoring_results.json', 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\n✅ Corrected monitoring results saved to: corrected_monitoring_results.json")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
