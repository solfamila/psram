#!/usr/bin/env python3
"""
MIMXRT700 Initialization Register Capture
=========================================

This script captures register changes during the critical initialization
phase by resetting the target and immediately starting monitoring.
"""

import sys
import time
from mimxrt700_peripheral_monitor import PeripheralMonitor

def capture_initialization_registers(probe_serial: str, elf_file: str):
    """Capture register changes during initialization"""
    
    print("🚀 MIMXRT700 INITIALIZATION REGISTER CAPTURE")
    print("=" * 60)
    print(f"Probe: {probe_serial}")
    print(f"ELF: {elf_file}")
    
    try:
        with PeripheralMonitor(probe_serial, "MIMXRT798S_M33_CORE0") as monitor:
            
            # Load analysis data
            if not monitor.load_analysis_data("complete_enhanced_peripheral_analysis.json"):
                return False
            
            # Connect to target
            if not monitor.connect():
                return False
            
            # Load firmware
            print("\n📥 Loading firmware...")
            if not monitor.load_firmware(elf_file):
                return False
            
            # Target is now halted after firmware load
            print("\n🎯 STARTING INITIALIZATION CAPTURE")
            print("   Strategy: Reset + immediate monitoring")
            print("   Duration: 10 seconds")
            print("   Focus: First 10 seconds of execution")
            
            # Read baseline values of accessible registers
            accessible_registers = [
                0x40001434,  # CLKCTL0 CLKSEL
                0x40001400,  # CLKCTL0 CLKDIV  
                0x4000407C,  # IOPCTL0 PIO0_31
                0x40004080,  # IOPCTL0 PIO1_0
                0x40004030,  # IOPCTL0 PIO0_12
                0x40004034,  # IOPCTL0 PIO0_13
                0xE000ED9C,  # MPU RBAR
                0xE000ED94,  # MPU CTRL
                0x40100000,  # GPIO0 PDOR
                0x40411000,  # XSPI2 MCR
            ]
            
            print(f"\n📋 Reading baseline values for {len(accessible_registers)} accessible registers...")
            baseline_values = {}
            
            for addr in accessible_registers:
                try:
                    value = monitor.read_register(addr)
                    if value is not None:
                        baseline_values[addr] = value
                        print(f"   0x{addr:08x}: 0x{value:08x}")
                    else:
                        print(f"   0x{addr:08x}: Failed to read")
                except:
                    print(f"   0x{addr:08x}: Read error")
            
            print(f"\n▶️  Starting execution and monitoring...")
            
            # Start execution
            if not monitor.start_execution():
                return False
            
            # Monitor for changes with high frequency polling
            start_time = time.time()
            changes_detected = 0
            
            print(f"\n📊 MONITORING INITIALIZATION (10 seconds)")
            print("-" * 50)
            
            while time.time() - start_time < 10.0:
                current_time = time.time() - start_time
                
                # Check each accessible register for changes
                for addr in accessible_registers:
                    if addr in baseline_values:
                        try:
                            current_value = monitor.read_register(addr)
                            if current_value is not None and current_value != baseline_values[addr]:
                                
                                # Register changed!
                                changes_detected += 1
                                
                                # Get register info
                                reg_info = monitor.get_register_info(f"0x{addr:08x}", "Unknown", "Unknown")
                                
                                print(f"\n🔄 CHANGE #{changes_detected} at {current_time:.3f}s")
                                print(f"   Address: 0x{addr:08x}")
                                print(f"   Peripheral: {reg_info['peripheral']}")
                                print(f"   Register: {reg_info['register']}")
                                print(f"   Before: 0x{baseline_values[addr]:08x}")
                                print(f"   After:  0x{current_value:08x}")
                                
                                # Analyze bit changes
                                changes = monitor.analyze_bit_changes(
                                    baseline_values[addr], current_value,
                                    reg_info['peripheral'], reg_info['register']
                                )
                                
                                print(f"   📝 BIT CHANGES:")
                                for change in changes:
                                    print(f"      {change}")
                                
                                # Update baseline
                                baseline_values[addr] = current_value
                        
                        except Exception as e:
                            # Ignore read errors during monitoring
                            pass
                
                # Brief pause - balance between responsiveness and target impact
                time.sleep(0.05)  # 50ms polling interval
            
            elapsed = time.time() - start_time
            print(f"\n📊 INITIALIZATION CAPTURE SUMMARY")
            print("=" * 50)
            print(f"   Duration: {elapsed:.2f} seconds")
            print(f"   Register changes detected: {changes_detected}")
            print(f"   Accessible registers monitored: {len(baseline_values)}")
            
            if changes_detected == 0:
                print(f"\n💡 ANALYSIS:")
                print(f"   - Initialization completed before monitoring started")
                print(f"   - Try shorter delay between firmware load and monitoring")
                print(f"   - Consider using hardware breakpoints at reset vector")
                print(f"   - Monitor during actual reset sequence")
            
            return True
            
    except Exception as e:
        print(f"❌ Initialization capture failed: {e}")
        return False

def main():
    """Main function"""
    if len(sys.argv) != 3:
        print("Usage: python3 capture_initialization_registers.py <probe_serial> <elf_file>")
        return 1
    
    probe_serial = sys.argv[1]
    elf_file = sys.argv[2]
    
    success = capture_initialization_registers(probe_serial, elf_file)
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
