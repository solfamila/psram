#!/usr/bin/env python3
import json
import subprocess
import pylink

print("🧪 TESTING WORKFLOW")
print("=" * 30)

# Test LLVM analysis
cmd = ["../../llvm_analysis_pass/build/bin/peripheral-analyzer", 
       "../../clang_ir_final/board_init/board.ll", 
       "--chronological", "-v", "-o", "test.json"]

result = subprocess.run(cmd, capture_output=True, text=True)
if result.returncode != 0:
    print(f"❌ LLVM failed: {result.stderr}")
    exit(1)

with open("test.json", 'r') as f:
    data = json.load(f)

accesses = data.get('chronological_sequence', [])
functions = set(a.get('call_stack') for a in accesses if a.get('call_stack'))
unknown = sum(1 for a in accesses if not a.get('call_stack'))

print(f"📊 Results:")
print(f"   Accesses: {len(accesses)}")
print(f"   Functions: {len(functions)}")
print(f"   UNKNOWN: {unknown}")

for func in sorted(functions):
    count = sum(1 for a in accesses if a.get('call_stack') == func)
    print(f"   - {func}: {count} accesses")

# Test hardware
try:
    jlink = pylink.JLink()
    jlink.open(serial_no=1065679149)
    jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
    jlink.connect('MIMXRT798S_M33_0')
    
    first = accesses[0]
    addr = int(first.get('address', '0x0'), 16)
    value = jlink.memory_read32(addr, 1)[0]
    
    print(f"🔗 Hardware test:")
    print(f"   Function: {first.get('call_stack')}")
    print(f"   Register: {first.get('peripheral_name')} {first.get('register_name')}")
    print(f"   Address: {first.get('address')}")
    print(f"   Value: 0x{value:08X}")
    
    jlink.close()
    print("✅ WORKFLOW SUCCESS!")
    
except Exception as e:
    print(f"❌ Hardware failed: {e}")
    print("✅ LLVM analysis still works!")

