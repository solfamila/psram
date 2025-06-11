#!/usr/bin/env python3
"""
Analyze Workflow Results Against C Source Code
Validate the chronological sequence against actual C code execution order
"""

import json

def analyze_against_c_code():
    print("🔍 ANALYZING WORKFLOW RESULTS AGAINST C SOURCE CODE")
    print("=" * 70)
    
    # Load workflow results
    result_file = "WORKFLOW_complete_sequence_20250610_235514.json"
    
    with open(result_file, 'r') as f:
        data = json.load(f)
    
    sequence = data['chronological_sequence']
    metadata = data['analysis_metadata']
    
    print(f"📊 Loaded {metadata['total_accesses']} accesses")
    print(f"📊 Hardware validated: {metadata['hardware_validated']}")
    print(f"📊 Functions found: {metadata['functions_analyzed']}")
    
    # Expected C source execution order from hardware_init.c
    expected_c_order = [
        "BOARD_ConfigMPU",        # Line 136: BOARD_ConfigMPU();
        "BOARD_InitPins",         # Line 137: BOARD_InitPins();
        "BOARD_BootClockRUN",     # Line 138: BOARD_BootClockRUN();
        "BOARD_InitDebugConsole", # Line 139: BOARD_InitDebugConsole();
        "BOARD_InitPsRamPins_Xspi2", # Line 141: BOARD_InitPsRamPins_Xspi2();
        "CLOCK_AttachClk",        # Line 142: CLOCK_AttachClk();
        "CLOCK_SetClkDiv",        # Line 143: CLOCK_SetClkDiv();
    ]
    
    print(f"\n📋 EXPECTED C SOURCE EXECUTION ORDER:")
    for i, func in enumerate(expected_c_order, 1):
        print(f"   {i}. {func}")
    
    # Analyze actual sequence
    print(f"\n🔍 ANALYZING ACTUAL SEQUENCE:")
    print("=" * 50)
    
    # Get function order from our results
    function_first_appearance = {}
    for i, access in enumerate(sequence):
        func = access.get('call_stack', 'UNKNOWN')
        if func not in function_first_appearance and func != 'UNKNOWN':
            function_first_appearance[func] = i
    
    # Sort functions by first appearance
    actual_order = sorted(function_first_appearance.items(), key=lambda x: x[1])
    
    print(f"📊 ACTUAL FUNCTION EXECUTION ORDER:")
    for i, (func, first_seq) in enumerate(actual_order, 1):
        print(f"   {i:2d}. {func:25s} (first at sequence {first_seq})")
    
    # Validate against expected order
    print(f"\n✅ VALIDATION AGAINST C SOURCE CODE:")
    print("=" * 50)
    
    validation_results = []
    
    for expected_pos, expected_func in enumerate(expected_c_order):
        # Find this function in actual results
        actual_pos = None
        for i, (actual_func, _) in enumerate(actual_order):
            if actual_func == expected_func:
                actual_pos = i
                break
        
        if actual_pos is not None:
            if actual_pos == expected_pos:
                status = "✅ CORRECT"
            else:
                status = f"❌ WRONG (expected pos {expected_pos+1}, got {actual_pos+1})"
            
            validation_results.append((expected_func, True, status))
            print(f"   {expected_func:25s}: {status}")
        else:
            validation_results.append((expected_func, False, "❌ MISSING"))
            print(f"   {expected_func:25s}: ❌ MISSING")
    
    # Check for unexpected functions
    expected_func_names = set(expected_c_order)
    actual_func_names = set(func for func, _ in actual_order)
    unexpected_functions = actual_func_names - expected_func_names
    
    if unexpected_functions:
        print(f"\n⚠️  UNEXPECTED FUNCTIONS FOUND:")
        for func in sorted(unexpected_functions):
            pos = next(i for i, (f, _) in enumerate(actual_order) if f == func)
            print(f"   {func:25s}: at position {pos+1}")
    
    # Analyze first access details
    print(f"\n🎯 FIRST ACCESS ANALYSIS:")
    print("=" * 40)
    
    first_access = sequence[0]
    first_func = first_access.get('call_stack', 'UNKNOWN')
    first_peripheral = first_access.get('peripheral_name', 'UNKNOWN')
    first_register = first_access.get('register_name', 'UNKNOWN')
    first_addr = first_access.get('address', 'UNKNOWN')
    first_value = first_access.get('current_value', 'N/A')
    
    print(f"📊 First register access:")
    print(f"   Function: {first_func}")
    print(f"   Peripheral: {first_peripheral}")
    print(f"   Register: {first_register}")
    print(f"   Address: {first_addr}")
    print(f"   Value: {first_value}")
    
    # Validate first access against C code
    # From board.c line 224: XCACHE_DisableCache(XCACHE0);
    expected_first_func = "BOARD_ConfigMPU"
    expected_first_peripheral = "XCACHE0"
    expected_first_register = "CCR"
    
    print(f"\n✅ FIRST ACCESS VALIDATION:")
    if first_func == expected_first_func:
        print(f"   Function: ✅ CORRECT ({first_func})")
    else:
        print(f"   Function: ❌ WRONG (expected {expected_first_func}, got {first_func})")
    
    if first_peripheral == expected_first_peripheral:
        print(f"   Peripheral: ✅ CORRECT ({first_peripheral})")
    else:
        print(f"   Peripheral: ❌ WRONG (expected {expected_first_peripheral}, got {first_peripheral})")
    
    if first_register == expected_first_register:
        print(f"   Register: ✅ CORRECT ({first_register})")
    else:
        print(f"   Register: ❌ WRONG (expected {expected_first_register}, got {first_register})")
    
    # Show register values for key functions
    print(f"\n📊 REGISTER VALUES BY FUNCTION:")
    print("=" * 50)
    
    for expected_func in expected_c_order[:5]:  # First 5 functions
        func_accesses = [a for a in sequence if a.get('call_stack') == expected_func]
        if func_accesses:
            print(f"\n🔍 {expected_func} ({len(func_accesses)} accesses):")
            for i, access in enumerate(func_accesses[:3]):  # First 3 accesses
                peripheral = access.get('peripheral_name', 'UNKNOWN')
                register = access.get('register_name', 'UNKNOWN')
                addr = access.get('address', 'UNKNOWN')
                value = access.get('current_value', 'N/A')
                validated = "✅" if access.get('hardware_validated', False) else "❌"
                
                print(f"   {i+1}. {validated} {peripheral:8s} {register:12s} {addr} = {value}")
        else:
            print(f"\n🔍 {expected_func}: ❌ NO ACCESSES FOUND")
    
    # Summary
    print(f"\n📋 VALIDATION SUMMARY:")
    print("=" * 40)
    
    correct_functions = sum(1 for _, found, status in validation_results if found and "✅ CORRECT" in status)
    total_expected = len(expected_c_order)
    
    print(f"   Functions in correct order: {correct_functions}/{total_expected}")
    print(f"   Success rate: {correct_functions/total_expected:.1%}")
    
    if first_func == expected_first_func and first_peripheral == expected_first_peripheral:
        print(f"   First access: ✅ CORRECT")
    else:
        print(f"   First access: ❌ INCORRECT")
    
    if metadata['hardware_validated']:
        print(f"   Hardware values: ✅ CAPTURED")
    else:
        print(f"   Hardware values: ❌ NOT CAPTURED")
    
    # Final verdict
    if correct_functions >= total_expected * 0.8 and first_func == expected_first_func:
        print(f"\n🏆 ANALYSIS VERDICT: GOLD STAR! ⭐")
        print("✅ Sequence matches C source code")
        print("✅ Register values captured")
        print("✅ Function order correct")
    else:
        print(f"\n❌ ANALYSIS VERDICT: NEEDS WORK")
        print("⚠️  Sequence does not match C source code")
    
    return correct_functions >= total_expected * 0.8

if __name__ == "__main__":
    success = analyze_against_c_code()
    exit(0 if success else 1)
