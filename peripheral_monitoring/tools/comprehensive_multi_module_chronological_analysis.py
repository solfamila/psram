#!/usr/bin/env python3
"""
Comprehensive Multi-Module Chronological Analysis
Combines all individual module analyses to create the complete chronological sequence
with proper execution order based on C source code call sequence.
"""

import json
import pylink
from datetime import datetime

class ComprehensiveChronologicalAnalyzer:
    """
    Creates comprehensive chronological sequence from all modules
    """
    
    def __init__(self):
        self.probe_serial = "1065679149"
        self.jlink = None
        self.all_accesses = []
        self.unique_registers = set()
        
        # Module analysis files in execution order
        self.module_files = [
            # 1. BOARD_ConfigMPU (first in BOARD_InitHardware)
            ("../../llvm_analysis_pass/build/board_chronological.json", "BOARD_ConfigMPU", 1),

            # 2. BOARD_InitPins (second in BOARD_InitHardware)
            ("../../llvm_analysis_pass/tests/pin_mux_analysis.json", "BOARD_InitPins", 2),

            # 3. BOARD_BootClockRUN (third in BOARD_InitHardware)
            ("../../llvm_analysis_pass/tests/clock_config_analysis.json", "BOARD_BootClockRUN", 3),

            # 4. BOARD_InitDebugConsole (fourth in BOARD_InitHardware) - included in board.ll
            # 5. CLOCK_AttachClk/SetClkDiv (called from BOARD_InitHardware)
            ("../../llvm_analysis_pass/tests/fsl_clock_analysis.json", "CLOCK_Functions", 4),
        ]
    
    def load_all_module_analyses(self):
        """Load and combine all module analyses in execution order"""
        print("📊 Loading all module analyses in execution order...")
        
        total_loaded = 0
        sequence_offset = 0
        
        for file_path, module_name, execution_order in self.module_files:
            try:
                with open(file_path, 'r') as f:
                    data = json.load(f)
                
                module_accesses = []
                
                # Handle different JSON formats
                if 'peripheral_accesses' in data:
                    # Standard format
                    for peripheral in data['peripheral_accesses']:
                        for access in peripheral['accesses']:
                            # Add execution order information
                            access['module_name'] = module_name
                            access['execution_order'] = execution_order
                            access['global_sequence'] = sequence_offset + len(module_accesses)
                            module_accesses.append(access)
                            
                            # Track unique registers
                            addr = access.get('address', '')
                            if addr:
                                self.unique_registers.add(addr)
                
                elif 'chronological_sequence' in data:
                    # Chronological format
                    for access in data['chronological_sequence']:
                        access['module_name'] = module_name
                        access['execution_order'] = execution_order
                        access['global_sequence'] = sequence_offset + len(module_accesses)
                        module_accesses.append(access)
                        
                        # Track unique registers
                        addr = access.get('address', '')
                        if addr:
                            self.unique_registers.add(addr)
                
                self.all_accesses.extend(module_accesses)
                sequence_offset += len(module_accesses)
                total_loaded += len(module_accesses)
                
                print(f"   ✅ {module_name:20s}: {len(module_accesses):3d} accesses")
                
            except Exception as e:
                print(f"   ❌ Failed to load {file_path}: {e}")
        
        print(f"\n📊 Total accesses loaded: {total_loaded}")
        print(f"📊 Unique registers: {len(self.unique_registers)}")
        
        return total_loaded > 0
    
    def setup_hardware_connection(self):
        """Setup J-Link hardware connection"""
        try:
            print(f"🔗 Connecting to J-Link probe {self.probe_serial}...")
            
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=int(self.probe_serial))
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect('MIMXRT798S_M33_0')
            
            print("✅ Hardware connection established")
            return True
            
        except Exception as e:
            print(f"❌ Hardware connection failed: {e}")
            return False
    
    def capture_hardware_values(self):
        """Capture before/after values for all registers"""
        if not self.jlink:
            print("⚠️  No hardware connection, using static analysis only")
            return
        
        print("📸 Capturing hardware register values...")
        
        # Capture initial state
        print("🛑 Capturing initial register state...")
        initial_values = {}
        
        for addr_str in self.unique_registers:
            try:
                addr = int(addr_str, 16)
                value = self.jlink.memory_read32(addr, 1)[0]
                initial_values[addr_str] = f"0x{value:08X}"
                print(f"   📍 {addr_str}: 0x{value:08X}")
            except Exception as e:
                print(f"   ⚠️  Failed to read {addr_str}: {e}")
                initial_values[addr_str] = "N/A"
        
        # Reset and run to capture final state
        try:
            print("🚀 Resetting target and running initialization...")
            self.jlink.reset()
            self.jlink.restart()
            # Let it run briefly
            import time
            time.sleep(0.1)
            self.jlink.halt()
        except Exception as e:
            print(f"⚠️  Reset/run failed: {e}")
        
        # Capture final state
        print("🛑 Capturing final register state...")
        final_values = {}
        
        for addr_str in self.unique_registers:
            try:
                addr = int(addr_str, 16)
                value = self.jlink.memory_read32(addr, 1)[0]
                final_values[addr_str] = f"0x{value:08X}"
            except Exception as e:
                print(f"   ⚠️  Failed to read final {addr_str}: {e}")
                final_values[addr_str] = "N/A"
        
        # Update all accesses with before/after values
        changes_detected = 0
        for access in self.all_accesses:
            addr = access.get('address', '')
            if addr in initial_values and addr in final_values:
                access['before_value'] = initial_values[addr]
                access['after_value'] = final_values[addr]
                access['register_changed'] = initial_values[addr] != final_values[addr]
                access['hardware_validated'] = True
                
                if access['register_changed']:
                    changes_detected += 1
            else:
                access['before_value'] = "N/A"
                access['after_value'] = "N/A"
                access['register_changed'] = False
                access['hardware_validated'] = False
        
        print(f"📊 Register changes detected: {changes_detected}/{len(self.unique_registers)}")
    
    def export_comprehensive_results(self):
        """Export comprehensive chronological results"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Create comprehensive metadata
        metadata = {
            "timestamp": timestamp,
            "probe_serial": self.probe_serial,
            "total_register_accesses": len(self.all_accesses),
            "unique_registers": len(self.unique_registers),
            "modules_analyzed": len(self.module_files),
            "hardware_validated": self.jlink is not None,
            "register_changes_detected": sum(1 for a in self.all_accesses if a.get('register_changed', False))
        }
        
        # Sort by execution order and global sequence
        sorted_accesses = sorted(self.all_accesses, key=lambda x: (x.get('execution_order', 0), x.get('global_sequence', 0)))
        
        # Create final JSON
        final_data = {
            "analysis_metadata": metadata,
            "chronological_sequence": sorted_accesses
        }
        
        # Export JSON
        json_filename = f"comprehensive_chronological_sequence_{timestamp}.json"
        with open(json_filename, 'w') as f:
            json.dump(final_data, f, indent=2)
        
        # Export human-readable summary
        txt_filename = f"comprehensive_summary_{timestamp}.txt"
        with open(txt_filename, 'w') as f:
            f.write("COMPREHENSIVE MULTI-MODULE CHRONOLOGICAL SEQUENCE\\n")
            f.write("=" * 60 + "\\n\\n")
            f.write(f"Analysis Timestamp: {timestamp}\\n")
            f.write(f"Total Register Accesses: {len(self.all_accesses)}\\n")
            f.write(f"Unique Registers: {len(self.unique_registers)}\\n")
            f.write(f"Hardware Validated: {metadata['hardware_validated']}\\n")
            f.write(f"Register Changes: {metadata['register_changes_detected']}\\n\\n")
            
            f.write("EXECUTION ORDER BY MODULE:\\n")
            f.write("-" * 40 + "\\n")
            for file_path, module_name, order in self.module_files:
                module_count = sum(1 for a in self.all_accesses if a.get('module_name') == module_name)
                f.write(f"{order}. {module_name:20s}: {module_count:3d} accesses\\n")
            
            f.write("\\nCHRONOLOGICAL SEQUENCE:\\n")
            f.write("-" * 40 + "\\n")
            for i, access in enumerate(sorted_accesses[:50], 1):  # First 50
                peripheral = access.get('peripheral_name', 'UNKNOWN')
                register = access.get('register_name', 'UNKNOWN')
                before = access.get('before_value', 'N/A')
                after = access.get('after_value', 'N/A')
                changed = "🔄" if access.get('register_changed', False) else "➖"
                module = access.get('module_name', 'UNKNOWN')
                
                f.write(f"{i:3d}. {changed} {peripheral:8s} {register:12s} [{before:10s} → {after:10s}] ({module})\\n")
            
            if len(sorted_accesses) > 50:
                f.write(f"... and {len(sorted_accesses) - 50} more accesses\\n")
        
        print(f"✅ Comprehensive results exported to: {json_filename}")
        print(f"✅ Human-readable summary exported to: {txt_filename}")
        
        return json_filename, txt_filename
    
    def run_comprehensive_analysis(self):
        """Run the complete comprehensive analysis"""
        print("🎯 COMPREHENSIVE MULTI-MODULE CHRONOLOGICAL ANALYSIS")
        print("=" * 70)
        print("MISSION: Create definitive chronological sequence from all modules")
        print("=" * 70)
        
        # Load all module analyses
        if not self.load_all_module_analyses():
            print("❌ Failed to load module analyses")
            return False
        
        # Setup hardware connection
        hardware_available = self.setup_hardware_connection()
        
        # Capture hardware values if available
        if hardware_available:
            self.capture_hardware_values()
        
        # Export comprehensive results
        json_file, txt_file = self.export_comprehensive_results()
        
        # Print summary
        print(f"\\n🎉 COMPREHENSIVE ANALYSIS COMPLETE")
        print("=" * 50)
        print(f"✅ Total register accesses: {len(self.all_accesses)}")
        print(f"✅ Modules analyzed: {len(self.module_files)}")
        print(f"✅ Hardware validated: {hardware_available}")
        print(f"✅ Results: {json_file}")
        
        # Show first 10 accesses
        sorted_accesses = sorted(self.all_accesses, key=lambda x: (x.get('execution_order', 0), x.get('global_sequence', 0)))
        print(f"\\n📋 First 10 register accesses:")
        for i, access in enumerate(sorted_accesses[:10], 1):
            peripheral = access.get('peripheral_name', 'UNKNOWN')
            register = access.get('register_name', 'UNKNOWN')
            before = access.get('before_value', 'N/A')
            after = access.get('after_value', 'N/A')
            changed = "🔄" if access.get('register_changed', False) else "➖"
            module = access.get('module_name', 'UNKNOWN')
            
            print(f"   {i:2d}. {changed} {peripheral:8s} {register:12s} [{before:10s} → {after:10s}] ({module})")
        
        if self.jlink:
            self.jlink.close()
        
        return True

def main():
    analyzer = ComprehensiveChronologicalAnalyzer()
    success = analyzer.run_comprehensive_analysis()
    return 0 if success else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
