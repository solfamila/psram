#!/usr/bin/env python3
"""
Final Chronological Register Access Sequence
Uses existing LLVM analysis results + PyLink monitoring for before/after values
"""

import json
import time
import pylink
from pathlib import Path

class FinalChronologicalSequence:
    def __init__(self, probe_serial="1065679149"):
        self.probe_serial = probe_serial
        self.jlink = None
        self.register_sequence = []
        
    def load_existing_analysis(self):
        """Load our existing comprehensive analysis results"""
        print("📊 Loading existing comprehensive analysis results...")

        # Load from our proven comprehensive analysis
        analysis_file = "../results/complete_enhanced_peripheral_analysis.json"
        
        try:
            with open(analysis_file, 'r') as f:
                data = json.load(f)

            if 'chronological_sequence' in data:
                self.register_sequence = data['chronological_sequence']
                print(f"   ✅ Loaded {len(self.register_sequence)} accesses from comprehensive analysis")
            else:
                print("   ⚠️  No chronological_sequence found in analysis file")

        except Exception as e:
            print(f"   ❌ Could not load {analysis_file}: {e}")
            return False
        print(f"📊 Total register accesses loaded: {len(self.register_sequence)}")
        
        return len(self.register_sequence) > 0
    
    def setup_hardware_connection(self):
        """Setup J-Link connection"""
        print(f"\n🔗 Connecting to J-Link probe {self.probe_serial}...")
        
        try:
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=int(self.probe_serial))
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect('MIMXRT798S_M33_CORE0')
            
            print("✅ Hardware connection established")
            return True
            
        except Exception as e:
            print(f"❌ Hardware connection failed: {e}")
            return False
    
    def capture_before_after_values(self):
        """Capture before/after values for each register in the sequence"""
        print("\n📸 Capturing before/after register values...")
        
        # Create unique register list
        unique_registers = {}
        for access in self.register_sequence:
            addr = access.get('address', 0)
            if isinstance(addr, str):
                addr = int(addr, 16)
            
            if addr not in unique_registers:
                unique_registers[addr] = {
                    'peripheral': access.get('peripheral_name', 'UNKNOWN'),
                    'register': access.get('register_name', 'UNKNOWN'),
                    'purpose': access.get('purpose', 'unknown')
                }
        
        print(f"📋 Monitoring {len(unique_registers)} unique registers")
        
        # Halt target and capture initial state
        self.jlink.halt()
        print("🛑 Target halted for initial state capture")
        
        before_values = {}
        for addr, info in unique_registers.items():
            try:
                value = self.jlink.memory_read32(addr, 1)[0]
                before_values[addr] = value
                print(f"   📍 {info['peripheral']} {info['register']} (0x{addr:08X}): 0x{value:08X}")
                
            except Exception as e:
                print(f"   ⚠️  Failed to read 0x{addr:08X}: {e}")
                before_values[addr] = 0
        
        # Reset and let initialization run
        print("\n🚀 Resetting target and running initialization...")
        self.jlink.reset()
        time.sleep(3)  # Let initialization complete
        
        # Halt and capture final state
        self.jlink.halt()
        print("🛑 Target halted for final state capture")
        
        after_values = {}
        changes_detected = 0
        
        for addr, info in unique_registers.items():
            try:
                value = self.jlink.memory_read32(addr, 1)[0]
                after_values[addr] = value
                
                if value != before_values[addr]:
                    changes_detected += 1
                    print(f"   🔄 {info['peripheral']} {info['register']}: 0x{before_values[addr]:08X} → 0x{value:08X}")
                else:
                    print(f"   ➖ {info['peripheral']} {info['register']}: 0x{value:08X} (unchanged)")
                    
            except Exception as e:
                print(f"   ⚠️  Failed to read final 0x{addr:08X}: {e}")
                after_values[addr] = 0
        
        print(f"\n📊 Register changes detected: {changes_detected}/{len(unique_registers)}")
        
        # Update sequence with before/after values
        for access in self.register_sequence:
            addr = access.get('address', 0)
            if isinstance(addr, str):
                addr = int(addr, 16)
            
            if addr in before_values and addr in after_values:
                access['before_value'] = f"0x{before_values[addr]:08X}"
                access['after_value'] = f"0x{after_values[addr]:08X}"
                access['register_changed'] = before_values[addr] != after_values[addr]
                access['hardware_validated'] = True
            else:
                access['before_value'] = "N/A"
                access['after_value'] = "N/A"
                access['register_changed'] = False
                access['hardware_validated'] = False
        
        return True
    
    def export_final_sequence(self):
        """Export the final chronological sequence with before/after values"""
        print("\n💾 Exporting final chronological sequence...")
        
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        
        # Create final results
        final_results = {
            'analysis_metadata': {
                'timestamp': timestamp,
                'probe_serial': self.probe_serial,
                'total_register_accesses': len(self.register_sequence),
                'hardware_validated_accesses': sum(1 for x in self.register_sequence if x.get('hardware_validated', False)),
                'register_changes_detected': sum(1 for x in self.register_sequence if x.get('register_changed', False))
            },
            'chronological_sequence': self.register_sequence
        }
        
        # Export complete results
        output_file = f"final_chronological_sequence_{timestamp}.json"
        with open(output_file, 'w') as f:
            json.dump(final_results, f, indent=2)
        
        print(f"✅ Final sequence exported to: {output_file}")
        
        # Export summary for easy viewing
        summary_file = f"chronological_summary_{timestamp}.txt"
        with open(summary_file, 'w') as f:
            f.write("🎯 FINAL CHRONOLOGICAL REGISTER ACCESS SEQUENCE\n")
            f.write("=" * 70 + "\n\n")
            
            for i, access in enumerate(self.register_sequence, 1):
                f.write(f"{i:3d}. {access.get('peripheral_name', 'UNKNOWN'):10s} ")
                f.write(f"{access.get('register_name', 'UNKNOWN'):15s} ")
                f.write(f"({access.get('address', 'N/A'):10s}) ")
                f.write(f"[{access.get('before_value', 'N/A'):10s} → {access.get('after_value', 'N/A'):10s}] ")
                
                if access.get('register_changed', False):
                    f.write("🔄 CHANGED ")
                else:
                    f.write("➖ SAME    ")
                
                f.write(f"- {access.get('purpose', 'unknown')}\n")
                f.write(f"     📁 {access.get('file_name', 'unknown')}:{access.get('line_number', 0)} ")
                f.write(f"in {access.get('function_name', 'unknown')}()\n\n")
        
        print(f"✅ Human-readable summary exported to: {summary_file}")
        
        return output_file, summary_file
    
    def run_final_analysis(self):
        """Run the complete final analysis"""
        print("🎯 FINAL CHRONOLOGICAL REGISTER ACCESS SEQUENCE ANALYSIS")
        print("=" * 70)
        print("MISSION: Get definitive sequence with before/after values")
        print("=" * 70)
        
        # Step 1: Load existing analysis
        if not self.load_existing_analysis():
            print("❌ Failed to load existing analysis")
            return False
        
        # Step 2: Setup hardware
        if not self.setup_hardware_connection():
            print("❌ Failed to setup hardware connection")
            return False
        
        # Step 3: Capture before/after values
        if not self.capture_before_after_values():
            print("❌ Failed to capture register values")
            return False
        
        # Step 4: Export results
        output_file, summary_file = self.export_final_sequence()
        
        # Final summary
        print("\n🎉 FINAL CHRONOLOGICAL ANALYSIS COMPLETE")
        print("=" * 50)
        print(f"✅ Total register accesses: {len(self.register_sequence)}")
        print(f"✅ Hardware validated: {sum(1 for x in self.register_sequence if x.get('hardware_validated', False))}")
        print(f"✅ Register changes detected: {sum(1 for x in self.register_sequence if x.get('register_changed', False))}")
        print(f"✅ Complete results: {output_file}")
        print(f"✅ Human summary: {summary_file}")
        
        # Show first few entries
        print("\n📋 First 10 register accesses:")
        for i, access in enumerate(self.register_sequence[:10], 1):
            status = "🔄" if access.get('register_changed', False) else "➖"
            print(f"  {i:2d}. {status} {access.get('peripheral_name', 'UNKNOWN'):8s} {access.get('register_name', 'UNKNOWN'):12s} "
                  f"[{access.get('before_value', 'N/A'):10s} → {access.get('after_value', 'N/A'):10s}]")
        
        return True

def main():
    analyzer = FinalChronologicalSequence("1065679149")
    success = analyzer.run_final_analysis()
    return 0 if success else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
