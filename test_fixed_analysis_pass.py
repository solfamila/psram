#!/usr/bin/env python3
"""
Test Script for Fixed LLVM Analysis Pass

This script tests the fixed LLVM analysis pass to verify that:
1. XCACHE_DisableCache functions are now detected
2. ARM_MPU_Disable functions are now detected  
3. The execution order and values are more accurate
"""

import subprocess
import json
import os
import sys
from pathlib import Path

def run_analysis_on_ir_file(ir_file, analyzer_path):
    """Run the peripheral analyzer on an IR file"""
    try:
        # Try to run the analyzer
        result = subprocess.run([analyzer_path, ir_file], 
                              capture_output=True, text=True, timeout=30)
        
        if result.returncode == 0:
            return True, result.stdout, result.stderr
        else:
            return False, result.stdout, result.stderr
            
    except subprocess.TimeoutExpired:
        return False, "", "Timeout"
    except Exception as e:
        return False, "", str(e)

def analyze_json_output(json_file):
    """Analyze the JSON output for our specific fixes"""
    if not os.path.exists(json_file):
        return None
    
    try:
        with open(json_file, 'r') as f:
            data = json.load(f)
        
        analysis = {
            'total_accesses': 0,
            'xcache_disable_found': False,
            'arm_mpu_disable_found': False,
            'arm_mpu_enable_found': False,
            'mpu_ctrl_value': None,
            'first_access_type': None,
            'xcache_disable_count': 0,
            'execution_order_issues': []
        }
        
        # Check chronological sequence if available
        if 'chronological_sequence' in data:
            accesses = data['chronological_sequence']
        elif 'register_accesses' in data:
            accesses = data['register_accesses']
        else:
            return analysis
        
        analysis['total_accesses'] = len(accesses)
        
        # Sort by sequence number
        sorted_accesses = sorted(accesses, key=lambda x: x.get('sequence_number', 0))
        
        for i, access in enumerate(sorted_accesses):
            purpose = access.get('purpose', '').lower()
            peripheral = access.get('peripheral_name', '')
            register = access.get('register_name', '')
            
            # Check for our specific fixes
            if 'cache disable' in purpose or 'xcache_disablecache' in purpose.lower():
                analysis['xcache_disable_found'] = True
                analysis['xcache_disable_count'] += 1
                
                # Check if it's first (should be!)
                if i == 0:
                    analysis['first_access_type'] = 'XCACHE_DisableCache'
            
            if 'mpu disable' in purpose or 'arm_mpu_disable' in purpose.lower():
                analysis['arm_mpu_disable_found'] = True
            
            if 'mpu enable' in purpose or 'arm_mpu_enable' in purpose.lower():
                analysis['arm_mpu_enable_found'] = True
                
                # Check MPU_CTRL value
                if peripheral == 'MPU' and register == 'CTRL':
                    value_written = access.get('value_written')
                    if value_written:
                        analysis['mpu_ctrl_value'] = value_written
        
        # Check execution order issues
        if sorted_accesses and analysis['first_access_type'] != 'XCACHE_DisableCache':
            first_access = sorted_accesses[0]
            analysis['execution_order_issues'].append(
                f"First access should be XCACHE_DisableCache, but found: {first_access.get('purpose', 'unknown')}"
            )
        
        return analysis
        
    except Exception as e:
        print(f"Error analyzing JSON: {e}")
        return None

def main():
    print("🧪 Testing Fixed LLVM Analysis Pass")
    print("=" * 60)
    print("Verifying fixes for:")
    print("1. XCACHE_DisableCache function detection")
    print("2. ARM_MPU_Disable function detection")
    print("3. Improved execution order and value extraction")
    print("=" * 60)
    
    # Find the analyzer
    analyzer_path = "llvm_analysis_pass/build/bin/peripheral-analyzer"
    if not os.path.exists(analyzer_path):
        print(f"❌ Analyzer not found: {analyzer_path}")
        return 1
    
    # Find IR files
    ir_files = []
    for root, dirs, files in os.walk("clang_ir_final"):
        for file in files:
            if file.endswith('.ll'):
                ir_files.append(os.path.join(root, file))
    
    if not ir_files:
        print("❌ No IR files found in clang_ir_final/")
        return 1
    
    print(f"📁 Found {len(ir_files)} IR files to analyze")
    
    # Test results
    successful_analyses = 0
    failed_analyses = 0
    xcache_disable_detections = 0
    arm_mpu_disable_detections = 0
    
    # Analyze each file
    for ir_file in ir_files[:5]:  # Test first 5 files
        print(f"\n🔍 Analyzing: {ir_file}")
        
        success, stdout, stderr = run_analysis_on_ir_file(ir_file, analyzer_path)
        
        if success:
            successful_analyses += 1
            print(f"  ✅ Analysis completed")
            
            # Check for JSON output
            json_file = ir_file.replace('.ll', '_peripheral_analysis.json')
            if os.path.exists(json_file):
                analysis = analyze_json_output(json_file)
                if analysis:
                    print(f"  📊 Total accesses: {analysis['total_accesses']}")
                    
                    if analysis['xcache_disable_found']:
                        xcache_disable_detections += 1
                        print(f"  ✅ XCACHE_DisableCache detected! (Count: {analysis['xcache_disable_count']})")
                    
                    if analysis['arm_mpu_disable_found']:
                        arm_mpu_disable_detections += 1
                        print(f"  ✅ ARM_MPU_Disable detected!")
                    
                    if analysis['mpu_ctrl_value']:
                        print(f"  📋 MPU_CTRL value: {analysis['mpu_ctrl_value']}")
                        if analysis['mpu_ctrl_value'] == '0x00000007':
                            print(f"  ✅ MPU_CTRL value is CORRECT!")
                        else:
                            print(f"  ⚠️  MPU_CTRL value may need verification")
                    
                    if analysis['first_access_type']:
                        print(f"  🥇 First access type: {analysis['first_access_type']}")
                    
                    if analysis['execution_order_issues']:
                        for issue in analysis['execution_order_issues']:
                            print(f"  ⚠️  {issue}")
        else:
            failed_analyses += 1
            print(f"  ❌ Analysis failed: {stderr}")
    
    # Summary
    print(f"\n📊 Test Summary")
    print("=" * 30)
    print(f"Successful analyses: {successful_analyses}")
    print(f"Failed analyses: {failed_analyses}")
    print(f"XCACHE_DisableCache detections: {xcache_disable_detections}")
    print(f"ARM_MPU_Disable detections: {arm_mpu_disable_detections}")
    
    if xcache_disable_detections > 0:
        print("\n✅ SUCCESS: XCACHE_DisableCache function is now being detected!")
    else:
        print("\n⚠️  No XCACHE_DisableCache detections found")
    
    if arm_mpu_disable_detections > 0:
        print("✅ SUCCESS: ARM_MPU_Disable function is now being detected!")
    else:
        print("⚠️  No ARM_MPU_Disable detections found")
    
    print(f"\n🎯 LLVM Analysis Pass Testing Complete")
    
    return 0 if successful_analyses > 0 else 1

if __name__ == "__main__":
    sys.exit(main())
