#!/usr/bin/env python3
"""
Run Complete Workflow: LLVM Analysis + PyLink Hardware + Merge Results
"""

import json
import subprocess
import pylink
from datetime import datetime

def run_complete_workflow():
    print("🎯 COMPLETE PERIPHERAL ANALYSIS WORKFLOW")
    print("=" * 70)
    print("MISSION: Get chronological sequence with register values")
    print("=" * 70)
    
    # Step 1: LLVM Analysis on all IR files
    print("\n🔍 STEP 1: LLVM ANALYSIS")
    print("=" * 40)
    
    ir_files = [
        ("../../clang_ir_final/board_init/board.ll", "board_functions"),
        ("../../clang_ir_final/board_init/pin_mux.ll", "pin_mux_functions"),
        ("../../clang_ir_final/board_init/clock_config.ll", "clock_config_functions"),
        ("../../clang_ir_final/drivers/fsl_clock.ll", "fsl_clock_functions"),
    ]
    
    all_accesses = []
    
    for ir_file, file_type in ir_files:
        print(f"📁 Analyzing {file_type}...")
        
        try:
            # Run LLVM analysis
            output_file = f"llvm_{file_type}.json"
            cmd = [
                "../../llvm_analysis_pass/build/bin/peripheral-analyzer",
                ir_file,
                "--chronological",
                "-v",
                "-o", output_file
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode != 0:
                print(f"   ❌ Failed: {result.stderr}")
                continue
            
            # Load results
            with open(output_file, 'r') as f:
                data = json.load(f)
            
            # Extract accesses
            if 'chronological_sequence' in data:
                accesses = data['chronological_sequence']
            elif 'peripheral_accesses' in data:
                accesses = []
                for peripheral in data['peripheral_accesses']:
                    accesses.extend(peripheral['accesses'])
            else:
                print(f"   ❌ No valid data format")
                continue
            
            # Add source file info
            for access in accesses:
                access['source_file'] = file_type
                access['llvm_sequence'] = len(all_accesses)
                all_accesses.append(access)
            
            print(f"   ✅ Found {len(accesses)} accesses")
            
        except Exception as e:
            print(f"   ❌ Error: {e}")
    
    print(f"\n📊 LLVM Analysis Complete: {len(all_accesses)} total accesses")
    
    # Step 2: PyLink Hardware Monitoring
    print(f"\n🔗 STEP 2: PYLINK HARDWARE MONITORING")
    print("=" * 40)
    
    hardware_values = {}
    
    try:
        # Connect to hardware
        print("🔌 Connecting to J-Link probe 1065679149...")
        jlink = pylink.JLink()
        jlink.open(serial_no=1065679149)
        jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        jlink.connect('MIMXRT798S_M33_CORE0', verbose=True)
        jlink.coresight_configure()
        print("✅ Hardware connected")
        
        # Get unique register addresses
        unique_addresses = set()
        for access in all_accesses:
            addr = access.get('address')
            if addr:
                unique_addresses.add(addr)
        
        print(f"📍 Capturing values for {len(unique_addresses)} unique registers...")
        
        # Capture current register state
        print("📊 Reading current register values...")
        current_values = {}
        successful_reads = 0
        
        for addr_str in unique_addresses:
            try:
                addr = int(addr_str, 16)
                value = jlink.memory_read32(addr, 1)[0]
                current_values[addr_str] = f"0x{value:08X}"
                successful_reads += 1
            except Exception as e:
                current_values[addr_str] = "READ_ERROR"
        
        print(f"✅ Successfully read {successful_reads}/{len(unique_addresses)} registers")
        
        # Store hardware values
        hardware_values = {
            'current': current_values,
            'timestamp': datetime.now().isoformat()
        }
        
        jlink.close()
        print("🔌 Hardware connection closed")
        
    except Exception as e:
        print(f"❌ Hardware monitoring failed: {e}")
        print("⚠️  Continuing with LLVM analysis only...")
    
    # Step 3: Merge Results
    print(f"\n🔗 STEP 3: MERGE LLVM + HARDWARE RESULTS")
    print("=" * 40)
    
    final_sequence = []
    
    for i, access in enumerate(all_accesses):
        addr = access.get('address', '')
        
        # Add hardware values if available
        if addr in hardware_values.get('current', {}):
            access['current_value'] = hardware_values['current'][addr]
            access['hardware_validated'] = True
        else:
            access['current_value'] = "N/A"
            access['hardware_validated'] = False
        
        # Add final sequence number
        access['final_sequence'] = i
        
        final_sequence.append(access)
    
    print(f"✅ Merged {len(final_sequence)} accesses with hardware data")
    
    # Step 4: Export Results
    print(f"\n📊 STEP 4: EXPORT RESULTS")
    print("=" * 40)
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Create metadata
    metadata = {
        "timestamp": timestamp,
        "workflow": "complete_llvm_pylink_analysis",
        "total_accesses": len(final_sequence),
        "hardware_validated": len(hardware_values) > 0,
        "unique_registers": len(set(a.get('address', '') for a in final_sequence if a.get('address'))),
        "functions_analyzed": len(set(a.get('call_stack', '') for a in final_sequence if a.get('call_stack'))),
        "successful_hardware_reads": sum(1 for a in final_sequence if a.get('hardware_validated', False))
    }
    
    # Create final data
    final_data = {
        "analysis_metadata": metadata,
        "chronological_sequence": final_sequence
    }
    
    # Export JSON
    json_filename = f"WORKFLOW_complete_sequence_{timestamp}.json"
    with open(json_filename, 'w') as f:
        json.dump(final_data, f, indent=2)
    
    print(f"✅ Results exported to: {json_filename}")
    
    # Show summary
    print(f"\n📊 WORKFLOW SUMMARY:")
    print(f"   Total accesses: {metadata['total_accesses']}")
    print(f"   Hardware validated: {metadata['hardware_validated']}")
    print(f"   Successful reads: {metadata['successful_hardware_reads']}")
    print(f"   Functions analyzed: {metadata['functions_analyzed']}")
    
    # Show first 10 accesses with values
    print(f"\n📋 First 10 accesses with register values:")
    for i, access in enumerate(final_sequence[:10], 1):
        func = access.get('call_stack', 'UNKNOWN')
        peripheral = access.get('peripheral_name', 'UNKNOWN')
        register = access.get('register_name', 'UNKNOWN')
        addr = access.get('address', 'N/A')
        value = access.get('current_value', 'N/A')
        validated = "✅" if access.get('hardware_validated', False) else "❌"
        
        print(f"   {i:2d}. {validated} {func:20s} {peripheral:8s} {register:12s} {addr} = {value}")
    
    print(f"\n🎉 COMPLETE WORKFLOW SUCCESS!")
    print(f"✅ Result file: {json_filename}")
    
    return json_filename

if __name__ == "__main__":
    result_file = run_complete_workflow()
    print(f"\n🎯 READY FOR C CODE ANALYSIS!")
    print(f"Use file: {result_file}")
