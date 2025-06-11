#!/usr/bin/env python3
"""
FIXED Comprehensive Multi-Module Chronological Analysis
Correctly separates functions within compilation units and assigns proper execution order
"""

import json
from datetime import datetime

class FixedComprehensiveChronologicalAnalyzer:
    """
    Creates comprehensive chronological sequence with CORRECT function separation
    """
    
    def __init__(self):
        self.all_accesses = []
        self.unique_registers = set()
        
        # CORRECTED: Map functions to their actual execution order in BOARD_InitHardware
        self.function_execution_order = {
            # From hardware_init.c lines 136-143
            "BOARD_ConfigMPU": 1,           # Line 136: BOARD_ConfigMPU();
            "BOARD_InitPins": 2,            # Line 137: BOARD_InitPins();
            "BOARD_BootClockRUN": 3,        # Line 138: BOARD_BootClockRUN();
            "BOARD_InitDebugConsole": 4,    # Line 139: BOARD_InitDebugConsole();
            "BOARD_InitPsRamPins_Xspi1": 5, # Line 145: BOARD_InitPsRamPins_Xspi1();
            "BOARD_InitPsRamPins_Xspi2": 5, # Line 141: BOARD_InitPsRamPins_Xspi2();
            "CLOCK_AttachClk": 6,           # Line 142/146: CLOCK_AttachClk();
            "CLOCK_SetClkDiv": 7,           # Line 143/147: CLOCK_SetClkDiv();
        }
        
        # Module analysis files with CORRECTED function mapping
        self.module_files = [
            ("../../llvm_analysis_pass/build/board_functions_analysis.json", "board_functions"),
            ("../../llvm_analysis_pass/tests/pin_mux_analysis.json", "pin_mux_functions"),
            ("../../llvm_analysis_pass/tests/clock_config_analysis.json", "clock_config_functions"),
            ("../../llvm_analysis_pass/tests/fsl_clock_analysis.json", "fsl_clock_functions"),
        ]
    
    def load_and_separate_functions(self):
        """Load all analyses and correctly separate functions by their actual call_stack"""
        print("📊 Loading and correctly separating functions...")
        
        total_loaded = 0
        
        for file_path, file_type in self.module_files:
            try:
                with open(file_path, 'r') as f:
                    data = json.load(f)
                
                print(f"\n📁 Processing {file_type}:")
                
                # Handle different JSON formats
                accesses = []
                if 'peripheral_accesses' in data:
                    # Standard format
                    for peripheral in data['peripheral_accesses']:
                        accesses.extend(peripheral['accesses'])
                elif 'chronological_sequence' in data:
                    # Chronological format
                    accesses = data['chronological_sequence']
                
                # Group by actual function (call_stack)
                functions_found = {}
                for access in accesses:
                    call_stack = access.get('call_stack', 'UNKNOWN')
                    
                    # Determine correct module name and execution order
                    if call_stack in self.function_execution_order:
                        module_name = call_stack
                        execution_order = self.function_execution_order[call_stack]
                    elif 'CLOCK_' in call_stack:
                        module_name = "CLOCK_Functions"
                        execution_order = 6  # CLOCK functions are called 6th/7th
                    else:
                        # For functions not in BOARD_InitHardware, assign based on context
                        if 'Pin' in call_stack or 'IOPCTL' in call_stack:
                            module_name = "BOARD_InitPins"
                            execution_order = 2
                        elif 'Clock' in call_stack or 'CLKCTL' in call_stack:
                            module_name = "BOARD_BootClockRUN"
                            execution_order = 3
                        else:
                            module_name = call_stack
                            execution_order = 8  # Other functions
                    
                    # Update access with correct information
                    access['module_name'] = module_name
                    access['execution_order'] = execution_order
                    access['original_call_stack'] = call_stack
                    
                    # Track unique registers
                    addr = access.get('address', '')
                    if addr:
                        self.unique_registers.add(addr)
                    
                    # Track functions found
                    if call_stack not in functions_found:
                        functions_found[call_stack] = 0
                    functions_found[call_stack] += 1
                
                # Add to all accesses
                self.all_accesses.extend(accesses)
                total_loaded += len(accesses)
                
                # Report functions found
                for func, count in functions_found.items():
                    order = self.function_execution_order.get(func, "N/A")
                    print(f"   {func:25s}: {count:3d} accesses (order: {order})")
                
            except Exception as e:
                print(f"   ❌ Failed to load {file_path}: {e}")
        
        print(f"\n📊 Total accesses loaded: {total_loaded}")
        print(f"📊 Unique registers: {len(self.unique_registers)}")
        
        return total_loaded > 0
    
    def assign_global_sequence_numbers(self):
        """Assign global sequence numbers based on execution order and original sequence"""
        print("🔢 Assigning corrected global sequence numbers...")
        
        # Sort by execution order, then by original sequence within each function
        self.all_accesses.sort(key=lambda x: (
            x.get('execution_order', 999),
            x.get('sequence_number', 0),
            x.get('global_sequence', 0)
        ))
        
        # Assign new global sequence numbers
        for i, access in enumerate(self.all_accesses):
            access['global_sequence'] = i
        
        print(f"   ✅ Assigned sequence numbers 0-{len(self.all_accesses)-1}")
    
    def export_corrected_results(self):
        """Export corrected comprehensive chronological results"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Create comprehensive metadata
        metadata = {
            "timestamp": timestamp,
            "total_register_accesses": len(self.all_accesses),
            "unique_registers": len(self.unique_registers),
            "functions_analyzed": len(set(a.get('original_call_stack', '') for a in self.all_accesses)),
            "correction_applied": "Fixed function separation and execution order assignment"
        }
        
        # Create final JSON
        final_data = {
            "analysis_metadata": metadata,
            "chronological_sequence": self.all_accesses
        }
        
        # Export JSON
        json_filename = f"CORRECTED_comprehensive_chronological_sequence_{timestamp}.json"
        with open(json_filename, 'w') as f:
            json.dump(final_data, f, indent=2)
        
        print(f"✅ Corrected results exported to: {json_filename}")
        
        return json_filename
    
    def run_corrected_analysis(self):
        """Run the complete corrected comprehensive analysis"""
        print("🎯 CORRECTED COMPREHENSIVE MULTI-MODULE CHRONOLOGICAL ANALYSIS")
        print("=" * 75)
        print("MISSION: Fix function separation and create accurate chronological sequence")
        print("=" * 75)
        
        # Load and correctly separate functions
        if not self.load_and_separate_functions():
            print("❌ Failed to load module analyses")
            return False
        
        # Assign corrected global sequence numbers
        self.assign_global_sequence_numbers()
        
        # Export corrected results
        json_file = self.export_corrected_results()
        
        # Print summary
        print(f"\n🎉 CORRECTED ANALYSIS COMPLETE")
        print("=" * 50)
        print(f"✅ Total register accesses: {len(self.all_accesses)}")
        print(f"✅ Functions properly separated: {len(set(a.get('original_call_stack', '') for a in self.all_accesses))}")
        print(f"✅ Results: {json_file}")
        
        # Show corrected first 10 accesses
        print(f"\n📋 CORRECTED First 10 register accesses:")
        for i, access in enumerate(self.all_accesses[:10], 1):
            peripheral = access.get('peripheral_name', 'UNKNOWN')
            register = access.get('register_name', 'UNKNOWN')
            module = access.get('module_name', 'UNKNOWN')
            call_stack = access.get('original_call_stack', 'UNKNOWN')
            order = access.get('execution_order', 'N/A')
            
            print(f"   {i:2d}. {peripheral:8s} {register:12s} [{module} → {call_stack}] (order: {order})")
        
        return True

def main():
    analyzer = FixedComprehensiveChronologicalAnalyzer()
    success = analyzer.run_corrected_analysis()
    return 0 if success else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
