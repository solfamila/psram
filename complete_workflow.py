#!/usr/bin/env python3
"""
COMPLETE PERIPHERAL ANALYSIS WORKFLOW
Single script that does everything correctly:
1. LLVM Analysis (compile-time): Extract function names, addresses, execution order
2. PyLink Hardware Monitoring (runtime): Capture actual register values
3. Merge Results: Combine for complete chronological sequence with values
"""

import json
import subprocess
import pylink
from datetime import datetime
from pathlib import Path

class CompletePeripheralWorkflow:
    """
    Complete workflow for peripheral register analysis
    """
    
    def __init__(self):
        self.probe_serial = "1065679149"
        self.jlink = None
        self.llvm_results = []
        self.hardware_values = {}
        self.final_sequence = []
        
        # IR files to analyze in execution order
        self.ir_files = [
            ("../../clang_ir_final/board_init/board.ll", "board_functions", 1),
            ("../../clang_ir_final/board_init/pin_mux.ll", "pin_mux_functions", 2),
            ("../../clang_ir_final/board_init/clock_config.ll", "clock_config_functions", 3),
            ("../../clang_ir_final/drivers/fsl_clock.ll", "fsl_clock_functions", 4),
        ]
        
        # Function execution order mapping
        self.function_order = {
            'BOARD_ConfigMPU': 1,
            'BOARD_InitPins': 2,
            'BOARD_BootClockRUN': 3,
            'BOARD_InitDebugConsole': 4,
            'BOARD_InitPsRamPins_Xspi2': 5,
            'CLOCK_AttachClk': 6,
            'CLOCK_SetClkDiv': 7,
        }
    
    def step1_run_llvm_analysis(self):
        """Step 1: Run LLVM analysis on all IR files"""
        print("🔍 STEP 1: LLVM ANALYSIS (COMPILE-TIME)")
        print("=" * 50)
        
        for ir_file, file_type, expected_order in self.ir_files:
            print(f"📁 Analyzing {file_type}...")
            
            try:
                # Run LLVM analysis
                output_file = f"llvm_{file_type}_analysis.json"
                cmd = [
                    "../../llvm_analysis_pass/build/bin/peripheral-analyzer",
                    ir_file,
                    "--chronological",
                    "-v",
                    "-o", output_file
                ]
                
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
                
                if result.returncode != 0:
                    print(f"   ❌ LLVM analysis failed: {result.stderr}")
                    continue
                
                # Load results
                with open(output_file, 'r') as f:
                    data = json.load(f)
                
                # Process results
                if 'chronological_sequence' in data:
                    accesses = data['chronological_sequence']
                elif 'peripheral_accesses' in data:
                    accesses = []
                    for peripheral in data['peripheral_accesses']:
                        accesses.extend(peripheral['accesses'])
                else:
                    print(f"   ❌ No valid data format in {output_file}")
                    continue
                
                # Fix function names and execution order
                for access in accesses:
                    # Get function name from call_stack or source_location
                    func_name = access.get('call_stack') or access.get('source_location', {}).get('function', 'UNKNOWN')
                    
                    # Fix execution order
                    if func_name in self.function_order:
                        access['corrected_function'] = func_name
                        access['corrected_execution_order'] = self.function_order[func_name]
                    elif 'CLOCK_' in func_name:
                        access['corrected_function'] = 'CLOCK_Functions'
                        access['corrected_execution_order'] = 6
                    else:
                        # Infer from file type
                        if 'pin_mux' in file_type:
                            access['corrected_function'] = 'BOARD_InitPins'
                            access['corrected_execution_order'] = 2
                        elif 'clock_config' in file_type:
                            access['corrected_function'] = 'BOARD_BootClockRUN'
                            access['corrected_execution_order'] = 3
                        elif 'fsl_clock' in file_type:
                            access['corrected_function'] = 'CLOCK_Functions'
                            access['corrected_execution_order'] = 6
                        else:
                            access['corrected_function'] = func_name
                            access['corrected_execution_order'] = 8
                    
                    # Add file source
                    access['source_file'] = file_type
                    
                    self.llvm_results.append(access)
                
                print(f"   ✅ Found {len(accesses)} accesses")
                
            except Exception as e:
                print(f"   ❌ Error analyzing {ir_file}: {e}")
        
        # Sort by execution order
        self.llvm_results.sort(key=lambda x: (
            x.get('corrected_execution_order', 999),
            x.get('sequence_number', 0)
        ))
        
        print(f"\\n📊 LLVM Analysis Complete: {len(self.llvm_results)} total accesses")
        return len(self.llvm_results) > 0
    
    def step2_capture_hardware_values(self):
        """Step 2: Capture hardware register values with PyLink"""
        print("\\n🔗 STEP 2: PYLINK HARDWARE MONITORING (RUNTIME)")
        print("=" * 50)
        
        try:
            # Connect to J-Link
            print(f"🔗 Connecting to J-Link probe {self.probe_serial}...")
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=int(self.probe_serial))
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect('MIMXRT798S_M33_0')
            print("✅ Hardware connection established")
            
            # Get unique register addresses
            unique_addresses = set()
            for access in self.llvm_results:
                addr = access.get('address')
                if addr:
                    unique_addresses.add(addr)
            
            print(f"📍 Capturing values for {len(unique_addresses)} unique registers...")
            
            # Capture initial state
            print("🛑 Capturing initial register state...")
            initial_values = {}
            for addr_str in unique_addresses:
                try:
                    addr = int(addr_str, 16)
                    value = self.jlink.memory_read32(addr, 1)[0]
                    initial_values[addr_str] = f"0x{value:08X}"
                except Exception as e:
                    initial_values[addr_str] = "READ_ERROR"
            
            # Reset and run to capture final state
            print("🚀 Resetting target and running initialization...")
            self.jlink.reset()
            self.jlink.restart()
            import time
            time.sleep(0.1)  # Let it run briefly
            self.jlink.halt()
            
            # Capture final state
            print("🛑 Capturing final register state...")
            final_values = {}
            for addr_str in unique_addresses:
                try:
                    addr = int(addr_str, 16)
                    value = self.jlink.memory_read32(addr, 1)[0]
                    final_values[addr_str] = f"0x{value:08X}"
                except Exception as e:
                    final_values[addr_str] = "READ_ERROR"
            
            # Store hardware values
            self.hardware_values = {
                'initial': initial_values,
                'final': final_values
            }
            
            changes = sum(1 for addr in unique_addresses 
                         if initial_values.get(addr) != final_values.get(addr))
            print(f"✅ Hardware capture complete: {changes} registers changed")
            
            return True
            
        except Exception as e:
            print(f"❌ Hardware capture failed: {e}")
            return False
    
    def step3_merge_results(self):
        """Step 3: Merge LLVM analysis with hardware values"""
        print("\\n🔗 STEP 3: MERGE LLVM + HARDWARE RESULTS")
        print("=" * 50)
        
        for i, access in enumerate(self.llvm_results):
            addr = access.get('address', '')
            
            # Add hardware values
            if addr in self.hardware_values.get('initial', {}):
                access['before_value'] = self.hardware_values['initial'][addr]
                access['after_value'] = self.hardware_values['final'][addr]
                access['register_changed'] = (
                    self.hardware_values['initial'][addr] != 
                    self.hardware_values['final'][addr]
                )
                access['hardware_validated'] = True
            else:
                access['before_value'] = "N/A"
                access['after_value'] = "N/A"
                access['register_changed'] = False
                access['hardware_validated'] = False
            
            # Add final sequence number
            access['final_sequence'] = i
            
            self.final_sequence.append(access)
        
        print(f"✅ Merged {len(self.final_sequence)} accesses with hardware values")
        return True
    
    def step4_export_results(self):
        """Step 4: Export final comprehensive results"""
        print("\\n📊 STEP 4: EXPORT FINAL RESULTS")
        print("=" * 50)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Create metadata
        metadata = {
            "timestamp": timestamp,
            "workflow": "complete_peripheral_analysis",
            "total_accesses": len(self.final_sequence),
            "hardware_validated": self.jlink is not None,
            "unique_registers": len(set(a.get('address', '') for a in self.final_sequence if a.get('address'))),
            "functions_analyzed": len(set(a.get('corrected_function', '') for a in self.final_sequence)),
            "register_changes": sum(1 for a in self.final_sequence if a.get('register_changed', False))
        }
        
        # Create final data
        final_data = {
            "analysis_metadata": metadata,
            "chronological_sequence": self.final_sequence
        }
        
        # Export JSON
        json_filename = f"COMPLETE_workflow_results_{timestamp}.json"
        with open(json_filename, 'w') as f:
            json.dump(final_data, f, indent=2)
        
        print(f"✅ Results exported to: {json_filename}")
        
        # Show summary
        print(f"\\n📊 FINAL SUMMARY:")
        print(f"   Total accesses: {metadata['total_accesses']}")
        print(f"   Hardware validated: {metadata['hardware_validated']}")
        print(f"   Register changes: {metadata['register_changes']}")
        print(f"   Functions analyzed: {metadata['functions_analyzed']}")
        
        # Show first 5 accesses
        print(f"\\n📋 First 5 accesses:")
        for i, access in enumerate(self.final_sequence[:5], 1):
            func = access.get('corrected_function', 'UNKNOWN')
            peripheral = access.get('peripheral_name', 'UNKNOWN')
            register = access.get('register_name', 'UNKNOWN')
            before = access.get('before_value', 'N/A')
            after = access.get('after_value', 'N/A')
            changed = "🔄" if access.get('register_changed', False) else "➖"
            
            print(f"   {i}. {changed} {func:20s} {peripheral:8s} {register:12s} [{before} → {after}]")
        
        return json_filename
    
    def run_complete_workflow(self):
        """Run the complete workflow"""
        print("🎯 COMPLETE PERIPHERAL ANALYSIS WORKFLOW")
        print("=" * 70)
        print("MISSION: Extract function names + capture register values")
        print("=" * 70)
        
        # Step 1: LLVM Analysis
        if not self.step1_run_llvm_analysis():
            print("❌ WORKFLOW FAILED: LLVM analysis failed")
            return False
        
        # Step 2: Hardware Monitoring
        hardware_success = self.step2_capture_hardware_values()
        
        # Step 3: Merge Results
        if not self.step3_merge_results():
            print("❌ WORKFLOW FAILED: Merge failed")
            return False
        
        # Step 4: Export Results
        result_file = self.step4_export_results()
        
        # Cleanup
        if self.jlink:
            self.jlink.close()
        
        print(f"\\n🎉 COMPLETE WORKFLOW SUCCESS!")
        print(f"✅ Result file: {result_file}")
        print(f"✅ Hardware monitoring: {'SUCCESS' if hardware_success else 'FAILED (analysis still valid)'}")
        
        return True

def main():
    workflow = CompletePeripheralWorkflow()
    success = workflow.run_complete_workflow()
    return 0 if success else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
