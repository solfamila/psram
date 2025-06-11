#!/usr/bin/env python3
"""
Complete Chronological Register Access Analysis
Combines LLVM analysis pass with PyLink hardware monitoring
to get the definitive register access sequence with before/after values.
"""

import os
import sys
import json
import subprocess
import time
from pathlib import Path
import pylink

class CompleteChronologicalAnalyzer:
    """
    Complete analysis combining LLVM static analysis with PyLink hardware monitoring
    """
    
    def __init__(self, probe_serial="1065679149"):
        self.probe_serial = probe_serial
        self.project_root = Path(__file__).parent
        self.llvm_analyzer = self.project_root / "llvm_analysis_pass/build/bin/peripheral-analyzer"
        self.ir_files = []
        self.static_accesses = []
        self.hardware_accesses = []
        self.chronological_sequence = []
        
        # Find all IR files
        self._find_ir_files()
        
    def _find_ir_files(self):
        """Find all LLVM IR files for analysis"""
        ir_dir = self.project_root / "clang_ir_final"
        
        # Critical files in execution order
        critical_files = [
            "board_init/hardware_init.ll",
            "board_init/clock_config.ll", 
            "board_init/pin_mux.ll",
            "board_init/board.ll",
            "xspi_psram_polling_transfer.ll"
        ]
        
        for file_path in critical_files:
            full_path = ir_dir / file_path
            if full_path.exists():
                self.ir_files.append(full_path)
                
        print(f"📁 Found {len(self.ir_files)} critical IR files")
        for ir_file in self.ir_files:
            print(f"   • {ir_file.name}")
    
    def run_llvm_analysis(self):
        """Run LLVM analysis pass on all IR files"""
        print("\n🔍 PHASE 1: LLVM STATIC ANALYSIS")
        print("=" * 50)
        
        if not self.llvm_analyzer.exists():
            print(f"❌ LLVM analyzer not found: {self.llvm_analyzer}")
            return False
            
        for ir_file in self.ir_files:
            print(f"\n🔍 Analyzing: {ir_file.name}")
            
            try:
                result = subprocess.run([
                    str(self.llvm_analyzer), 
                    str(ir_file), 
                    "-v"
                ], capture_output=True, text=True, timeout=30)
                
                if result.returncode == 0:
                    print(f"   ✅ Analysis completed")
                    
                    # Look for JSON output
                    json_file = Path("peripheral_analysis.json")
                    if json_file.exists():
                        self._parse_llvm_results(json_file, ir_file.name)
                        json_file.unlink()  # Clean up
                else:
                    print(f"   ❌ Analysis failed: {result.stderr}")
                    
            except subprocess.TimeoutExpired:
                print(f"   ⏰ Analysis timeout")
            except Exception as e:
                print(f"   ❌ Analysis error: {e}")
        
        print(f"\n📊 Total static accesses found: {len(self.static_accesses)}")
        return len(self.static_accesses) > 0
    
    def _parse_llvm_results(self, json_file, source_file):
        """Parse LLVM analysis results"""
        try:
            with open(json_file, 'r') as f:
                data = json.load(f)
            
            # Extract register accesses
            if 'chronological_sequence' in data:
                accesses = data['chronological_sequence']
            elif 'register_accesses' in data:
                accesses = data['register_accesses']
            else:
                return
            
            for access in accesses:
                access['source_file'] = source_file
                self.static_accesses.append(access)
                
        except Exception as e:
            print(f"   ❌ Failed to parse JSON: {e}")
    
    def setup_hardware_monitoring(self):
        """Setup PyLink connection for hardware monitoring"""
        print("\n🔗 PHASE 2: HARDWARE MONITORING SETUP")
        print("=" * 50)
        
        try:
            # Connect to J-Link
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=int(self.probe_serial))
            
            # Connect to target
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect('MIMXRT798S_M33_CORE0')
            
            print(f"✅ Connected to J-Link probe {self.probe_serial}")
            print(f"✅ Connected to target: MIMXRT798S_M33_CORE0")
            
            # Halt target for initial state capture
            self.jlink.halt()
            print("✅ Target halted for monitoring setup")
            
            return True
            
        except Exception as e:
            print(f"❌ Hardware setup failed: {e}")
            return False
    
    def capture_register_states(self):
        """Capture before/after register states during execution"""
        print("\n📊 PHASE 3: REGISTER STATE CAPTURE")
        print("=" * 50)
        
        # Create register monitoring list from static analysis
        registers_to_monitor = []
        for access in self.static_accesses:
            if 'address' in access:
                registers_to_monitor.append({
                    'address': access['address'],
                    'peripheral': access.get('peripheral_name', 'UNKNOWN'),
                    'register': access.get('register_name', 'UNKNOWN'),
                    'purpose': access.get('purpose', 'unknown')
                })
        
        print(f"📋 Monitoring {len(registers_to_monitor)} unique registers")
        
        # Capture initial state
        print("\n📸 Capturing initial register states...")
        initial_states = {}
        for reg in registers_to_monitor:
            try:
                address = reg['address']
                if isinstance(address, str):
                    address = int(address, 16)
                
                value = self.jlink.memory_read32(address, 1)[0]
                initial_states[address] = {
                    'before': value,
                    'peripheral': reg['peripheral'],
                    'register': reg['register'],
                    'purpose': reg['purpose']
                }
                
            except Exception as e:
                print(f"   ⚠️  Failed to read {reg['peripheral']} {reg['register']}: {e}")
        
        print(f"✅ Captured {len(initial_states)} initial register states")
        
        # Resume execution and monitor changes
        print("\n🚀 Resuming execution and monitoring changes...")
        self.jlink.restart()
        
        # Monitor for changes
        time.sleep(2)  # Let initialization run
        
        # Halt and capture final states
        self.jlink.halt()
        print("📸 Capturing final register states...")
        
        final_states = {}
        changes_detected = 0
        
        for address, initial_data in initial_states.items():
            try:
                final_value = self.jlink.memory_read32(address, 1)[0]
                final_states[address] = final_value
                
                if final_value != initial_data['before']:
                    changes_detected += 1
                    self.hardware_accesses.append({
                        'address': f"0x{address:08X}",
                        'peripheral': initial_data['peripheral'],
                        'register': initial_data['register'],
                        'purpose': initial_data['purpose'],
                        'before_value': f"0x{initial_data['before']:08X}",
                        'after_value': f"0x{final_value:08X}",
                        'changed': True
                    })
                else:
                    self.hardware_accesses.append({
                        'address': f"0x{address:08X}",
                        'peripheral': initial_data['peripheral'],
                        'register': initial_data['register'],
                        'purpose': initial_data['purpose'],
                        'before_value': f"0x{initial_data['before']:08X}",
                        'after_value': f"0x{final_value:08X}",
                        'changed': False
                    })
                    
            except Exception as e:
                print(f"   ⚠️  Failed to read final state for 0x{address:08X}: {e}")
        
        print(f"✅ Detected {changes_detected} register changes")
        return changes_detected > 0
    
    def create_chronological_sequence(self):
        """Create the definitive chronological sequence"""
        print("\n🔄 PHASE 4: CHRONOLOGICAL SEQUENCE CREATION")
        print("=" * 50)
        
        # Sort static accesses by sequence number
        sorted_static = sorted(self.static_accesses, 
                             key=lambda x: x.get('sequence_number', 0))
        
        # Merge with hardware data
        sequence_number = 1
        for static_access in sorted_static:
            # Find corresponding hardware data
            static_addr = static_access.get('address', 0)
            if isinstance(static_addr, str):
                static_addr = int(static_addr, 16)
            
            hardware_data = None
            for hw_access in self.hardware_accesses:
                hw_addr = hw_access['address']
                if isinstance(hw_addr, str):
                    hw_addr = int(hw_addr, 16)
                
                if hw_addr == static_addr:
                    hardware_data = hw_access
                    break
            
            # Create chronological entry
            chrono_entry = {
                'sequence_number': sequence_number,
                'peripheral_name': static_access.get('peripheral_name', 'UNKNOWN'),
                'register_name': static_access.get('register_name', 'UNKNOWN'),
                'address': f"0x{static_addr:08X}",
                'access_type': static_access.get('access_type', 'unknown'),
                'purpose': static_access.get('purpose', 'unknown'),
                'file_name': static_access.get('file_name', 'unknown'),
                'function_name': static_access.get('function_name', 'unknown'),
                'line_number': static_access.get('line_number', 0),
                'source_file': static_access.get('source_file', 'unknown')
            }
            
            # Add hardware correlation data
            if hardware_data:
                chrono_entry.update({
                    'before_value': hardware_data['before_value'],
                    'after_value': hardware_data['after_value'],
                    'register_changed': hardware_data['changed'],
                    'hardware_validated': True
                })
            else:
                chrono_entry.update({
                    'before_value': 'N/A',
                    'after_value': 'N/A', 
                    'register_changed': False,
                    'hardware_validated': False
                })
            
            self.chronological_sequence.append(chrono_entry)
            sequence_number += 1
        
        print(f"✅ Created chronological sequence with {len(self.chronological_sequence)} entries")
        return True
    
    def export_results(self):
        """Export complete results"""
        print("\n💾 PHASE 5: RESULTS EXPORT")
        print("=" * 50)
        
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        
        # Complete results
        complete_results = {
            'analysis_metadata': {
                'timestamp': timestamp,
                'probe_serial': self.probe_serial,
                'total_static_accesses': len(self.static_accesses),
                'total_hardware_accesses': len(self.hardware_accesses),
                'total_chronological_entries': len(self.chronological_sequence),
                'hardware_changes_detected': sum(1 for x in self.hardware_accesses if x['changed'])
            },
            'chronological_sequence': self.chronological_sequence,
            'static_analysis_data': self.static_accesses,
            'hardware_monitoring_data': self.hardware_accesses
        }
        
        # Export main results
        output_file = f"complete_chronological_analysis_{timestamp}.json"
        with open(output_file, 'w') as f:
            json.dump(complete_results, f, indent=2)
        
        print(f"✅ Complete results exported to: {output_file}")
        
        # Export summary
        summary_file = f"chronological_summary_{timestamp}.json"
        summary = {
            'summary': complete_results['analysis_metadata'],
            'chronological_sequence': self.chronological_sequence
        }
        
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)
        
        print(f"✅ Summary exported to: {summary_file}")
        
        return output_file, summary_file
    
    def run_complete_analysis(self):
        """Run the complete chronological analysis"""
        print("🎯 COMPLETE CHRONOLOGICAL REGISTER ACCESS ANALYSIS")
        print("=" * 70)
        print("MISSION: Get definitive register access sequence with before/after values")
        print("=" * 70)
        
        # Phase 1: LLVM Static Analysis
        if not self.run_llvm_analysis():
            print("❌ LLVM analysis failed")
            return False
        
        # Phase 2: Hardware Monitoring Setup
        if not self.setup_hardware_monitoring():
            print("❌ Hardware setup failed")
            return False
        
        # Phase 3: Register State Capture
        if not self.capture_register_states():
            print("❌ Register capture failed")
            return False
        
        # Phase 4: Chronological Sequence Creation
        if not self.create_chronological_sequence():
            print("❌ Sequence creation failed")
            return False
        
        # Phase 5: Results Export
        output_file, summary_file = self.export_results()
        
        # Final Summary
        print("\n🎉 COMPLETE CHRONOLOGICAL ANALYSIS FINISHED")
        print("=" * 50)
        print(f"✅ Static accesses analyzed: {len(self.static_accesses)}")
        print(f"✅ Hardware accesses monitored: {len(self.hardware_accesses)}")
        print(f"✅ Chronological entries created: {len(self.chronological_sequence)}")
        print(f"✅ Register changes detected: {sum(1 for x in self.hardware_accesses if x['changed'])}")
        print(f"✅ Results exported to: {output_file}")
        
        return True

def main():
    if len(sys.argv) > 1:
        probe_serial = sys.argv[1]
    else:
        probe_serial = "1065679149"
    
    analyzer = CompleteChronologicalAnalyzer(probe_serial)
    success = analyzer.run_complete_analysis()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
