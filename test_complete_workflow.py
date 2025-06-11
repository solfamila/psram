#!/usr/bin/env python3
"""
Test Complete Workflow
Tests the fixed workflow and validates function names + register values
"""

import json
import subprocess
import pylink

def test_workflow():
    print("🧪 TESTING COMPLETE WORKFLOW")
    print("=" * 50)
    
    # Test 1: LLVM Analysis Function Names
    print("🔍 TEST 1: LLVM Function Name Extraction")
    print("-" * 40)
    
    # Run LLVM analysis on board.ll
    cmd = [
        "../../llvm_analysis_pass/build/bin/peripheral-analyzer",
        "../../clang_ir_final/board_init/board.ll",
        "--chronological",
        "-v",
        "-o", "test_board_analysis.json"
    ]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"❌ LLVM analysis failed: {result.stderr}")
        return False
    
    # Load and validate results
    with open("test_board_analysis.json", 'r') as f:
        data = json.load(f)
    
    accesses = data.get('chronological_sequence', [])
    
    # Check function names
    functions_found = set()
    unknown_count = 0
    for access in accesses:
        func = access.get('call_stack')
        if func:
            functions_found.add(func)
        else:
            unknown_count += 1
    
    print(f"📊 Results:")
    print(f"   Total accesses: {len(accesses)}")
    print(f"   Functions found: {len(functions_found)}")
    print(f"   UNKNOWN call_stack: {unknown_count}")
    
    # Validate expected functions
    expected_functions = ['BOARD_ConfigMPU', 'BOARD_InitDebugConsole']
    missing_functions = []
    for func in expected_functions:
        if func not in functions_found:
            missing_functions.append(func)
    
    if missing_functions:
        print(f"❌ TEST 1 FAILED: Missing functions: {missing_functions}")
        return False
    
    if unknown_count > 0:
        print(f"❌ TEST 1 FAILED: {unknown_count} UNKNOWN call_stacks")
        return False
    
    print("✅ TEST 1 PASSED: All function names captured correctly")
    
    # Test 2: Hardware Register Values
    print(f"\\n🔗 TEST 2: Hardware Register Value Capture")
    print("-" * 40)
    
    try:
        # Connect to hardware
        jlink = pylink.JLink()
        jlink.open(serial_no=1065679149)
        jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        jlink.connect('MIMXRT798S_M33_0')
        print("✅ Hardware connected")
        
        # Test reading first few register addresses
        test_addresses = []
        for access in accesses[:5]:
            addr_str = access.get('address')
            if addr_str:
                test_addresses.append(addr_str)
        
        print(f"📍 Testing {len(test_addresses)} register addresses:")
        
        successful_reads = 0
        for addr_str in test_addresses:
            try:
                addr = int(addr_str, 16)
                value = jlink.memory_read32(addr, 1)[0]
                peripheral = next((a.get('peripheral_name', 'UNKNOWN') for a in accesses if a.get('address') == addr_str), 'UNKNOWN')
                register = next((a.get('register_name', 'UNKNOWN') for a in accesses if a.get('address') == addr_str), 'UNKNOWN')
                func = next((a.get('call_stack', 'UNKNOWN') for a in accesses if a.get('address') == addr_str), 'UNKNOWN')
                
                print(f"   ✅ {addr_str}: 0x{value:08X} ({peripheral} {register} in {func})")
                successful_reads += 1
                
            except Exception as e:
                print(f"   ❌ {addr_str}: Read failed - {e}")
        
        jlink.close()
        
        if successful_reads == 0:
            print("❌ TEST 2 FAILED: No register values captured")
            return False
        
        print(f"✅ TEST 2 PASSED: {successful_reads}/{len(test_addresses)} register values captured")
        
    except Exception as e:
        print(f"❌ TEST 2 FAILED: Hardware connection failed - {e}")
        return False
    
    # Test 3: Validate First Access
    print(f"\\n🎯 TEST 3: First Access Validation")
    print("-" * 40)
    
    first_access = accesses[0]
    func = first_access.get('call_stack', 'UNKNOWN')
    peripheral = first_access.get('peripheral_name', 'UNKNOWN')
    register = first_access.get('register_name', 'UNKNOWN')
    addr = first_access.get('address', 'UNKNOWN')
    
    print(f"📊 First access details:")
    print(f"   Function: {func}")
    print(f"   Peripheral: {peripheral}")
    print(f"   Register: {register}")
    print(f"   Address: {addr}")
    
    # Validate against expected C source sequence
    if func != 'BOARD_ConfigMPU':
        print(f"❌ TEST 3 FAILED: Expected BOARD_ConfigMPU, got {func}")
        return False
    
    if peripheral != 'XCACHE0':
        print(f"❌ TEST 3 FAILED: Expected XCACHE0, got {peripheral}")
        return False
    
    print("✅ TEST 3 PASSED: First access matches C source code")
    
    print(f"\\n🎉 ALL TESTS PASSED!")
    print("=" * 50)
    print("✅ Function names: FIXED")
    print("✅ Register values: CAPTURED")
    print("✅ Execution order: CORRECT")
    print("\\n🏆 WORKFLOW IS WORKING CORRECTLY!")
    
    return True

if __name__ == "__main__":
    success = test_workflow()
    exit(0 if success else 1)
