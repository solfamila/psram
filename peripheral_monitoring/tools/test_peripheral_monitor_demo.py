#!/usr/bin/env python3
"""
MIMXRT700 Peripheral Monitor Demonstration
=========================================

This script demonstrates the peripheral register monitoring capabilities
without requiring actual hardware. It shows how the monitor would analyze
both GCC and Clang compiled versions of the MIMXRT700 XSPI PSRAM project.

Features demonstrated:
1. Loading and parsing peripheral analysis data
2. Register bit field analysis
3. Comparison between GCC and Clang compiled binaries
4. Simulated register access monitoring
"""

import json
import os
import sys
import time
from typing import Dict, List, Optional

# Import our monitor classes
from mimxrt700_peripheral_monitor import PeripheralMonitor, RegisterAccess, REGISTER_BIT_FIELDS

class PeripheralMonitorDemo:
    """Demonstration version of the peripheral monitor"""
    
    def __init__(self):
        self.analysis_data = None
        self.register_accesses = []
        
    def load_analysis_data(self, json_file: str) -> bool:
        """Load peripheral analysis data from JSON file"""
        try:
            with open(json_file, 'r') as f:
                self.analysis_data = json.load(f)
            
            print(f"📊 Loading analysis data from {json_file}")
            print(f"   Total accesses: {self.analysis_data['total_accesses']}")
            print(f"   Files analyzed: {self.analysis_data['files_analyzed']}")
            
            # Convert JSON data to RegisterAccess objects
            for access_data in self.analysis_data['chronological_sequence']:
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
            
            print(f"✅ Loaded {len(self.register_accesses)} register accesses")
            return True
            
        except Exception as e:
            print(f"❌ Failed to load analysis data: {e}")
            return False
    
    def analyze_peripheral_distribution(self):
        """Analyze the distribution of peripheral accesses"""
        print(f"\n📊 PERIPHERAL ACCESS DISTRIBUTION")
        print("=" * 50)
        
        peripheral_counts = {}
        phase_counts = {}
        access_type_counts = {}
        
        for access in self.register_accesses:
            # Count by peripheral
            peripheral_counts[access.peripheral_name] = peripheral_counts.get(access.peripheral_name, 0) + 1
            
            # Count by execution phase
            phase_counts[access.execution_phase] = phase_counts.get(access.execution_phase, 0) + 1
            
            # Count by access type
            access_type_counts[access.access_type] = access_type_counts.get(access.access_type, 0) + 1
        
        print("Peripheral Access Counts:")
        for peripheral, count in sorted(peripheral_counts.items(), key=lambda x: x[1], reverse=True):
            percentage = (count / len(self.register_accesses)) * 100
            print(f"   {peripheral:12}: {count:3d} accesses ({percentage:5.1f}%)")
        
        print(f"\nExecution Phase Distribution:")
        for phase, count in sorted(phase_counts.items(), key=lambda x: x[1], reverse=True):
            percentage = (count / len(self.register_accesses)) * 100
            print(f"   {phase:12}: {count:3d} accesses ({percentage:5.1f}%)")
        
        print(f"\nAccess Type Distribution:")
        for access_type, count in sorted(access_type_counts.items(), key=lambda x: x[1], reverse=True):
            percentage = (count / len(self.register_accesses)) * 100
            print(f"   {access_type:20}: {count:3d} accesses ({percentage:5.1f}%)")
    
    def demonstrate_bit_field_analysis(self):
        """Demonstrate bit field analysis capabilities"""
        print(f"\n🔍 BIT FIELD ANALYSIS DEMONSTRATION")
        print("=" * 50)
        
        # Example register changes for demonstration
        demo_changes = [
            {
                "address": "0x40411000",
                "peripheral": "XSPI2",
                "register": "MCR",
                "before": 0x00000000,
                "after": 0x00004001,
                "description": "XSPI2 Module Configuration - Reset and Disable"
            },
            {
                "address": "0x40001020",
                "peripheral": "CLKCTL0",
                "register": "PSCCTL4",
                "before": 0x00000000,
                "after": 0x00000004,
                "description": "Clock Control - Enable System Cache"
            },
            {
                "address": "0x4000407C",
                "peripheral": "IOPCTL0",
                "register": "PIO0_31",
                "before": 0x00000000,
                "after": 0x00000116,
                "description": "Pin Configuration - Function 6 with pull-up"
            }
        ]
        
        for i, change in enumerate(demo_changes, 1):
            print(f"\n🔄 REGISTER CHANGE EXAMPLE #{i}")
            print(f"   Address: {change['address']}")
            print(f"   Peripheral: {change['peripheral']}")
            print(f"   Register: {change['register']}")
            print(f"   Description: {change['description']}")
            print(f"   Before: 0x{change['before']:08x} ({change['before']:>10})")
            print(f"   After:  0x{change['after']:08x} ({change['after']:>10})")
            
            # Analyze bit changes
            changes = self.analyze_bit_changes(
                change['before'], change['after'], 
                change['peripheral'], change['register']
            )
            
            print(f"   📝 BIT CHANGES:")
            for bit_change in changes:
                print(f"      {bit_change}")
    
    def analyze_bit_changes(self, old_value: int, new_value: int, 
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
        if peripheral in ["XSPI0", "XSPI1", "XSPI2"] and register == "MCR":
            return "XSPI_MCR"
        elif peripheral == "CLKCTL0":
            if "PSCCTL0" in register:
                return "CLKCTL0_PSCCTL0"
            elif "PSCCTL1" in register:
                return "CLKCTL0_PSCCTL1"
            elif "PSCCTL2" in register:
                return "CLKCTL0_PSCCTL2"
        elif peripheral in ["IOPCTL0", "IOPCTL1", "IOPCTL2"]:
            return "IOPCTL_PIO"
        
        return f"{peripheral}_{register}"
    
    def analyze_critical_registers(self):
        """Analyze the most critical register accesses"""
        print(f"\n🎯 CRITICAL REGISTER ANALYSIS")
        print("=" * 50)
        
        # Focus on key peripherals and registers
        critical_peripherals = ["XSPI2", "CLKCTL0", "IOPCTL0", "IOPCTL2"]
        critical_accesses = []
        
        for access in self.register_accesses:
            if (access.peripheral_name in critical_peripherals and 
                'write' in access.access_type):
                critical_accesses.append(access)
        
        print(f"Found {len(critical_accesses)} critical register writes")
        
        # Group by execution phase
        phase_groups = {}
        for access in critical_accesses:
            phase = access.execution_phase
            if phase not in phase_groups:
                phase_groups[phase] = []
            phase_groups[phase].append(access)
        
        for phase, accesses in phase_groups.items():
            print(f"\n📋 {phase.upper()} PHASE ({len(accesses)} accesses):")
            
            # Show first few accesses in each phase
            for access in accesses[:5]:
                print(f"   {access.sequence_number:3d}: {access.peripheral_name:8} {access.register_name:12} - {access.purpose}")
            
            if len(accesses) > 5:
                print(f"   ... and {len(accesses) - 5} more accesses")
    
    def simulate_toolchain_comparison(self):
        """Simulate comparison between GCC and Clang toolchains"""
        print(f"\n⚖️  TOOLCHAIN COMPARISON SIMULATION")
        print("=" * 50)
        
        print("This simulation shows how the monitor would compare")
        print("register access patterns between GCC and Clang compiled binaries:")
        
        print(f"\n📊 Simulated Results:")
        print(f"   GCC Compiled Binary:")
        print(f"      Total register accesses: 601")
        print(f"      XSPI2 accesses: 306 (50.9%)")
        print(f"      CLKCTL0 accesses: 147 (24.5%)")
        print(f"      Initialization time: ~2.3s")
        
        print(f"\n   Clang Compiled Binary (projected):")
        print(f"      Total register accesses: 598-605 (similar)")
        print(f"      XSPI2 accesses: 304-308 (similar pattern)")
        print(f"      CLKCTL0 accesses: 145-149 (similar pattern)")
        print(f"      Initialization time: ~2.1-2.4s (potentially optimized)")
        
        print(f"\n🔍 Expected Differences:")
        print(f"   • Register access order may vary due to optimization")
        print(f"   • Some redundant accesses might be eliminated")
        print(f"   • Function inlining could change call stack traces")
        print(f"   • Overall peripheral initialization should be identical")
        
        print(f"\n✅ Key Validation Points:")
        print(f"   • Same peripheral addresses accessed")
        print(f"   • Same final register values achieved")
        print(f"   • Same hardware initialization sequence")
        print(f"   • Compatible with existing analysis framework")

def main():
    """Main demonstration function"""
    print("🚀 MIMXRT700 Peripheral Register Monitor Demonstration")
    print("=" * 60)
    
    # Check if analysis file exists
    analysis_file = "complete_enhanced_peripheral_analysis.json"
    if not os.path.exists(analysis_file):
        print(f"❌ Analysis file not found: {analysis_file}")
        print("   Please ensure the peripheral analysis JSON file is present")
        return 1
    
    # Check for ELF files
    gcc_elf = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/debug/xspi_psram_polling_transfer_cm33_core0.elf"
    clang_elf = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/build_clang_debug/debug/xspi_psram_polling_transfer_cm33_core0.elf"
    
    print(f"\n📁 File Status:")
    print(f"   Analysis data: {analysis_file} {'✅' if os.path.exists(analysis_file) else '❌'}")
    print(f"   GCC ELF: {gcc_elf} {'✅' if os.path.exists(gcc_elf) else '❌'}")
    print(f"   Clang ELF: {clang_elf} {'✅' if os.path.exists(clang_elf) else '❌'}")
    
    # Initialize demo
    demo = PeripheralMonitorDemo()
    
    # Load analysis data
    if not demo.load_analysis_data(analysis_file):
        return 1
    
    # Run demonstrations
    try:
        demo.analyze_peripheral_distribution()
        demo.demonstrate_bit_field_analysis()
        demo.analyze_critical_registers()
        demo.simulate_toolchain_comparison()
        
        print(f"\n🎉 DEMONSTRATION COMPLETE")
        print("=" * 60)
        print("The peripheral monitor is ready to analyze both GCC and Clang")
        print("compiled binaries when connected to actual MIMXRT700 hardware.")
        print(f"\nNext steps for real hardware testing:")
        print(f"1. Connect J-Link probe to MIMXRT700 target")
        print(f"2. Build Clang version: cd mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc && make clang")
        print(f"3. Run monitor: python mimxrt700_peripheral_monitor.py --probe <serial> --elf <elf_file>")
        
        return 0
        
    except Exception as e:
        print(f"❌ Demonstration failed: {e}")
        return 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n⚠️  Demonstration interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Demonstration failed: {e}")
        sys.exit(1)
