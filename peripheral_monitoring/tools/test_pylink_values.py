#!/usr/bin/env python3
import json
import pylink

print("🔗 TESTING PYLINK REGISTER VALUE CAPTURE")
print("=" * 50)

# Load LLVM analysis results
with open("test.json", 'r') as f:
    data = json.load(f)

accesses = data.get('chronological_sequence', [])
print(f"📊 Found {len(accesses)} register accesses from LLVM")

# Test PyLink connection
try:
    print("🔗 Connecting to J-Link...")
    jlink = pylink.JLink()
    
    # Try different connection methods
    print("   Trying to open J-Link...")
    jlink.open()  # Try without serial number first
    
    print("   Setting interface to SWD...")
    jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
    
    print("   Connecting to target...")
    # Try different target names
    targets_to_try = [
        'MIMXRT798S_M33_0',
        'MIMXRT798S', 
        'MIMXRT700',
        'Cortex-M33'
    ]
    
    connected = False
    for target in targets_to_try:
        try:
            print(f"   Trying target: {target}")
            jlink.connect(target)
            connected = True
            print(f"   ✅ Connected to {target}")
            break
        except Exception as e:
            print(f"   ❌ Failed {target}: {e}")
    
    if not connected:
        print("❌ Could not connect to any target")
        exit(1)
    
    # Test register reading
    print(f"\n📍 Testing register value capture...")
    
    # Get first few register addresses
    test_registers = []
    for access in accesses[:5]:
        addr_str = access.get('address')
        if addr_str:
            test_registers.append({
                'address': addr_str,
                'function': access.get('call_stack'),
                'peripheral': access.get('peripheral_name'),
                'register': access.get('register_name')
            })
    
    print(f"🎯 Testing {len(test_registers)} registers:")
    
    successful_reads = 0
    for reg in test_registers:
        try:
            addr = int(reg['address'], 16)
            value = jlink.memory_read32(addr, 1)[0]
            
            print(f"   ✅ {reg['address']}: 0x{value:08X}")
            print(f"      Function: {reg['function']}")
            print(f"      Register: {reg['peripheral']} {reg['register']}")
            print()
            
            successful_reads += 1
            
        except Exception as e:
            print(f"   ❌ {reg['address']}: Read failed - {e}")
    
    print(f"📊 RESULTS:")
    print(f"   Successful reads: {successful_reads}/{len(test_registers)}")
    
    if successful_reads > 0:
        print("🎉 PYLINK REGISTER VALUE CAPTURE WORKING!")
    else:
        print("❌ NO REGISTER VALUES CAPTURED")
    
    jlink.close()
    
except Exception as e:
    print(f"❌ PyLink connection failed: {e}")
    print("\n🔍 Debugging info:")
    try:
        jlink = pylink.JLink()
        print(f"   J-Link DLL version: {jlink.version}")
        jlink.open()
        print(f"   Connected J-Links: {jlink.connected_emulators()}")
    except Exception as debug_e:
        print(f"   Debug failed: {debug_e}")

