#!/usr/bin/env python3
"""
MIMXRT700 Register Access Diagnostics
====================================

Comprehensive diagnostic tests to investigate why 87.7% of register reads
are failing. This is inconsistent with typical ARM Cortex-M behavior.
"""

import sys
import time
import json
from typing import Dict, List, Optional, Tuple
import pylink

class RegisterAccessDiagnostics:
    """Comprehensive register access diagnostics"""
    
    def __init__(self, probe_serial: str, target_device: str):
        self.probe_serial = probe_serial
        self.target_device = target_device
        self.jlink = None
        self.diagnostic_results = {}
        
    def connect(self) -> bool:
        """Connect to J-Link and target"""
        try:
            self.jlink = pylink.JLink()
            self.jlink.open(serial_no=self.probe_serial)
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            self.jlink.connect(self.target_device)
            
            print(f"✅ Connected to J-Link {self.probe_serial}")
            print(f"✅ Connected to target {self.target_device}")
            print(f"   Core ID: 0x{self.jlink.core_id():08x}")
            print(f"   Device family: {self.jlink.core_name()}")
            
            return True
            
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            return False
    
    def test_basic_system_registers(self) -> Dict:
        """Test basic ARM Cortex-M system registers that should always work"""
        
        print(f"\n🔍 TESTING BASIC SYSTEM REGISTERS")
        print("=" * 50)
        
        system_registers = {
            0xE000ED00: "CPUID - CPU ID Register",
            0xE000ED04: "ICSR - Interrupt Control State Register", 
            0xE000ED08: "VTOR - Vector Table Offset Register",
            0xE000ED0C: "AIRCR - Application Interrupt/Reset Control",
            0xE000ED10: "SCR - System Control Register",
            0xE000ED14: "CCR - Configuration Control Register",
            0xE000ED94: "MPU_CTRL - MPU Control Register",
            0xE000ED9C: "MPU_RBAR - MPU Region Base Address"
        }
        
        results = {}
        
        for addr, desc in system_registers.items():
            try:
                # Test different access methods
                value_32 = self.jlink.memory_read32(addr, 1)[0]
                value_16 = self.jlink.memory_read16(addr, 2)
                value_8 = self.jlink.memory_read8(addr, 4)
                
                results[addr] = {
                    'description': desc,
                    'success': True,
                    'value_32bit': f"0x{value_32:08x}",
                    'value_16bit': [f"0x{v:04x}" for v in value_16],
                    'value_8bit': [f"0x{v:02x}" for v in value_8],
                    'consistent': all(v == value_32 for v in [
                        (value_16[1] << 16) | value_16[0],
                        (value_8[3] << 24) | (value_8[2] << 16) | (value_8[1] << 8) | value_8[0]
                    ])
                }
                
                print(f"✅ 0x{addr:08x}: {desc}")
                print(f"   32-bit: 0x{value_32:08x}")
                
            except Exception as e:
                results[addr] = {
                    'description': desc,
                    'success': False,
                    'error': str(e)
                }
                print(f"❌ 0x{addr:08x}: {desc} - {e}")
        
        self.diagnostic_results['system_registers'] = results
        return results
    
    def test_target_state_dependency(self) -> Dict:
        """Test register access in different target states"""
        
        print(f"\n🔍 TESTING TARGET STATE DEPENDENCY")
        print("=" * 50)
        
        # Test registers from our failed list
        test_registers = [
            0x40001020,  # CLKCTL0 PSCCTL4
            0x40001024,  # CLKCTL0 PSCCTL5
            0x400A5080,  # IOPCTL2 PIO5_0
            0x40411004,  # XSPI2 register
            0x40002000,  # SYSCON0 AHBMATPRIO
        ]
        
        results = {}
        
        for addr in test_registers:
            results[addr] = {}
            
            # Test in running state
            print(f"\nTesting 0x{addr:08x} in different states:")
            
            try:
                # Ensure target is running
                if self.jlink.halted():
                    self.jlink.restart()
                
                time.sleep(0.1)  # Let it run briefly
                
                # Try to read while running
                try:
                    value_running = self.jlink.memory_read32(addr, 1)[0]
                    results[addr]['running_state'] = {
                        'success': True,
                        'value': f"0x{value_running:08x}"
                    }
                    print(f"   Running: ✅ 0x{value_running:08x}")
                except Exception as e:
                    results[addr]['running_state'] = {
                        'success': False,
                        'error': str(e)
                    }
                    print(f"   Running: ❌ {e}")
                
                # Halt target and try again
                self.jlink.halt()
                time.sleep(0.05)
                
                try:
                    value_halted = self.jlink.memory_read32(addr, 1)[0]
                    results[addr]['halted_state'] = {
                        'success': True,
                        'value': f"0x{value_halted:08x}"
                    }
                    print(f"   Halted:  ✅ 0x{value_halted:08x}")
                except Exception as e:
                    results[addr]['halted_state'] = {
                        'success': False,
                        'error': str(e)
                    }
                    print(f"   Halted:  ❌ {e}")
                
            except Exception as e:
                results[addr]['error'] = str(e)
                print(f"   Error: {e}")
        
        self.diagnostic_results['target_state'] = results
        return results
    
    def test_memory_access_widths(self) -> Dict:
        """Test different memory access widths for peripheral registers"""
        
        print(f"\n🔍 TESTING MEMORY ACCESS WIDTHS")
        print("=" * 50)
        
        # Test a mix of registers that should be accessible
        test_registers = [
            0x40001434,  # CLKCTL0 CLKSEL (known working)
            0x40001400,  # CLKCTL0 CLKDIV (known working)
            0x40001020,  # CLKCTL0 PSCCTL4 (failed before)
            0x400A5080,  # IOPCTL2 PIO5_0 (failed before)
            0x40411000,  # XSPI2 MCR (known working)
        ]
        
        results = {}
        
        for addr in test_registers:
            print(f"\nTesting 0x{addr:08x} with different access widths:")
            results[addr] = {}
            
            # Ensure target is halted for consistent access
            if not self.jlink.halted():
                self.jlink.halt()
            
            # Test 32-bit access
            try:
                value_32 = self.jlink.memory_read32(addr, 1)[0]
                results[addr]['32bit'] = {
                    'success': True,
                    'value': f"0x{value_32:08x}"
                }
                print(f"   32-bit: ✅ 0x{value_32:08x}")
            except Exception as e:
                results[addr]['32bit'] = {
                    'success': False,
                    'error': str(e)
                }
                print(f"   32-bit: ❌ {e}")
            
            # Test 16-bit access
            try:
                values_16 = self.jlink.memory_read16(addr, 2)
                combined_16 = (values_16[1] << 16) | values_16[0]
                results[addr]['16bit'] = {
                    'success': True,
                    'values': [f"0x{v:04x}" for v in values_16],
                    'combined': f"0x{combined_16:08x}"
                }
                print(f"   16-bit: ✅ {[f'0x{v:04x}' for v in values_16]} -> 0x{combined_16:08x}")
            except Exception as e:
                results[addr]['16bit'] = {
                    'success': False,
                    'error': str(e)
                }
                print(f"   16-bit: ❌ {e}")
            
            # Test 8-bit access
            try:
                values_8 = self.jlink.memory_read8(addr, 4)
                combined_8 = (values_8[3] << 24) | (values_8[2] << 16) | (values_8[1] << 8) | values_8[0]
                results[addr]['8bit'] = {
                    'success': True,
                    'values': [f"0x{v:02x}" for v in values_8],
                    'combined': f"0x{combined_8:08x}"
                }
                print(f"   8-bit:  ✅ {[f'0x{v:02x}' for v in values_8]} -> 0x{combined_8:08x}")
            except Exception as e:
                results[addr]['8bit'] = {
                    'success': False,
                    'error': str(e)
                }
                print(f"   8-bit:  ❌ {e}")
        
        self.diagnostic_results['access_widths'] = results
        return results
    
    def test_clock_enable_status(self) -> Dict:
        """Test actual clock enable status by reading status registers"""
        
        print(f"\n🔍 TESTING CLOCK ENABLE STATUS")
        print("=" * 50)
        
        # CLKCTL0 PSCCTL status registers (not SET/CLR variants)
        clock_status_registers = {
            0x40001010: "PSCCTL0 - Peripheral Clock Control 0",
            0x40001014: "PSCCTL1 - Peripheral Clock Control 1", 
            0x40001018: "PSCCTL2 - Peripheral Clock Control 2",
            0x4000101C: "PSCCTL3 - Peripheral Clock Control 3",
            0x40001020: "PSCCTL4 - Peripheral Clock Control 4",
            0x40001024: "PSCCTL5 - Peripheral Clock Control 5",
        }
        
        results = {}
        
        # Ensure target is halted
        if not self.jlink.halted():
            self.jlink.halt()
        
        print("Reading CLKCTL0 peripheral clock status registers:")
        
        for addr, desc in clock_status_registers.items():
            try:
                value = self.jlink.memory_read32(addr, 1)[0]
                results[addr] = {
                    'description': desc,
                    'success': True,
                    'value': f"0x{value:08x}",
                    'binary': f"0b{value:032b}"
                }
                
                print(f"✅ 0x{addr:08x}: {desc}")
                print(f"   Value: 0x{value:08x} = 0b{value:032b}")
                
                # Analyze which peripherals are enabled
                enabled_peripherals = []
                for bit in range(32):
                    if value & (1 << bit):
                        enabled_peripherals.append(f"Bit{bit}")
                
                if enabled_peripherals:
                    print(f"   Enabled: {', '.join(enabled_peripherals[:8])}")
                    if len(enabled_peripherals) > 8:
                        print(f"            ... and {len(enabled_peripherals) - 8} more")
                
            except Exception as e:
                results[addr] = {
                    'description': desc,
                    'success': False,
                    'error': str(e)
                }
                print(f"❌ 0x{addr:08x}: {desc} - {e}")
        
        self.diagnostic_results['clock_status'] = results
        return results
    
    def test_mpu_configuration_impact(self) -> Dict:
        """Test if MPU configuration is blocking peripheral access"""
        
        print(f"\n🔍 TESTING MPU CONFIGURATION IMPACT")
        print("=" * 50)
        
        results = {}
        
        # Ensure target is halted
        if not self.jlink.halted():
            self.jlink.halt()
        
        # Read current MPU configuration
        try:
            mpu_ctrl = self.jlink.memory_read32(0xE000ED94, 1)[0]
            mpu_enabled = bool(mpu_ctrl & 0x1)
            
            print(f"MPU Control Register (0xE000ED94): 0x{mpu_ctrl:08x}")
            print(f"MPU Enabled: {mpu_enabled}")
            
            results['mpu_ctrl'] = {
                'value': f"0x{mpu_ctrl:08x}",
                'enabled': mpu_enabled
            }
            
            if mpu_enabled:
                # Read MPU regions
                print(f"\nMPU Region Configuration:")
                
                for region in range(8):  # MIMXRT700 has 8 MPU regions
                    try:
                        # Select region
                        self.jlink.memory_write32(0xE000ED98, [region])  # MPU_RNR
                        
                        # Read region configuration
                        rbar = self.jlink.memory_read32(0xE000ED9C, 1)[0]  # MPU_RBAR
                        rasr = self.jlink.memory_read32(0xE000EDA0, 1)[0]  # MPU_RASR
                        
                        region_enabled = bool(rasr & 0x1)
                        
                        if region_enabled:
                            base_addr = rbar & 0xFFFFFFE0
                            size_field = (rasr >> 1) & 0x1F
                            region_size = 1 << (size_field + 1)
                            
                            print(f"   Region {region}: Base=0x{base_addr:08x}, Size={region_size} bytes")
                            
                            # Check if this region covers peripheral space
                            peripheral_start = 0x40000000
                            peripheral_end = 0x5FFFFFFF
                            
                            if (base_addr <= peripheral_end and 
                                (base_addr + region_size) >= peripheral_start):
                                print(f"     ⚠️  Covers peripheral space!")
                                
                                # Check access permissions
                                ap_field = (rasr >> 24) & 0x7
                                access_perms = {
                                    0: "No access",
                                    1: "Privileged RW",
                                    2: "Privileged RW, User RO", 
                                    3: "Full access",
                                    4: "Reserved",
                                    5: "Privileged RO",
                                    6: "RO",
                                    7: "RO"
                                }
                                
                                print(f"     Access: {access_perms.get(ap_field, 'Unknown')}")
                        
                        results[f'region_{region}'] = {
                            'rbar': f"0x{rbar:08x}",
                            'rasr': f"0x{rasr:08x}",
                            'enabled': region_enabled
                        }
                        
                    except Exception as e:
                        print(f"   Region {region}: Error reading - {e}")
                        results[f'region_{region}'] = {'error': str(e)}
            
        except Exception as e:
            print(f"❌ Error reading MPU configuration: {e}")
            results['error'] = str(e)
        
        self.diagnostic_results['mpu_config'] = results
        return results
    
    def test_alternative_access_methods(self) -> Dict:
        """Test alternative register access methods"""
        
        print(f"\n🔍 TESTING ALTERNATIVE ACCESS METHODS")
        print("=" * 50)
        
        # Test a register that previously failed
        test_addr = 0x400A5080  # IOPCTL2 PIO5_0
        
        results = {}
        
        # Ensure target is halted
        if not self.jlink.halted():
            self.jlink.halt()
        
        print(f"Testing alternative access methods for 0x{test_addr:08x}:")
        
        # Method 1: Direct memory read (our current method)
        try:
            value1 = self.jlink.memory_read32(test_addr, 1)[0]
            results['direct_read'] = {
                'success': True,
                'value': f"0x{value1:08x}"
            }
            print(f"   Direct read: ✅ 0x{value1:08x}")
        except Exception as e:
            results['direct_read'] = {
                'success': False,
                'error': str(e)
            }
            print(f"   Direct read: ❌ {e}")
        
        # Method 2: Memory dump approach
        try:
            # Read a block of memory containing the register
            start_addr = test_addr & 0xFFFFFFF0  # Align to 16-byte boundary
            data = self.jlink.memory_read32(start_addr, 4)  # Read 16 bytes
            
            offset = (test_addr - start_addr) // 4
            value2 = data[offset]
            
            results['memory_dump'] = {
                'success': True,
                'value': f"0x{value2:08x}",
                'full_block': [f"0x{v:08x}" for v in data]
            }
            print(f"   Memory dump: ✅ 0x{value2:08x}")
            
        except Exception as e:
            results['memory_dump'] = {
                'success': False,
                'error': str(e)
            }
            print(f"   Memory dump: ❌ {e}")
        
        # Method 3: With timing delays
        try:
            time.sleep(0.1)  # Add delay
            value3 = self.jlink.memory_read32(test_addr, 1)[0]
            results['delayed_read'] = {
                'success': True,
                'value': f"0x{value3:08x}"
            }
            print(f"   Delayed read: ✅ 0x{value3:08x}")
        except Exception as e:
            results['delayed_read'] = {
                'success': False,
                'error': str(e)
            }
            print(f"   Delayed read: ❌ {e}")
        
        self.diagnostic_results['alternative_methods'] = results
        return results
    
    def run_comprehensive_diagnostics(self) -> Dict:
        """Run all diagnostic tests"""
        
        print("🔍 MIMXRT700 REGISTER ACCESS COMPREHENSIVE DIAGNOSTICS")
        print("=" * 70)
        print(f"J-Link Probe: {self.probe_serial}")
        print(f"Target: {self.target_device}")
        
        if not self.connect():
            return {}
        
        try:
            # Run all diagnostic tests
            self.test_basic_system_registers()
            self.test_target_state_dependency()
            self.test_memory_access_widths()
            self.test_clock_enable_status()
            self.test_mpu_configuration_impact()
            self.test_alternative_access_methods()
            
            # Generate summary
            self.generate_diagnostic_summary()
            
            return self.diagnostic_results
            
        finally:
            if self.jlink:
                self.jlink.close()
    
    def generate_diagnostic_summary(self):
        """Generate comprehensive diagnostic summary"""
        
        print(f"\n📊 DIAGNOSTIC SUMMARY")
        print("=" * 50)
        
        # Count successes and failures across all tests
        total_tests = 0
        successful_tests = 0
        
        for test_category, results in self.diagnostic_results.items():
            if isinstance(results, dict):
                for test_name, test_result in results.items():
                    if isinstance(test_result, dict) and 'success' in test_result:
                        total_tests += 1
                        if test_result['success']:
                            successful_tests += 1
        
        success_rate = (successful_tests / total_tests * 100) if total_tests > 0 else 0
        
        print(f"Overall diagnostic success rate: {successful_tests}/{total_tests} ({success_rate:.1f}%)")
        
        # Identify key findings
        print(f"\n🔍 KEY FINDINGS:")
        
        # System register accessibility
        if 'system_registers' in self.diagnostic_results:
            sys_results = self.diagnostic_results['system_registers']
            sys_success = sum(1 for r in sys_results.values() if r.get('success', False))
            print(f"• System registers: {sys_success}/{len(sys_results)} accessible")
        
        # Clock status
        if 'clock_status' in self.diagnostic_results:
            clk_results = self.diagnostic_results['clock_status']
            clk_success = sum(1 for r in clk_results.values() if r.get('success', False))
            print(f"• Clock status registers: {clk_success}/{len(clk_results)} accessible")
        
        # MPU impact
        if 'mpu_config' in self.diagnostic_results:
            mpu_enabled = self.diagnostic_results['mpu_config'].get('mpu_ctrl', {}).get('enabled', False)
            print(f"• MPU enabled: {mpu_enabled}")
        
        print(f"\n💡 RECOMMENDATIONS:")
        print(f"• Review clock enable status for failed peripherals")
        print(f"• Check MPU region configuration if blocking peripheral access")
        print(f"• Consider target state (halted vs running) for register access")
        print(f"• Verify register access width requirements")

def main():
    """Main diagnostic function"""
    if len(sys.argv) != 2:
        print("Usage: python3 register_access_diagnostics.py <probe_serial>")
        return 1
    
    probe_serial = sys.argv[1]
    target_device = "MIMXRT798S_M33_CORE0"
    
    diagnostics = RegisterAccessDiagnostics(probe_serial, target_device)
    results = diagnostics.run_comprehensive_diagnostics()
    
    # Save results
    with open('register_access_diagnostic_results.json', 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\n✅ Diagnostic results saved to: register_access_diagnostic_results.json")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
