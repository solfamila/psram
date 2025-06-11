#!/usr/bin/env python3
"""
Test PyLink with Correct Device Name and Connection Method
Based on pylink.md tutorial
"""

import pylink
import json
import time

def test_pylink_connection():
    print("🔗 TESTING PYLINK WITH CORRECT DEVICE NAME")
    print("=" * 60)
    
    # Load LLVM analysis results
    with open("test.json", 'r') as f:
        data = json.load(f)
    
    accesses = data.get('chronological_sequence', [])
    print(f"📊 Found {len(accesses)} register accesses from LLVM")
    
    # Get first register for testing
    first_access = accesses[0]
    addr_str = first_access.get('address', '0x0')
    func = first_access.get('call_stack', 'UNKNOWN')
    peripheral = first_access.get('peripheral_name', 'UNKNOWN')
    register = first_access.get('register_name', 'UNKNOWN')
    
    print(f"\n📍 Testing first register:")
    print(f"   Function: {func}")
    print(f"   Register: {peripheral} {register}")
    print(f"   Address: {addr_str}")
    
    # Try different device names from pylink.md
    device_names_to_try = [
        'MIMXRT798S_M33_CORE0',  # Correct format from tutorial
        'MIMXRT798S_M33_0',      # Alternative format
        'MIMXRT798S',            # Simplified
        'MIMXRT700',             # Base family
        'Cortex-M33'             # Generic
    ]
    
    jlink = None
    try:
        print(f"\n🔌 Connecting to J-Link probe 1065679149...")
        jlink = pylink.JLink()
        
        # Open connection with specific probe
        jlink.open(serial_no=1065679149)
        print(f"✅ Connected to J-Link: {jlink.product_name}")
        
        # Configure interface for ARM Cortex-M (from tutorial)
        jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        print("✅ SWD interface configured")
        
        # Try different device names
        connected = False
        for device_name in device_names_to_try:
            try:
                print(f"\n🎯 Trying device: {device_name}")
                jlink.connect(device_name, verbose=True)
                print(f"✅ Connected to {device_name}")
                
                # Configure CoreSight (from tutorial)
                try:
                    jlink.coresight_configure()
                    print("✅ CoreSight configured")
                except Exception as e:
                    print(f"⚠️  CoreSight warning: {e}")
                
                # Verify connection
                try:
                    core_id = hex(jlink.core_id())
                    print(f"📋 Target Core ID: {core_id}")
                except:
                    print("📋 Target connected successfully")
                
                connected = True
                break
                
            except Exception as e:
                print(f"❌ Failed {device_name}: {e}")
        
        if not connected:
            print("❌ Could not connect to any device")
            return False
        
        # Test register reading
        print(f"\n📍 Testing register value capture...")
        
        try:
            addr = int(addr_str, 16)
            
            # Read register value using memory_read32 (from tutorial)
            value = jlink.memory_read32(addr, 1)[0]
            
            print(f"🎯 REGISTER VALUE CAPTURED:")
            print(f"   Address: {addr_str}")
            print(f"   Value: 0x{value:08X}")
            print(f"   Function: {func}")
            print(f"   Register: {peripheral} {register}")
            
            # Test a few more registers
            print(f"\n📊 Testing additional registers:")
            successful_reads = 1
            
            for i, access in enumerate(accesses[1:6], 2):  # Test next 5 registers
                try:
                    test_addr_str = access.get('address', '0x0')
                    test_addr = int(test_addr_str, 16)
                    test_value = jlink.memory_read32(test_addr, 1)[0]
                    test_func = access.get('call_stack', 'UNKNOWN')
                    test_peripheral = access.get('peripheral_name', 'UNKNOWN')
                    test_register = access.get('register_name', 'UNKNOWN')
                    
                    print(f"   {i}. {test_addr_str}: 0x{test_value:08X} ({test_func} {test_peripheral} {test_register})")
                    successful_reads += 1
                    
                except Exception as e:
                    print(f"   {i}. {test_addr_str}: Read failed - {e}")
            
            print(f"\n📊 RESULTS:")
            print(f"   Successful reads: {successful_reads}/6")
            print(f"   Success rate: {successful_reads/6:.1%}")
            
            if successful_reads >= 3:
                print("🎉 PYLINK REGISTER VALUE CAPTURE WORKING!")
                return True
            else:
                print("⚠️  Limited register access")
                return False
                
        except Exception as e:
            print(f"❌ Register read failed: {e}")
            return False
        
    except Exception as e:
        print(f"❌ PyLink connection failed: {e}")
        return False
    
    finally:
        if jlink:
            try:
                jlink.close()
                print("🔌 J-Link connection closed")
            except:
                pass

if __name__ == "__main__":
    success = test_pylink_connection()
    if success:
        print("\n✅ PYLINK TEST SUCCESSFUL!")
        print("Register values can be captured!")
    else:
        print("\n❌ PYLINK TEST FAILED!")
        print("Register value capture not working")
    
    exit(0 if success else 1)
