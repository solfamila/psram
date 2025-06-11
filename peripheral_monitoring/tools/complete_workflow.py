#!/usr/bin/env python3
"""
COMPLETE PERIPHERAL ANALYSIS WORKFLOW
"""

import json
import subprocess
import pylink
from datetime import datetime

def run_workflow():
    print("🎯 COMPLETE PERIPHERAL ANALYSIS WORKFLOW")
    print("=" * 70)
    
    # Step 1: Run LLVM analysis on board.ll to get function names
    print("🔍 STEP 1: LLVM ANALYSIS")
    print("=" * 30)
    
    cmd = [
        "../../llvm_analysis_pass/build/bin/peripheral-analyzer",
        "../../clang_ir_final/board_init/board.ll",
        "--chronological",
        "-v",
        "-o", "board_with_functions.json"
    ]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"❌ LLVM analysis failed: {result.stderr}")
        return False
    
    # Load and check results
    with open("board_with_functions.json", 'r') as f:
        data = json.load(f)
    
    accesses = data.get('chronological_sequence', [])
    print(f"✅ Found {len(accesses)} accesses")
    
    # Check function names
    functions_found = set()
    unknown_count = 0
    for access in accesses:
        func = access.get('call_stack')
        if func:
            functions_found.add(func)
        else:
            unknown_count += 1
    
    print(f"📊 Functions found: {len(functions_found)}")
    for func in sorted(functions_found):
        print(f"   - {func}")
    print(f"⚠️  UNKNOWN call_stack: {unknown_count}")
    
    # Step 2: Hardware monitoring
    print(f"\n🔗 STEP 2: HARDWARE MONITORING")
    print("=" * 30)
    
    try:
        jlink = pylink.JLink()
        jlink.open(serial_no=1065679149)
        jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        jlink.connect('MIMXRT798S_M33_0')
        print("✅ Hardware connected")
        
        # Get first register address
        first_access = accesses[0]
        addr_str = first_access.get('address', '0x0')
        addr = int(addr_str, 16)
        
        # Read current value
        value = jlink.memory_read32(addr, 1)[0]
        print(f"📍 First register {addr_str}: 0x{value:08X}")
        
        jlink.close()
        
    except Exception as e:
        print(f"❌ Hardware failed: {e}")
    
    print(f"\n🎉 WORKFLOW COMPLETE")
    return True

if __name__ == "__main__":
    run_workflow()
