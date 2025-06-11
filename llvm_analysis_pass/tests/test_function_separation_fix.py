#!/usr/bin/env python3
"""
Test Cases for Function Separation Fix
Validates that the comprehensive analysis correctly separates functions
and assigns proper execution order based on actual call_stack.
"""

import json
import os
import sys
import subprocess
from pathlib import Path

class FunctionSeparationTestSuite:
    """
    Comprehensive test suite for function separation fix
    """
    
    def __init__(self):
        self.test_results = []
        self.expected_function_order = {
            'BOARD_ConfigMPU': 1,
            'BOARD_InitPins': 2,
            'BOARD_BootClockRUN': 3,
            'BOARD_InitDebugConsole': 4,
            'BOARD_InitPsRamPins_Xspi2': 5,
            'CLOCK_AttachClk': 6,
            'CLOCK_SetClkDiv': 7,
        }
    
    def test_board_ll_function_separation(self):
        """Test that board.ll functions are properly separated"""
        print("🧪 TEST 1: Board.ll Function Separation")
        print("-" * 50)
        
        # Run analysis on board.ll
        board_analysis_file = "../build/board_functions_analysis.json"
        
        if not os.path.exists(board_analysis_file):
            print("❌ FAIL: board_functions_analysis.json not found")
            self.test_results.append(("Board Function Separation", False, "Analysis file missing"))
            return False
        
        try:
            with open(board_analysis_file, 'r') as f:
                data = json.load(f)
            
            # Extract all call_stacks
            call_stacks = set()
            if 'chronological_sequence' in data:
                for access in data['chronological_sequence']:
                    call_stack = access.get('call_stack')
                    if call_stack:
                        call_stacks.add(call_stack)
            
            print(f"📊 Functions found in board.ll: {len(call_stacks)}")
            for func in sorted(call_stacks):
                print(f"   - {func}")
            
            # Validate expected functions are present
            expected_functions = ['BOARD_ConfigMPU', 'BOARD_InitDebugConsole']
            missing_functions = []
            for func in expected_functions:
                if func not in call_stacks:
                    missing_functions.append(func)
            
            if missing_functions:
                print(f"❌ FAIL: Missing expected functions: {missing_functions}")
                self.test_results.append(("Board Function Separation", False, f"Missing: {missing_functions}"))
                return False
            
            print("✅ PASS: Board.ll functions properly detected")
            self.test_results.append(("Board Function Separation", True, f"Found {len(call_stacks)} functions"))
            return True
            
        except Exception as e:
            print(f"❌ FAIL: Error analyzing board.ll: {e}")
            self.test_results.append(("Board Function Separation", False, str(e)))
            return False
    
    def test_comprehensive_analysis_function_assignment(self):
        """Test that comprehensive analysis assigns correct module names"""
        print("\n🧪 TEST 2: Comprehensive Analysis Function Assignment")
        print("-" * 50)
        
        # Run the FIXED comprehensive analysis
        try:
            result = subprocess.run([
                'python3', '../../peripheral_monitoring/tools/comprehensive_multi_module_chronological_analysis.py'
            ], capture_output=True, text=True, timeout=60)
            
            if result.returncode != 0:
                print(f"❌ FAIL: Comprehensive analysis failed: {result.stderr}")
                self.test_results.append(("Comprehensive Function Assignment", False, "Analysis failed"))
                return False
            
            # Find the output file
            output_files = list(Path("../../peripheral_monitoring/tools").glob("comprehensive_chronological_sequence_*.json"))
            if not output_files:
                print("❌ FAIL: No comprehensive analysis output found")
                self.test_results.append(("Comprehensive Function Assignment", False, "No output file"))
                return False
            
            # Load the most recent output
            latest_file = max(output_files, key=lambda p: p.stat().st_mtime)
            with open(latest_file, 'r') as f:
                data = json.load(f)
            
            # Test function assignment correctness
            assignment_errors = []
            function_counts = {}
            
            for access in data['chronological_sequence']:
                call_stack = access.get('original_call_stack') or access.get('call_stack', 'UNKNOWN')
                module_name = access.get('module_name', 'UNKNOWN')
                execution_order = access.get('execution_order', 0)
                
                # Track function counts
                if call_stack not in function_counts:
                    function_counts[call_stack] = 0
                function_counts[call_stack] += 1
                
                # Validate assignment
                if call_stack in self.expected_function_order:
                    expected_module = call_stack
                    expected_order = self.expected_function_order[call_stack]
                    
                    if module_name != expected_module:
                        assignment_errors.append(f"{call_stack}: module_name='{module_name}' (expected '{expected_module}')")
                    
                    if execution_order != expected_order:
                        assignment_errors.append(f"{call_stack}: execution_order={execution_order} (expected {expected_order})")
            
            print(f"📊 Functions in comprehensive analysis: {len(function_counts)}")
            for func, count in sorted(function_counts.items()):
                expected_order = self.expected_function_order.get(func, 'N/A')
                print(f"   - {func:25s}: {count:3d} accesses (expected order: {expected_order})")
            
            if assignment_errors:
                print(f"❌ FAIL: Assignment errors found:")
                for error in assignment_errors[:5]:  # Show first 5 errors
                    print(f"   - {error}")
                self.test_results.append(("Comprehensive Function Assignment", False, f"{len(assignment_errors)} errors"))
                return False
            
            print("✅ PASS: All functions correctly assigned")
            self.test_results.append(("Comprehensive Function Assignment", True, f"{len(function_counts)} functions"))
            return True
            
        except Exception as e:
            print(f"❌ FAIL: Error in comprehensive analysis test: {e}")
            self.test_results.append(("Comprehensive Function Assignment", False, str(e)))
            return False
    
    def test_execution_order_correctness(self):
        """Test that execution order matches C source code"""
        print("\n🧪 TEST 3: Execution Order Correctness")
        print("-" * 50)
        
        # Load latest comprehensive analysis
        try:
            output_files = list(Path("../../peripheral_monitoring/tools").glob("comprehensive_chronological_sequence_*.json"))
            if not output_files:
                print("❌ FAIL: No comprehensive analysis output found")
                self.test_results.append(("Execution Order Correctness", False, "No output file"))
                return False
            
            latest_file = max(output_files, key=lambda p: p.stat().st_mtime)
            with open(latest_file, 'r') as f:
                data = json.load(f)
            
            # Check execution order sequence
            order_sequence = []
            for access in data['chronological_sequence']:
                order = access.get('execution_order', 0)
                module = access.get('module_name', 'UNKNOWN')
                if (order, module) not in order_sequence:
                    order_sequence.append((order, module))
            
            # Sort by execution order
            order_sequence.sort(key=lambda x: x[0])
            
            print("📊 Execution order sequence:")
            for order, module in order_sequence:
                print(f"   {order}. {module}")
            
            # Validate expected sequence
            expected_sequence = [
                (1, 'BOARD_ConfigMPU'),
                (2, 'BOARD_InitPins'),
                (3, 'BOARD_BootClockRUN'),
                (4, 'BOARD_InitDebugConsole'),
                (6, 'CLOCK_Functions'),
            ]
            
            sequence_errors = []
            for expected_order, expected_module in expected_sequence:
                found = False
                for actual_order, actual_module in order_sequence:
                    if actual_order == expected_order and (actual_module == expected_module or 
                                                          (expected_module == 'CLOCK_Functions' and 'CLOCK' in actual_module)):
                        found = True
                        break
                
                if not found:
                    sequence_errors.append(f"Missing: order {expected_order}, module {expected_module}")
            
            if sequence_errors:
                print(f"❌ FAIL: Execution order errors:")
                for error in sequence_errors:
                    print(f"   - {error}")
                self.test_results.append(("Execution Order Correctness", False, f"{len(sequence_errors)} errors"))
                return False
            
            print("✅ PASS: Execution order matches C source code")
            self.test_results.append(("Execution Order Correctness", True, "Sequence correct"))
            return True
            
        except Exception as e:
            print(f"❌ FAIL: Error in execution order test: {e}")
            self.test_results.append(("Execution Order Correctness", False, str(e)))
            return False
    
    def test_first_access_correctness(self):
        """Test that the first access is correctly assigned"""
        print("\n🧪 TEST 4: First Access Correctness")
        print("-" * 50)
        
        try:
            # Load latest comprehensive analysis
            output_files = list(Path("../../peripheral_monitoring/tools").glob("comprehensive_chronological_sequence_*.json"))
            if not output_files:
                print("❌ FAIL: No comprehensive analysis output found")
                self.test_results.append(("First Access Correctness", False, "No output file"))
                return False
            
            latest_file = max(output_files, key=lambda p: p.stat().st_mtime)
            with open(latest_file, 'r') as f:
                data = json.load(f)
            
            # Get first access
            first_access = data['chronological_sequence'][0]
            
            call_stack = first_access.get('original_call_stack') or first_access.get('call_stack', 'UNKNOWN')
            module_name = first_access.get('module_name', 'UNKNOWN')
            execution_order = first_access.get('execution_order', 0)
            
            print(f"📊 First access details:")
            print(f"   call_stack: {call_stack}")
            print(f"   module_name: {module_name}")
            print(f"   execution_order: {execution_order}")
            print(f"   peripheral: {first_access.get('peripheral_name', 'UNKNOWN')}")
            print(f"   register: {first_access.get('register_name', 'UNKNOWN')}")
            
            # Validate first access should be from BOARD_ConfigMPU (execution order 1)
            if execution_order != 1:
                print(f"❌ FAIL: First access should have execution_order=1, got {execution_order}")
                self.test_results.append(("First Access Correctness", False, f"Wrong order: {execution_order}"))
                return False
            
            if call_stack in self.expected_function_order and module_name != call_stack:
                print(f"❌ FAIL: module_name '{module_name}' doesn't match call_stack '{call_stack}'")
                self.test_results.append(("First Access Correctness", False, f"Mismatch: {module_name} vs {call_stack}"))
                return False
            
            print("✅ PASS: First access correctly assigned")
            self.test_results.append(("First Access Correctness", True, f"Order: {execution_order}, Module: {module_name}"))
            return True
            
        except Exception as e:
            print(f"❌ FAIL: Error in first access test: {e}")
            self.test_results.append(("First Access Correctness", False, str(e)))
            return False
    
    def run_all_tests(self):
        """Run all test cases"""
        print("🎯 FUNCTION SEPARATION FIX TEST SUITE")
        print("=" * 60)
        print("MISSION: Validate that function separation fix works correctly")
        print("=" * 60)
        
        # Run all tests
        tests = [
            self.test_board_ll_function_separation,
            self.test_comprehensive_analysis_function_assignment,
            self.test_execution_order_correctness,
            self.test_first_access_correctness,
        ]
        
        passed = 0
        total = len(tests)
        
        for test in tests:
            if test():
                passed += 1
        
        # Print summary
        print(f"\n🎉 TEST SUITE SUMMARY")
        print("=" * 40)
        print(f"Tests passed: {passed}/{total}")
        print(f"Success rate: {passed/total:.1%}")
        
        print(f"\n📋 Detailed Results:")
        for test_name, result, details in self.test_results:
            status = "✅ PASS" if result else "❌ FAIL"
            print(f"   {status} {test_name:30s}: {details}")
        
        if passed == total:
            print(f"\n🏆 ALL TESTS PASSED!")
            print("Function separation fix is working correctly!")
        else:
            print(f"\n⚠️  TESTS FAILED!")
            print("Function separation fix needs more work!")
        
        return passed == total

def main():
    test_suite = FunctionSeparationTestSuite()
    success = test_suite.run_all_tests()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
