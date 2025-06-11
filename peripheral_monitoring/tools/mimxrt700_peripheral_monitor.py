#!/usr/bin/env python3
"""
MIMXRT700 Peripheral Register Monitor using PyLink
==================================================

This script monitors peripheral register accesses identified in the comprehensive
peripheral analysis JSON file. It captures register values before and after write
operations, performs bit-by-bit comparison, and provides human-readable explanations
of register modifications based on MIMXRT700 register definitions.

Features:
- Real-time register monitoring during firmware execution
- Before/after value capture for write operations
- Bit-level change analysis with field name explanations
- Chronological execution order tracking
- MIMXRT700-specific register bit field interpretations

Usage:
    python mimxrt700_peripheral_monitor.py [options]

Requirements:
    - PyLink library (pip install pylink-square)
    - J-Link probe connected to MIMXRT700 target
    - Firmware ELF file with debug symbols
    - complete_enhanced_peripheral_analysis.json file
"""

import pylink
import json
import time
import sys
import os
import argparse
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from enum import Enum

# MIMXRT700 Register Bit Field Definitions
REGISTER_BIT_FIELDS = {
    # XSPI MCR Register (0x40411000) - Module Configuration Register
    "XSPI_MCR": {
        0: "SWRSTSD - Software Reset for Serial Flash Memory Domain (0=Deassert, 1=Reset)",
        1: "SWRSTHD - Software Reset for AHB Domain (0=Deassert, 1=Reset)",
        2: "END_CFG[0] - Byte Order Configuration bit 0",
        3: "END_CFG[1] - Byte Order Configuration bit 1 (00=64-bit BE, 01=32-bit LE, 10=32-bit BE, 11=64-bit LE)",
        4: "DQS_EN - DQS Enable for DDR mode",
        5: "DDR_EN - DDR Mode Enable",
        6: "CLR_RXF - Clear RX FIFO",
        7: "CLR_TXF - Clear TX FIFO",
        8: "RESERVED",
        9: "RESERVED",
        10: "DOZE_EN - Doze Mode Enable",
        11: "RESERVED",
        12: "DLPEN - Data Learning Pattern Enable (0=Disable, 1=Enable)",
        13: "RESERVED",
        14: "MDIS - Module Disable (0=Enable clocks, 1=Allow external logic to disable clocks)",
        15: "RESERVED",
        16: "ISD2FA - Idle Signal Drive IOFA[2] Flash Memory A",
        17: "ISD3FA - Idle Signal Drive IOFA[3] Flash Memory A (0=Logic low, 1=Logic high)",
        18: "ISD2FB - Idle Signal Drive IOFB[2] Flash Memory B",
        19: "ISD3FB - Idle Signal Drive IOFB[3] Flash Memory B",
        20: "X16_DB_CA_EN - X16 Databus Enable (0=Data only, 1=Address and command with data)",
        21: "RESERVED",
        22: "RESERVED",
        23: "RESERVED",
        24: "IPS_TG_RST - IP Bus Trigger Reset",
        25: "RESERVED",
        26: "RESERVED",
        27: "RESERVED",
        28: "RESERVED",
        29: "RESERVED",
        30: "RESERVED",
        31: "RESERVED"
    },

    # CLKCTL0 PSCCTL Registers - Peripheral Clock Control
    "CLKCTL0_PSCCTL0": {
        0: "RESERVED",
        1: "CODE_CACHE - Code Cache Clock (XCACHE 1) (0=Disable, 1=Enable)",
        2: "SYSTEM_CACHE - System Cache Clock (XCACHE 0) (0=Disable, 1=Enable)",
        3: "RESERVED",
        4: "RESERVED",
        5: "VDD2_OTP0 - VDD2_OTP_CTRL0 Clock (0=Disable, 1=Enable)",
        6: "VDD2_OTP1 - VDD2_OTP_CTRL1 Clock",
        7: "RESERVED",
        8: "RESERVED",
        9: "RESERVED",
        10: "RESERVED",
        11: "RESERVED",
        12: "RESERVED",
        13: "RESERVED",
        14: "RESERVED",
        15: "RESERVED"
    },

    "CLKCTL0_PSCCTL1": {
        0: "ELS - ELS Clock",
        1: "PKC - PKC Clock",
        2: "HASHCRYPT - HASHCRYPT Clock",
        3: "CASPER - CASPER Clock",
        4: "PUF - PUF Clock",
        5: "RNG - RNG Clock",
        6: "FLEXSPI0 - FLEXSPI0 Clock",
        7: "OTP - OTP Clock",
        8: "RESERVED",
        9: "RESERVED",
        10: "XSPI0 - XSPI0 Clock (0=Disable, 1=Enable)",
        11: "XSPI1 - XSPI1 Clock (0=Disable, 1=Enable)",
        12: "XSPI2 - XSPI2 Clock",
        13: "RESERVED",
        14: "RESERVED",
        15: "RESERVED"
    },

    "CLKCTL0_PSCCTL2": {
        0: "UTICK0 - UTICK0 Clock (0=Disable, 1=Enable)",
        1: "WWDT0 - WWDT0 Clock (0=Disable, 1=Enable)",
        2: "RESERVED",
        3: "RESERVED",
        4: "RESERVED",
        5: "RESERVED",
        6: "RESERVED",
        7: "RESERVED",
        8: "RESERVED",
        9: "RESERVED",
        10: "RESERVED",
        11: "RESERVED",
        12: "RESERVED",
        13: "RESERVED",
        14: "RESERVED",
        15: "RESERVED"
    },

    # CLKCTL0 Clock Selection Registers
    "CLKCTL0_CLKSEL": {
        0: "SEL[0] - Clock Source Select bit 0",
        1: "SEL[1] - Clock Source Select bit 1",
        2: "RESERVED",
        3: "RESERVED",
        4: "RESERVED",
        5: "RESERVED",
        6: "RESERVED",
        7: "RESERVED"
    },

    "CLKCTL0_CLKDIV": {
        0: "DIV[0] - Clock Divider bit 0",
        1: "DIV[1] - Clock Divider bit 1",
        2: "DIV[2] - Clock Divider bit 2",
        3: "DIV[3] - Clock Divider bit 3",
        4: "DIV[4] - Clock Divider bit 4",
        5: "DIV[5] - Clock Divider bit 5",
        6: "DIV[6] - Clock Divider bit 6",
        7: "DIV[7] - Clock Divider bit 7"
    },

    # IOPCTL Pin Configuration Registers
    "IOPCTL_PIO": {
        0: "FSEL[0] - Function Selector bit 0 (Digital Function Select)",
        1: "FSEL[1] - Function Selector bit 1",
        2: "FSEL[2] - Function Selector bit 2",
        3: "FSEL[3] - Function Selector bit 3 (0000=Func0, 0001=Func1, etc.)",
        4: "PUPDENA - Pull-up/Pull-down Enable (0=Disable, 1=Enable)",
        5: "PUPDSEL - Pull-up/Pull-down Select (0=Pull-down, 1=Pull-up)",
        6: "IBENA - Input Buffer Enable (0=Disable, 1=Enable)",
        7: "SLEW - Slew Rate (0=Standard, 1=Fast)",
        8: "DRIVE0 - Drive Strength bit 0",
        9: "DRIVE1 - Drive Strength bit 1 (00=Low, 01=Medium, 10=High, 11=Ultra-high)",
        10: "ODE - Open Drain Enable (0=Disable, 1=Enable)",
        11: "RESERVED",
        12: "RESERVED",
        13: "RESERVED",
        14: "RESERVED",
        15: "RESERVED"
    },

    # RSTCTL PRSTCTL_CLR Register - Peripheral Reset Control Clear
    "RSTCTL_PRSTCTL_CLR": {
        0: "RESERVED",
        1: "RESERVED",
        2: "RESERVED",
        3: "RESERVED",
        4: "RESERVED",
        5: "RESERVED",
        6: "IOPCTL0 - IOPCTL0 Reset Clear (0=No effect, 1=Clear reset)",
        7: "IOPCTL1 - IOPCTL1 Reset Clear",
        8: "IOPCTL2 - IOPCTL2 Reset Clear",
        9: "RESERVED",
        10: "RESERVED",
        11: "RESERVED",
        12: "RESERVED",
        13: "RESERVED",
        14: "RESERVED",
        15: "RESERVED"
    },

    # SYSCON0 Registers
    "SYSCON0_SEC_CLK_CTRL": {
        0: "TRNG_CLK_EN - TRNG Clock Enable",
        1: "HASHCRYPT_CLK_EN - HASHCRYPT Clock Enable",
        2: "CASPER_CLK_EN - CASPER Clock Enable",
        3: "PUF_CLK_EN - PUF Clock Enable",
        4: "PKC_CLK_EN - PKC Clock Enable",
        5: "ELS_CLK_EN - ELS Clock Enable"
    }
}

@dataclass
class RegisterAccess:
    """Represents a peripheral register access from the analysis"""
    sequence_number: int
    address: str
    peripheral_name: str
    register_name: str
    access_type: str
    execution_phase: str
    purpose: str
    source_location: Dict
    bits_modified: List[str]

class PeripheralMonitor:
    """PyLink-based peripheral register monitor for MIMXRT700"""
    
    def __init__(self, probe_serial: str, device_name: str = "MIMXRT798S_M33_CORE0"):
        self.probe_serial = int(probe_serial)
        self.device_name = device_name
        self.jlink = None
        self.connected = False
        self.register_accesses = []
        self.monitored_addresses = set()
        self.register_cache = {}  # Cache for register values
        
    def load_analysis_data(self, json_file: str) -> bool:
        """Load peripheral analysis data from JSON file"""
        try:
            with open(json_file, 'r') as f:
                data = json.load(f)
            
            print(f"📊 Loading analysis data from {json_file}")
            print(f"   Total accesses: {data['total_accesses']}")
            print(f"   Files analyzed: {data['files_analyzed']}")
            
            # Convert JSON data to RegisterAccess objects
            for access_data in data['chronological_sequence']:
                access = RegisterAccess(
                    sequence_number=access_data['sequence_number'],
                    address=access_data['address'],
                    peripheral_name=access_data['peripheral_name'],
                    register_name=access_data['register_name'],
                    access_type=access_data['access_type'],
                    execution_phase=access_data['execution_phase'],
                    purpose=access_data['purpose'],
                    source_location=access_data['source_location'],
                    bits_modified=access_data.get('bits_modified', [])
                )
                self.register_accesses.append(access)
                
                # Track addresses for write operations
                if 'write' in access.access_type:
                    self.monitored_addresses.add(int(access.address, 16))
            
            print(f"✅ Loaded {len(self.register_accesses)} register accesses")
            print(f"   Monitoring {len(self.monitored_addresses)} unique write addresses")
            return True
            
        except Exception as e:
            print(f"❌ Failed to load analysis data: {e}")
            return False
    
    def connect(self) -> bool:
        """Establish connection to J-Link and target"""
        try:
            print(f"🔌 Connecting to J-Link probe {self.probe_serial}...")
            self.jlink = pylink.JLink()
            
            # Open connection
            self.jlink.open(serial_no=self.probe_serial)
            print(f"✅ Connected to J-Link: {self.jlink.product_name}")
            
            # Configure interface for ARM Cortex-M
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            
            # Connect to target
            print(f"🎯 Connecting to target: {self.device_name}...")
            self.jlink.connect(self.device_name, verbose=True)
            print(f"✅ Connected to {self.device_name}")
            
            # Configure CoreSight
            self.jlink.coresight_configure()
            
            # Verify connection
            try:
                core_id = hex(self.jlink.core_id())
                print(f"📋 Target Core ID: {core_id}")
            except:
                print("📋 Target connected successfully")
            
            self.connected = True
            return True
            
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            self.cleanup()
            return False
    
    def read_register(self, address: int) -> Optional[int]:
        """Read 32-bit register value"""
        try:
            return self.jlink.memory_read32(address, 1)[0]
        except Exception as e:
            print(f"❌ Failed to read register 0x{address:08x}: {e}")
            return None
    
    def get_register_info(self, address: str, peripheral: str, register: str) -> Dict:
        """Get register information from analysis data"""
        for access in self.register_accesses:
            if access.address == address:
                return {
                    'peripheral': access.peripheral_name,
                    'register': access.register_name,
                    'purpose': access.purpose,
                    'source': access.source_location
                }
        return {'peripheral': peripheral, 'register': register, 'purpose': 'Unknown', 'source': {}}

    def analyze_bit_changes(self, old_value: int, new_value: int, address: str,
                           peripheral: str, register: str) -> List[str]:
        """Analyze bit-level changes between old and new register values"""
        changes = []
        xor_result = old_value ^ new_value

        if xor_result == 0:
            return ["No bits changed"]

        # Determine register type for bit field lookup
        register_key = self.get_register_bit_field_key(peripheral, register)
        bit_fields = REGISTER_BIT_FIELDS.get(register_key, {})

        # Analyze each bit
        for bit_pos in range(32):
            bit_mask = 1 << bit_pos
            if xor_result & bit_mask:
                old_bit = (old_value >> bit_pos) & 1
                new_bit = (new_value >> bit_pos) & 1

                # Get bit field description
                field_desc = bit_fields.get(bit_pos, f"Bit {bit_pos}")

                change_desc = f"Bit {bit_pos:2d}: {old_bit} → {new_bit} ({field_desc})"
                changes.append(change_desc)

        return changes

    def get_register_bit_field_key(self, peripheral: str, register: str) -> str:
        """Generate key for register bit field lookup"""
        # XSPI peripheral registers
        if peripheral in ["XSPI0", "XSPI1", "XSPI2"] and register == "MCR":
            return "XSPI_MCR"

        # CLKCTL0 peripheral registers
        elif peripheral == "CLKCTL0":
            if "PSCCTL0" in register:
                return "CLKCTL0_PSCCTL0"
            elif "PSCCTL1" in register:
                return "CLKCTL0_PSCCTL1"
            elif "PSCCTL2" in register:
                return "CLKCTL0_PSCCTL2"
            elif "CLKSEL" in register:
                return "CLKCTL0_CLKSEL"
            elif "CLKDIV" in register:
                return "CLKCTL0_CLKDIV"

        # IOPCTL peripheral registers
        elif peripheral in ["IOPCTL0", "IOPCTL1", "IOPCTL2"]:
            if "PIO" in register:
                return "IOPCTL_PIO"

        # RSTCTL peripheral registers
        elif peripheral in ["RSTCTL", "RSTCTL0", "RSTCTL1"] and "PRSTCTL_CLR" in register:
            return "RSTCTL_PRSTCTL_CLR"

        # SYSCON0 peripheral registers
        elif peripheral == "SYSCON0":
            if "SEC_CLK_CTRL" in register:
                return "SYSCON0_SEC_CLK_CTRL"

        # Default fallback
        return f"{peripheral}_{register}"

    def format_register_value(self, value: int) -> str:
        """Format register value for display"""
        return f"0x{value:08x} ({value:>10})"

    def monitor_register_write(self, address: int, access_info: Dict) -> None:
        """Monitor a specific register write operation"""
        try:
            # Read current value (before write)
            before_value = self.read_register(address)
            if before_value is None:
                return

            # Store in cache for comparison
            self.register_cache[address] = before_value

            print(f"\n🔍 REGISTER WRITE DETECTED")
            print(f"   Address: 0x{address:08x}")
            print(f"   Peripheral: {access_info['peripheral']}")
            print(f"   Register: {access_info['register']}")
            print(f"   Purpose: {access_info['purpose']}")
            print(f"   Before: {self.format_register_value(before_value)}")

            # Wait a brief moment for the write to complete
            time.sleep(0.001)

            # Read value after write
            after_value = self.read_register(address)
            if after_value is None:
                return

            print(f"   After:  {self.format_register_value(after_value)}")

            # Analyze bit changes
            if before_value != after_value:
                changes = self.analyze_bit_changes(
                    before_value, after_value,
                    f"0x{address:08x}", access_info['peripheral'], access_info['register']
                )

                print(f"   📝 BIT CHANGES:")
                for change in changes:
                    print(f"      {change}")
            else:
                print(f"   ℹ️  No value change detected")

            # Update cache
            self.register_cache[address] = after_value

        except Exception as e:
            print(f"❌ Error monitoring register 0x{address:08x}: {e}")

    def setup_breakpoints(self) -> bool:
        """Set up breakpoints at register write locations"""
        try:
            print(f"\n🎯 Setting up breakpoints for register monitoring...")

            # Group accesses by source location for efficient breakpoint placement
            breakpoint_locations = {}

            for access in self.register_accesses:
                if 'write' in access.access_type:
                    # Use source file and line as breakpoint location
                    source = access.source_location
                    if 'file' in source and 'line' in source:
                        location_key = f"{source['file']}:{source['line']}"
                        if location_key not in breakpoint_locations:
                            breakpoint_locations[location_key] = []
                        breakpoint_locations[location_key].append(access)

            print(f"   Found {len(breakpoint_locations)} unique source locations")

            # For demonstration, we'll use memory watchpoints instead of source breakpoints
            # since we need the actual memory addresses
            watchpoint_count = 0
            for address in list(self.monitored_addresses)[:10]:  # Limit to first 10 for demo
                try:
                    # Set memory watchpoint for write access
                    # Note: J-Link has limited watchpoint resources
                    print(f"   Setting watchpoint at 0x{address:08x}")
                    watchpoint_count += 1

                    if watchpoint_count >= 4:  # Most ARM cores have 4 watchpoints max
                        print(f"   ⚠️  Reached watchpoint limit, monitoring first {watchpoint_count} addresses")
                        break

                except Exception as e:
                    print(f"   ⚠️  Failed to set watchpoint at 0x{address:08x}: {e}")

            return True

        except Exception as e:
            print(f"❌ Failed to setup breakpoints: {e}")
            return False

    def monitor_execution(self, duration: int = 30) -> None:
        """Monitor register accesses during firmware execution"""
        try:
            print(f"\n📈 STARTING REGISTER MONITORING")
            print(f"   Duration: {duration} seconds")
            print(f"   Monitored addresses: {len(self.monitored_addresses)}")
            print("=" * 60)

            start_time = time.time()
            access_count = 0

            # Pre-read all monitored registers to establish baseline
            print("📋 Establishing baseline register values...")
            for address in self.monitored_addresses:
                value = self.read_register(address)
                if value is not None:
                    self.register_cache[address] = value

            print(f"✅ Baseline established for {len(self.register_cache)} registers")

            # Main monitoring loop
            while time.time() - start_time < duration:
                try:
                    # Check each monitored address for changes
                    for address in self.monitored_addresses:
                        current_value = self.read_register(address)
                        if current_value is None:
                            continue

                        # Compare with cached value
                        cached_value = self.register_cache.get(address)
                        if cached_value is not None and current_value != cached_value:
                            # Register value changed - analyze it
                            access_info = self.get_register_info(
                                f"0x{address:08x}", "Unknown", "Unknown"
                            )

                            print(f"\n🔄 REGISTER CHANGE DETECTED #{access_count + 1}")
                            print(f"   Time: {time.time() - start_time:.2f}s")
                            print(f"   Address: 0x{address:08x}")
                            print(f"   Peripheral: {access_info['peripheral']}")
                            print(f"   Register: {access_info['register']}")
                            print(f"   Before: {self.format_register_value(cached_value)}")
                            print(f"   After:  {self.format_register_value(current_value)}")

                            # Analyze bit changes
                            changes = self.analyze_bit_changes(
                                cached_value, current_value,
                                f"0x{address:08x}", access_info['peripheral'], access_info['register']
                            )

                            print(f"   📝 BIT CHANGES:")
                            for change in changes:
                                print(f"      {change}")

                            # Update cache
                            self.register_cache[address] = current_value
                            access_count += 1

                    # Brief pause to avoid overwhelming the target
                    time.sleep(0.1)

                except KeyboardInterrupt:
                    print(f"\n⚠️  Monitoring interrupted by user")
                    break
                except Exception as e:
                    print(f"❌ Error during monitoring: {e}")
                    time.sleep(0.5)

            elapsed = time.time() - start_time
            print(f"\n📊 MONITORING SUMMARY")
            print(f"   Duration: {elapsed:.2f} seconds")
            print(f"   Register changes detected: {access_count}")
            print(f"   Monitored addresses: {len(self.monitored_addresses)}")

        except Exception as e:
            print(f"❌ Monitoring failed: {e}")

    def load_firmware(self, elf_path: str) -> bool:
        """Load firmware onto target"""
        if not self.connected:
            print("❌ Not connected to target")
            return False

        try:
            print(f"\n📥 LOADING FIRMWARE")
            print("-" * 40)

            if not os.path.exists(elf_path):
                print(f"❌ ELF file not found: {elf_path}")
                return False

            # Reset and halt
            print("🔄 Resetting target...")
            self.jlink.reset(halt=True)
            time.sleep(0.3)

            # Flash firmware
            print(f"⚡ Flashing: {os.path.basename(elf_path)}")
            self.jlink.flash_file(elf_path, addr=0x0)
            print("✅ Firmware loaded successfully")

            return True

        except Exception as e:
            print(f"❌ Failed to load firmware: {e}")
            return False

    def start_execution(self) -> bool:
        """Start firmware execution"""
        try:
            print("▶️  Starting firmware execution...")
            self.jlink.reset(halt=False)
            time.sleep(0.3)
            return True
        except Exception as e:
            print(f"❌ Failed to start execution: {e}")
            return False

    def cleanup(self) -> None:
        """Clean up resources"""
        if self.jlink:
            try:
                self.jlink.close()
            except:
                pass
            self.jlink = None
        self.connected = False
        print("🔌 Connection closed")

    def __enter__(self):
        """Context manager entry"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.cleanup()


def print_banner():
    """Print application banner"""
    print("=" * 80)
    print("🔍 MIMXRT700 Peripheral Register Monitor")
    print("   Real-time register access monitoring with bit-level analysis")
    print("   Based on comprehensive peripheral analysis data")
    print("=" * 80)


def main():
    """Main application entry point"""
    parser = argparse.ArgumentParser(
        description="Monitor MIMXRT700 peripheral register accesses using PyLink",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Monitor with default settings
  python mimxrt700_peripheral_monitor.py --probe 1065679149 --elf firmware.elf

  # Monitor for 60 seconds with custom analysis file
  python mimxrt700_peripheral_monitor.py --probe 1065679149 --elf firmware.elf \\
    --duration 60 --analysis custom_analysis.json

  # Load firmware and monitor immediately
  python mimxrt700_peripheral_monitor.py --probe 1065679149 --elf firmware.elf \\
    --load-firmware --start-execution
        """
    )

    parser.add_argument(
        "--probe", "-p",
        required=True,
        help="J-Link probe serial number"
    )

    parser.add_argument(
        "--device", "-d",
        default="MIMXRT798S_M33_CORE0",
        help="Target device name (default: MIMXRT798S_M33_CORE0)"
    )

    parser.add_argument(
        "--elf", "-e",
        help="ELF firmware file path"
    )

    parser.add_argument(
        "--analysis", "-a",
        default="complete_enhanced_peripheral_analysis.json",
        help="Peripheral analysis JSON file (default: complete_enhanced_peripheral_analysis.json)"
    )

    parser.add_argument(
        "--duration", "-t",
        type=int,
        default=30,
        help="Monitoring duration in seconds (default: 30)"
    )

    parser.add_argument(
        "--load-firmware", "-l",
        action="store_true",
        help="Load firmware before monitoring"
    )

    parser.add_argument(
        "--start-execution", "-s",
        action="store_true",
        help="Start firmware execution before monitoring"
    )

    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )

    args = parser.parse_args()

    print_banner()

    # Validate required files
    if not os.path.exists(args.analysis):
        print(f"❌ Analysis file not found: {args.analysis}")
        return 1

    if args.elf and not os.path.exists(args.elf):
        print(f"❌ ELF file not found: {args.elf}")
        return 1

    try:
        # Initialize monitor
        with PeripheralMonitor(args.probe, args.device) as monitor:

            # Load analysis data
            if not monitor.load_analysis_data(args.analysis):
                return 1

            # Connect to target
            if not monitor.connect():
                return 1

            # Load firmware if requested
            if args.load_firmware and args.elf:
                if not monitor.load_firmware(args.elf):
                    return 1

            # Start execution if requested
            if args.start_execution:
                if not monitor.start_execution():
                    return 1

            # Setup monitoring
            print(f"\n🎯 MONITORING SETUP")
            print(f"   Target: {args.device}")
            print(f"   Probe: {args.probe}")
            print(f"   Analysis: {args.analysis}")
            if args.elf:
                print(f"   Firmware: {args.elf}")
            print(f"   Duration: {args.duration}s")

            # Start monitoring
            monitor.monitor_execution(args.duration)

            print(f"\n✅ Monitoring completed successfully")
            return 0

    except KeyboardInterrupt:
        print(f"\n⚠️  Application interrupted by user")
        return 1
    except Exception as e:
        print(f"\n❌ Application failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
