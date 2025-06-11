#!/usr/bin/env python3
"""
Example Usage of MIMXRT700 Peripheral Register Monitor
=====================================================

This script demonstrates how to use the peripheral register monitor
for common debugging scenarios with the MIMXRT700 platform.

Examples included:
1. Basic register monitoring during firmware execution
2. Clock configuration analysis
3. XSPI initialization monitoring
4. Pin configuration tracking
5. Custom analysis with specific peripherals

Usage:
    python example_monitor_usage.py
"""

import sys
import os
import time
from mimxrt700_peripheral_monitor import PeripheralMonitor

# Configuration - Update these values for your setup
JLINK_PROBE_SERIAL = "1065679149"  # Replace with your J-Link serial number
DEVICE_NAME = "MIMXRT798S_M33_CORE0"
FIRMWARE_ELF_GCC = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/debug/xspi_psram_polling_transfer_cm33_core0.elf"
FIRMWARE_ELF_CLANG = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/build_clang_debug/debug/xspi_psram_polling_transfer_cm33_core0.elf"
ANALYSIS_JSON = "complete_enhanced_peripheral_analysis.json"

def example_basic_monitoring():
    """Example 1: Basic register monitoring"""
    print("=" * 60)
    print("🔍 EXAMPLE 1: Basic Register Monitoring")
    print("=" * 60)
    
    try:
        with PeripheralMonitor(JLINK_PROBE_SERIAL, DEVICE_NAME) as monitor:
            # Load analysis data
            if not monitor.load_analysis_data(ANALYSIS_JSON):
                print("❌ Failed to load analysis data")
                return False
            
            # Connect to target
            if not monitor.connect():
                print("❌ Failed to connect to target")
                return False
            
            print("\n📊 Starting basic monitoring for 15 seconds...")
            print("   This will capture any register changes during execution")
            
            # Monitor for 15 seconds
            monitor.monitor_execution(duration=15)
            
            print("✅ Basic monitoring completed")
            return True
            
    except Exception as e:
        print(f"❌ Basic monitoring failed: {e}")
        return False

def example_firmware_load_and_monitor():
    """Example 2: Load firmware and monitor initialization"""
    print("\n" + "=" * 60)
    print("🔍 EXAMPLE 2: Firmware Load and Initialization Monitoring")
    print("=" * 60)
    
    if not os.path.exists(FIRMWARE_ELF):
        print(f"⚠️  Firmware file not found: {FIRMWARE_ELF}")
        print("   Skipping firmware load example")
        return True
    
    try:
        with PeripheralMonitor(JLINK_PROBE_SERIAL, DEVICE_NAME) as monitor:
            # Load analysis data
            if not monitor.load_analysis_data(ANALYSIS_JSON):
                return False
            
            # Connect to target
            if not monitor.connect():
                return False
            
            print("\n📥 Loading firmware...")
            if not monitor.load_firmware(FIRMWARE_ELF):
                return False
            
            print("\n▶️  Starting firmware execution...")
            if not monitor.start_execution():
                return False
            
            print("\n📊 Monitoring initialization sequence for 20 seconds...")
            print("   This captures board init, clock config, and driver init phases")
            
            # Monitor initialization
            monitor.monitor_execution(duration=20)
            
            print("✅ Firmware load and monitoring completed")
            return True
            
    except Exception as e:
        print(f"❌ Firmware load monitoring failed: {e}")
        return False

def example_targeted_peripheral_analysis():
    """Example 3: Focus on specific peripheral types"""
    print("\n" + "=" * 60)
    print("🔍 EXAMPLE 3: Targeted Peripheral Analysis")
    print("=" * 60)
    
    try:
        with PeripheralMonitor(JLINK_PROBE_SERIAL, DEVICE_NAME) as monitor:
            # Load analysis data
            if not monitor.load_analysis_data(ANALYSIS_JSON):
                return False
            
            # Connect to target
            if not monitor.connect():
                return False
            
            # Filter monitored addresses to focus on specific peripherals
            original_addresses = monitor.monitored_addresses.copy()
            
            # Focus on XSPI and CLKCTL0 registers only
            xspi_clkctl_addresses = set()
            for access in monitor.register_accesses:
                if access.peripheral_name in ["XSPI2", "CLKCTL0"] and 'write' in access.access_type:
                    xspi_clkctl_addresses.add(int(access.address, 16))
            
            monitor.monitored_addresses = xspi_clkctl_addresses
            
            print(f"\n🎯 Focusing on XSPI and CLKCTL0 peripherals")
            print(f"   Monitoring {len(monitor.monitored_addresses)} targeted addresses")
            print(f"   (Reduced from {len(original_addresses)} total addresses)")
            
            # Monitor targeted peripherals
            monitor.monitor_execution(duration=15)
            
            print("✅ Targeted peripheral analysis completed")
            return True
            
    except Exception as e:
        print(f"❌ Targeted analysis failed: {e}")
        return False

def example_register_value_tracking():
    """Example 4: Track specific register values over time"""
    print("\n" + "=" * 60)
    print("🔍 EXAMPLE 4: Register Value Tracking")
    print("=" * 60)
    
    try:
        with PeripheralMonitor(JLINK_PROBE_SERIAL, DEVICE_NAME) as monitor:
            # Load analysis data
            if not monitor.load_analysis_data(ANALYSIS_JSON):
                return False
            
            # Connect to target
            if not monitor.connect():
                return False
            
            # Define specific registers to track
            tracked_registers = {
                0x40411000: "XSPI2_MCR",      # XSPI2 Module Configuration
                0x40001434: "CLKCTL0_CLKSEL", # Clock Selection
                0x40001400: "CLKCTL0_CLKDIV", # Clock Divider
            }
            
            print(f"\n📈 Tracking specific register values:")
            for addr, name in tracked_registers.items():
                print(f"   0x{addr:08x}: {name}")
            
            # Read initial values
            print(f"\n📋 Initial register values:")
            initial_values = {}
            for addr, name in tracked_registers.items():
                value = monitor.read_register(addr)
                if value is not None:
                    initial_values[addr] = value
                    print(f"   {name}: {monitor.format_register_value(value)}")
            
            # Monitor for changes
            print(f"\n⏱️  Monitoring for 10 seconds...")
            start_time = time.time()
            
            while time.time() - start_time < 10:
                for addr, name in tracked_registers.items():
                    current_value = monitor.read_register(addr)
                    if current_value is not None and addr in initial_values:
                        if current_value != initial_values[addr]:
                            print(f"\n🔄 {name} changed:")
                            print(f"   Before: {monitor.format_register_value(initial_values[addr])}")
                            print(f"   After:  {monitor.format_register_value(current_value)}")
                            
                            # Analyze changes
                            changes = monitor.analyze_bit_changes(
                                initial_values[addr], current_value,
                                f"0x{addr:08x}", "Unknown", name
                            )
                            for change in changes:
                                print(f"   {change}")
                            
                            initial_values[addr] = current_value
                
                time.sleep(0.5)
            
            print("✅ Register value tracking completed")
            return True
            
    except Exception as e:
        print(f"❌ Register tracking failed: {e}")
        return False

def main():
    """Run all examples"""
    print("🚀 MIMXRT700 Peripheral Register Monitor Examples")
    print("=" * 60)
    print(f"Target Device: {DEVICE_NAME}")
    print(f"J-Link Probe: {JLINK_PROBE_SERIAL}")
    print(f"Analysis Data: {ANALYSIS_JSON}")
    if os.path.exists(FIRMWARE_ELF):
        print(f"Firmware: {FIRMWARE_ELF}")
    else:
        print(f"Firmware: {FIRMWARE_ELF} (not found - some examples will be skipped)")
    
    # Verify analysis file exists
    if not os.path.exists(ANALYSIS_JSON):
        print(f"\n❌ Analysis file not found: {ANALYSIS_JSON}")
        print("   Please ensure the peripheral analysis JSON file is present")
        return 1
    
    examples = [
        ("Basic Monitoring", example_basic_monitoring),
        ("Firmware Load & Monitor", example_firmware_load_and_monitor),
        ("Targeted Peripheral Analysis", example_targeted_peripheral_analysis),
        ("Register Value Tracking", example_register_value_tracking),
    ]
    
    results = []
    
    for name, example_func in examples:
        try:
            print(f"\n🎯 Running: {name}")
            success = example_func()
            results.append((name, success))
            
            if success:
                print(f"✅ {name} completed successfully")
            else:
                print(f"❌ {name} failed")
                
        except KeyboardInterrupt:
            print(f"\n⚠️  {name} interrupted by user")
            results.append((name, False))
            break
        except Exception as e:
            print(f"❌ {name} failed with exception: {e}")
            results.append((name, False))
    
    # Summary
    print("\n" + "=" * 60)
    print("📊 EXAMPLES SUMMARY")
    print("=" * 60)
    
    passed = 0
    for name, success in results:
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"   {name}: {status}")
        if success:
            passed += 1
    
    print(f"\nOverall: {passed}/{len(results)} examples completed successfully")
    
    if passed == len(results):
        print("🎉 All examples completed successfully!")
        return 0
    else:
        print("⚠️  Some examples failed - check hardware connections and configuration")
        return 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n⚠️  Examples interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Examples failed: {e}")
        sys.exit(1)
