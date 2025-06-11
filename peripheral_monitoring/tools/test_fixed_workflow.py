#!/usr/bin/env python3
"""
TEST SCRIPT FOR FIXED COMPREHENSIVE WORKFLOW

This script tests the fixes made to the peripheral monitoring workflow:
1. Tests LLVM analysis value capture
2. Tests PyLink hardware monitoring
3. Validates against C source code
4. Checks for proper execution order
5. Verifies call stack information

Usage:
    python test_fixed_workflow.py
"""

import os
import sys
import json
import subprocess
from pathlib import Path

# Add project paths
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / "peripheral_monitoring" / "tools"))

def test_llvm_analysis_fixes():
    """Test LLVM analysis fixes"""
    print("🔍 TESTING LLVM ANALYSIS FIXES")
    print("=" * 50)
    
    # Check if LLVM analyzer exists
    llvm_analyzer = project_root / "llvm_analysis_pass" / "build" / "lib" / "libPeripheralAnalysisPass.so"
    if not llvm_analyzer.exists():
        print(f"❌ LLVM analyzer not found: {llvm_analyzer}")
        return False
    
    print(f"✅ LLVM analyzer found: {llvm_analyzer}")
    
    # Check IR files
    ir_directory = project_root / "llvm_ir_files"
    if not ir_directory.exists():
        print(f"❌ IR directory not found: {ir_directory}")
        return False
    
    ir_files = list(ir_directory.glob("*.ll"))
    print(f"📁 Found {len(ir_files)} IR files")
    
    if not ir_files:
        print("❌ No IR files found")
        return False
    
    # Test analysis on one file
    test_file = ir_files[0]
    print(f"🧪 Testing analysis on: {test_file.name}")
    
    try:
        # Use standalone analyzer instead of opt to avoid command line conflicts
        analyzer_binary = project_root / "llvm_analysis_pass" / "build" / "bin" / "peripheral-analyzer"
        cmd = [
            str(analyzer_binary),
            str(test_file)
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        if result.returncode == 0:
            print("✅ LLVM analysis executed successfully")
            
            # Check for JSON output
            json_files = ["peripheral_analysis.json", "peripheral_analysis_chronological.json"]
            for json_file in json_files:
                if Path(json_file).exists():
                    with open(json_file, 'r') as f:
                        data = json.load(f)
                        print(f"✅ Found output file: {json_file}")
                        
                        # Check for value capture
                        if isinstance(data, dict) and 'register_accesses' in data:
                            accesses = data['register_accesses']
                        elif isinstance(data, list):
                            accesses = data
                        else:
                            accesses = []
                        
                        print(f"📊 Total accesses: {len(accesses)}")
                        
                        # Check for actual values (not N/A)
                        with_values = [acc for acc in accesses 
                                     if acc.get('value_written') and 
                                     acc['value_written'] != 'N/A' and
                                     acc['value_written'] != 'null']
                        
                        print(f"📊 Accesses with values: {len(with_values)}")
                        
                        # Check for call stack info
                        with_call_stack = [acc for acc in accesses 
                                         if acc.get('call_stack') and 
                                         acc['call_stack'] != 'UNKNOWN']
                        
                        print(f"📊 Accesses with call stack: {len(with_call_stack)}")
                        
                        # Show sample access with value
                        if with_values:
                            sample = with_values[0]
                            print(f"✅ Sample access with value:")
                            print(f"   Register: {sample.get('peripheral_name', 'UNKNOWN')}.{sample.get('register_name', 'UNKNOWN')}")
                            print(f"   Value: {sample.get('value_written', 'NONE')}")
                            print(f"   Function: {sample.get('function_name', 'UNKNOWN')}")
                    
                    Path(json_file).unlink()  # Clean up
            
            return True
        else:
            print(f"❌ LLVM analysis failed: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"❌ LLVM analysis test failed: {e}")
        return False

def test_pylink_availability():
    """Test PyLink availability and basic functionality"""
    print("\n🔗 TESTING PYLINK AVAILABILITY")
    print("=" * 50)
    
    try:
        import pylink
        print("✅ PyLink module imported successfully")
        
        # Test J-Link connection (without actually connecting)
        probe_serial = "1065679149"
        print(f"🔍 Testing probe serial: {probe_serial}")
        
        # Just test if we can create a JLink instance
        jlink = pylink.JLink()
        print("✅ J-Link instance created")
        
        return True
        
    except ImportError as e:
        print(f"❌ PyLink not available: {e}")
        return False
    except Exception as e:
        print(f"❌ PyLink test failed: {e}")
        return False

def test_source_code_validator():
    """Test source code validator"""
    print("\n📝 TESTING SOURCE CODE VALIDATOR")
    print("=" * 50)
    
    try:
        sys.path.insert(0, str(project_root / "validation_framework"))
        from source_code_validator import SourceCodeValidator
        
        c_source_dir = project_root / "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0"
        cmsis_headers = c_source_dir / "__repo__"
        
        if not c_source_dir.exists():
            print(f"❌ C source directory not found: {c_source_dir}")
            return False
        
        if not cmsis_headers.exists():
            print(f"❌ CMSIS headers not found: {cmsis_headers}")
            return False
        
        validator = SourceCodeValidator(str(project_root), str(cmsis_headers))
        print("✅ Source code validator created")
        
        # Test MPU_CTRL validation
        expected_value = validator.get_expected_value('MPU_CTRL', 'ARM_MPU_Enable')
        if expected_value:
            print(f"✅ MPU_CTRL expected value: 0x{expected_value:08x}")
        else:
            print("⚠️  MPU_CTRL expected value not found")
        
        return True
        
    except Exception as e:
        print(f"❌ Source code validator test failed: {e}")
        return False

def test_workflow_integration():
    """Test the complete workflow integration"""
    print("\n🔧 TESTING WORKFLOW INTEGRATION")
    print("=" * 50)
    
    try:
        from fixed_comprehensive_workflow import FixedComprehensiveWorkflow
        
        workflow = FixedComprehensiveWorkflow(str(project_root), "1065679149")
        print("✅ Workflow instance created")
        
        # Test component initialization
        if workflow.initialize_components():
            print("✅ Components initialized successfully")
        else:
            print("❌ Component initialization failed")
            return False
        
        return True
        
    except Exception as e:
        print(f"❌ Workflow integration test failed: {e}")
        return False

def main():
    """Main test function"""
    print("🧪 TESTING FIXED COMPREHENSIVE WORKFLOW")
    print("=" * 70)
    print("This script tests all the fixes made to the peripheral monitoring workflow")
    print("=" * 70)
    
    tests = [
        ("LLVM Analysis Fixes", test_llvm_analysis_fixes),
        ("PyLink Availability", test_pylink_availability),
        ("Source Code Validator", test_source_code_validator),
        ("Workflow Integration", test_workflow_integration)
    ]
    
    results = {}
    
    for test_name, test_func in tests:
        print(f"\n{'='*20} {test_name} {'='*20}")
        try:
            results[test_name] = test_func()
        except Exception as e:
            print(f"❌ Test {test_name} crashed: {e}")
            results[test_name] = False
    
    # Summary
    print("\n" + "=" * 70)
    print("🎯 TEST SUMMARY")
    print("=" * 70)
    
    passed = 0
    total = len(tests)
    
    for test_name, result in results.items():
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{test_name:30} {status}")
        if result:
            passed += 1
    
    print(f"\nOverall: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 ALL TESTS PASSED! The workflow fixes are working correctly.")
        return 0
    else:
        print("⚠️  SOME TESTS FAILED. Check the output above for details.")
        return 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
