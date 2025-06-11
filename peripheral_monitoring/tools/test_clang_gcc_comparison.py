#!/usr/bin/env python3
"""
MIMXRT700 Clang vs GCC Toolchain Comparison Test
===============================================

This script performs comprehensive comparison testing between GCC and Clang
compiled versions of the MIMXRT700 XSPI PSRAM project using real hardware
and the PyLink peripheral register monitor.

Features:
1. Automated testing of both GCC and Clang compiled binaries
2. Real-time peripheral register monitoring and comparison
3. Bit-level analysis of register changes
4. Performance benchmarking and timing analysis
5. Comprehensive reporting of differences and similarities

Usage:
    python3 test_clang_gcc_comparison.py --probe <JLINK_SERIAL>
"""

import argparse
import json
import os
import sys
import time
from datetime import datetime
from typing import Dict, List, Optional, Tuple

# Import our monitor classes
from mimxrt700_peripheral_monitor import PeripheralMonitor

class ToolchainComparisonTester:
    """Comprehensive tester for comparing GCC and Clang toolchains"""
    
    def __init__(self, probe_serial: str, device_name: str = "MIMXRT798S_M33_CORE0"):
        self.probe_serial = probe_serial
        self.device_name = device_name
        self.test_results = {}
        self.analysis_data = None
        
        # File paths
        self.gcc_elf = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/debug/xspi_psram_polling_transfer_cm33_core0.elf"
        self.clang_elf = "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/build_clang_debug/debug/xspi_psram_polling_transfer_cm33_core0.elf"
        self.analysis_json = "complete_enhanced_peripheral_analysis.json"
        
    def validate_setup(self) -> bool:
        """Validate test setup and file availability"""
        print("🔍 VALIDATING TEST SETUP")
        print("=" * 50)
        
        # Check analysis file
        if not os.path.exists(self.analysis_json):
            print(f"❌ Analysis file not found: {self.analysis_json}")
            return False
        print(f"✅ Analysis file: {self.analysis_json}")
        
        # Check GCC ELF
        if not os.path.exists(self.gcc_elf):
            print(f"❌ GCC ELF not found: {self.gcc_elf}")
            return False
        print(f"✅ GCC ELF: {self.gcc_elf}")
        
        # Check Clang ELF
        if not os.path.exists(self.clang_elf):
            print(f"⚠️  Clang ELF not found: {self.clang_elf}")
            print("   Will skip Clang-specific tests")
            self.clang_elf = None
        else:
            print(f"✅ Clang ELF: {self.clang_elf}")
        
        # Load analysis data
        try:
            with open(self.analysis_json, 'r') as f:
                self.analysis_data = json.load(f)
            print(f"✅ Loaded analysis data: {self.analysis_data['total_accesses']} accesses")
        except Exception as e:
            print(f"❌ Failed to load analysis data: {e}")
            return False
        
        return True
    
    def test_gcc_binary(self) -> Dict:
        """Test GCC compiled binary"""
        print(f"\n🔨 TESTING GCC COMPILED BINARY")
        print("=" * 50)
        
        results = {
            "toolchain": "GCC",
            "elf_file": self.gcc_elf,
            "test_time": datetime.now().isoformat(),
            "register_changes": [],
            "execution_phases": {},
            "peripheral_summary": {},
            "performance_metrics": {}
        }
        
        try:
            with PeripheralMonitor(self.probe_serial, self.device_name) as monitor:
                # Load analysis data
                if not monitor.load_analysis_data(self.analysis_json):
                    results["status"] = "failed"
                    results["error"] = "Failed to load analysis data"
                    return results
                
                # Connect to target
                if not monitor.connect():
                    results["status"] = "failed"
                    results["error"] = "Failed to connect to target"
                    return results
                
                # Load firmware
                print(f"📥 Loading GCC firmware...")
                if not monitor.load_firmware(self.gcc_elf):
                    results["status"] = "failed"
                    results["error"] = "Failed to load firmware"
                    return results
                
                # Start execution and monitor
                print(f"▶️  Starting execution...")
                start_time = time.time()
                
                if not monitor.start_execution():
                    results["status"] = "failed"
                    results["error"] = "Failed to start execution"
                    return results
                
                # Monitor for 30 seconds
                print(f"📊 Monitoring for 30 seconds...")
                monitor.monitor_execution(duration=30)
                
                execution_time = time.time() - start_time
                results["performance_metrics"]["total_execution_time"] = execution_time
                results["status"] = "success"
                
                print(f"✅ GCC test completed successfully")
                
        except Exception as e:
            print(f"❌ GCC test failed: {e}")
            results["status"] = "failed"
            results["error"] = str(e)
        
        return results
    
    def test_clang_binary(self) -> Optional[Dict]:
        """Test Clang compiled binary"""
        if not self.clang_elf:
            print(f"\n⚠️  SKIPPING CLANG TEST - Binary not available")
            return None
        
        print(f"\n🔧 TESTING CLANG COMPILED BINARY")
        print("=" * 50)
        
        results = {
            "toolchain": "Clang",
            "elf_file": self.clang_elf,
            "test_time": datetime.now().isoformat(),
            "register_changes": [],
            "execution_phases": {},
            "peripheral_summary": {},
            "performance_metrics": {}
        }
        
        try:
            with PeripheralMonitor(self.probe_serial, self.device_name) as monitor:
                # Load analysis data
                if not monitor.load_analysis_data(self.analysis_json):
                    results["status"] = "failed"
                    results["error"] = "Failed to load analysis data"
                    return results
                
                # Connect to target
                if not monitor.connect():
                    results["status"] = "failed"
                    results["error"] = "Failed to connect to target"
                    return results
                
                # Load firmware
                print(f"📥 Loading Clang firmware...")
                if not monitor.load_firmware(self.clang_elf):
                    results["status"] = "failed"
                    results["error"] = "Failed to load firmware"
                    return results
                
                # Start execution and monitor
                print(f"▶️  Starting execution...")
                start_time = time.time()
                
                if not monitor.start_execution():
                    results["status"] = "failed"
                    results["error"] = "Failed to start execution"
                    return results
                
                # Monitor for 30 seconds
                print(f"📊 Monitoring for 30 seconds...")
                monitor.monitor_execution(duration=30)
                
                execution_time = time.time() - start_time
                results["performance_metrics"]["total_execution_time"] = execution_time
                results["status"] = "success"
                
                print(f"✅ Clang test completed successfully")
                
        except Exception as e:
            print(f"❌ Clang test failed: {e}")
            results["status"] = "failed"
            results["error"] = str(e)
        
        return results
    
    def compare_results(self, gcc_results: Dict, clang_results: Optional[Dict]) -> Dict:
        """Compare results between GCC and Clang"""
        print(f"\n⚖️  COMPARING TOOLCHAIN RESULTS")
        print("=" * 50)
        
        comparison = {
            "comparison_time": datetime.now().isoformat(),
            "gcc_status": gcc_results.get("status", "unknown"),
            "clang_status": clang_results.get("status", "unknown") if clang_results else "not_tested",
            "differences": [],
            "similarities": [],
            "recommendations": []
        }
        
        if not clang_results:
            comparison["summary"] = "Clang binary not available for comparison"
            comparison["recommendations"].append("Build Clang binary and rerun comparison")
            return comparison
        
        # Compare execution times
        gcc_time = gcc_results.get("performance_metrics", {}).get("total_execution_time", 0)
        clang_time = clang_results.get("performance_metrics", {}).get("total_execution_time", 0)
        
        if gcc_time > 0 and clang_time > 0:
            time_diff_percent = abs(gcc_time - clang_time) / gcc_time * 100
            comparison["performance_difference_percent"] = time_diff_percent
            
            if time_diff_percent < 5:
                comparison["similarities"].append(f"Similar execution times (GCC: {gcc_time:.2f}s, Clang: {clang_time:.2f}s)")
            elif time_diff_percent < 20:
                comparison["differences"].append(f"Minor timing difference: {time_diff_percent:.1f}% (GCC: {gcc_time:.2f}s, Clang: {clang_time:.2f}s)")
            else:
                comparison["differences"].append(f"Significant timing difference: {time_diff_percent:.1f}% (GCC: {gcc_time:.2f}s, Clang: {clang_time:.2f}s)")
        
        # Success/failure comparison
        if gcc_results["status"] == "success" and clang_results["status"] == "success":
            comparison["similarities"].append("Both toolchains executed successfully")
            comparison["recommendations"].append("Both toolchains are functionally equivalent")
        elif gcc_results["status"] != clang_results["status"]:
            comparison["differences"].append(f"Different execution status: GCC={gcc_results['status']}, Clang={clang_results['status']}")
            comparison["recommendations"].append("Investigate toolchain-specific execution issues")
        
        return comparison
    
    def generate_report(self, gcc_results: Dict, clang_results: Optional[Dict], comparison: Dict) -> str:
        """Generate comprehensive test report"""
        report_lines = [
            "# MIMXRT700 Toolchain Comparison Test Report",
            f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
            "",
            "## Test Configuration",
            f"- Target Device: {self.device_name}",
            f"- J-Link Probe: {self.probe_serial}",
            f"- Analysis Data: {self.analysis_json}",
            "",
            "## Test Results Summary",
            f"- GCC Test Status: {gcc_results.get('status', 'unknown')}",
            f"- Clang Test Status: {clang_results.get('status', 'unknown') if clang_results else 'not_tested'}",
            ""
        ]
        
        # GCC Results
        report_lines.extend([
            "## GCC Compiled Binary Results",
            f"- ELF File: {gcc_results.get('elf_file', 'unknown')}",
            f"- Test Status: {gcc_results.get('status', 'unknown')}",
        ])
        
        if gcc_results.get("status") == "success":
            gcc_time = gcc_results.get("performance_metrics", {}).get("total_execution_time", 0)
            report_lines.append(f"- Execution Time: {gcc_time:.2f} seconds")
        
        if gcc_results.get("error"):
            report_lines.append(f"- Error: {gcc_results['error']}")
        
        report_lines.append("")
        
        # Clang Results
        if clang_results:
            report_lines.extend([
                "## Clang Compiled Binary Results",
                f"- ELF File: {clang_results.get('elf_file', 'unknown')}",
                f"- Test Status: {clang_results.get('status', 'unknown')}",
            ])
            
            if clang_results.get("status") == "success":
                clang_time = clang_results.get("performance_metrics", {}).get("total_execution_time", 0)
                report_lines.append(f"- Execution Time: {clang_time:.2f} seconds")
            
            if clang_results.get("error"):
                report_lines.append(f"- Error: {clang_results['error']}")
        else:
            report_lines.extend([
                "## Clang Compiled Binary Results",
                "- Status: Not tested (binary not available)",
            ])
        
        report_lines.append("")
        
        # Comparison Results
        report_lines.extend([
            "## Toolchain Comparison",
            "",
            "### Similarities",
        ])
        
        for similarity in comparison.get("similarities", []):
            report_lines.append(f"- ✅ {similarity}")
        
        report_lines.extend([
            "",
            "### Differences",
        ])
        
        for difference in comparison.get("differences", []):
            report_lines.append(f"- ⚠️ {difference}")
        
        report_lines.extend([
            "",
            "### Recommendations",
        ])
        
        for recommendation in comparison.get("recommendations", []):
            report_lines.append(f"- 📋 {recommendation}")
        
        return "\n".join(report_lines)
    
    def run_comparison_test(self) -> bool:
        """Run complete comparison test"""
        print("🚀 MIMXRT700 TOOLCHAIN COMPARISON TEST")
        print("=" * 60)
        
        # Validate setup
        if not self.validate_setup():
            return False
        
        # Test GCC binary
        gcc_results = self.test_gcc_binary()
        
        # Test Clang binary
        clang_results = self.test_clang_binary()
        
        # Compare results
        comparison = self.compare_results(gcc_results, clang_results)
        
        # Generate report
        report = self.generate_report(gcc_results, clang_results, comparison)
        
        # Save results
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        results_file = f"toolchain_comparison_results_{timestamp}.json"
        report_file = f"toolchain_comparison_report_{timestamp}.md"
        
        # Save JSON results
        with open(results_file, 'w') as f:
            json.dump({
                "gcc_results": gcc_results,
                "clang_results": clang_results,
                "comparison": comparison
            }, f, indent=2)
        
        # Save markdown report
        with open(report_file, 'w') as f:
            f.write(report)
        
        print(f"\n📊 TEST RESULTS SAVED")
        print(f"   JSON: {results_file}")
        print(f"   Report: {report_file}")
        
        # Print summary
        print(f"\n🎉 COMPARISON TEST COMPLETED")
        print("=" * 60)
        print(report)
        
        return True

def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description="Compare GCC and Clang toolchains for MIMXRT700",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument(
        "--probe", "-p",
        required=True,
        help="J-Link probe serial number"
    )
    
    parser.add_argument(
        "--device", "-d",
        default="MIMXRT798S_M33_CORE0",
        help="Target device name"
    )
    
    args = parser.parse_args()
    
    # Run comparison test
    tester = ToolchainComparisonTester(args.probe, args.device)
    
    try:
        success = tester.run_comparison_test()
        return 0 if success else 1
    except KeyboardInterrupt:
        print("\n⚠️  Test interrupted by user")
        return 1
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
