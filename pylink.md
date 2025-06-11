PyLink Tutorial for Embedded ARM Cortex-M Development
📚 Complete Guide for Hardware-in-the-Loop Debugging
This comprehensive tutorial provides practical, tested approaches for using PyLink with ARM Cortex-M embedded targets, based on proven methods used in real embedded projects.

1. Installation and Setup
1.1 Virtual Environment Setup

# Create dedicated virtual environment for embedded projects
python3 -m venv pylink_env
source pylink_env/bin/activate  # Linux/macOS
# pylink_env\Scripts\activate   # Windows

# Upgrade pip
pip install --upgrade pip

1.2 Install PyLink and Dependencies

# Install PyLink Python library
pip install pylink-square

# Install additional dependencies for embedded development
pip install numpy matplotlib  # For data analysis and plotting
pip install pyserial          # For UART communication
pip install intelhex          # For Intel HEX file handling

1.3 J-Link Software Prerequisites
Required J-Link Software:

Download and install SEGGER J-Link Software Pack from: https://www.segger.com/downloads/jlink/
Minimum version: J-Link Software v7.00 or later
Ensure J-Link drivers are properly installed
Verify Installation:

# Test J-Link installation
JLinkExe -CommanderScript /dev/stdin << EOF
ShowEmuList
qc
EOF

1.4 Project Structure

embedded_project/
├── pylink_env/              # Virtual environment
├── scripts/
│   ├── pylink_debug.py      # Main debugging script
│   ├── register_monitor.py  # Register monitoring utilities
│   └── flash_utils.py       # Flash programming utilities
├── binaries/
│   ├── firmware.elf         # ELF binary with debug symbols
│   └── firmware.bin         # Raw binary file
├── config/
│   └── device_config.py     # Device-specific configurations
└── logs/                    # Debug session logs

2. Hardware Connection and Device Configuration
2.1 J-Link Probe Identification

import pylink
import subprocess

def list_available_probes():
    """List all available J-Link probes"""
    try:
        result = subprocess.run(
            ['JLinkExe', '-CommanderScript', '/dev/stdin'],
            input='ShowEmuList\nqc\n',
            capture_output=True, text=True, timeout=10
        )
        
        probes = []
        for line in result.stdout.split('\n'):
            if 'S/N:' in line:
                # Extract serial number
                serial = line.split('S/N:')[1].strip().split()[0]
                probes.append(serial)
        
        return probes
    except Exception as e:
        print(f"Error listing probes: {e}")
        return []

# Example usage
available_probes = list_available_probes()
print(f"Available J-Link probes: {available_probes}")

2.2 Device Naming Conventions

# Device naming patterns for common ARM Cortex-M targets
DEVICE_NAMES = {
    # NXP MIMXRT series
    'MIMXRT798S_M33_CORE0': 'MIMXRT798S_M33_CORE0',  # Correct format
    'MIMXRT798S_M33_0': 'MIMXRT798S_M33_CORE0',      # Alternative format
    'MIMXRT1064': 'MIMXRT1064xxx6A',
    
    # STM32 series
    'STM32F407': 'STM32F407VG',
    'STM32H743': 'STM32H743ZI',
    'STM32L476': 'STM32L476RG',
    
    # Nordic nRF series
    'NRF52840': 'nRF52840_xxAA',
    'NRF5340': 'nRF5340_CPUAPP',
    
    # Microchip SAMD series
    'SAMD21': 'ATSAMD21G18',
    'SAMD51': 'ATSAMD51J19',
}

def get_device_name(target):
    """Get correct device name for J-Link connection"""
    return DEVICE_NAMES.get(target.upper(), target)
    
2.3 Interface Configuration

def configure_interface(jlink, target_type='cortex_m'):
    """Configure J-Link interface for ARM Cortex-M targets"""
    
    # Set interface to SWD (recommended for Cortex-M)
    jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
    
    # Alternative: JTAG interface (if SWD not available)
    # jlink.set_tif(pylink.enums.JLinkInterfaces.JTAG)
    
    # Set SWD speed (optional, auto-detection usually works)
    # jlink.set_speed(4000)  # 4 MHz
    
    # Configure CoreSight for ARM Cortex-M targets
    if target_type == 'cortex_m':
        try:
            jlink.coresight_configure()
        except Exception as e:
            print(f"CoreSight configuration warning: {e}")
            # Continue anyway, often not critical
            
3. Core PyLink Operations

3.1 Connection Management

class PyLinkManager:
    """Robust PyLink connection manager with error handling"""
    
    def __init__(self, probe_serial=None, device_name=None):
        self.probe_serial = int(probe_serial) if probe_serial else None
        self.device_name = device_name
        self.jlink = None
        self.connected = False
    
    def connect(self, verbose=True):
        """Establish connection to J-Link and target"""
        try:
            print(f"🔌 Connecting to J-Link probe {self.probe_serial}...")
            self.jlink = pylink.JLink()
            
            # Open connection with specific probe
            if self.probe_serial:
                self.jlink.open(serial_no=self.probe_serial)
            else:
                self.jlink.open()  # Use first available probe
            
            print(f"✅ Connected to J-Link: {self.jlink.product_name}")
            
            # Configure interface
            configure_interface(self.jlink)
            
            # Connect to target device
            if self.device_name:
                print(f"🎯 Connecting to target: {self.device_name}...")
                self.jlink.connect(self.device_name, verbose=verbose)
                print(f"✅ Connected to {self.device_name}")
                
                # Verify connection
                try:
                    core_id = hex(self.jlink.core_id())
                    print(f"📋 Target Core ID: {core_id}")
                except:
                    print("📋 Target connected successfully")
            
            self.connected = True
            return True
            
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            self.cleanup()
            return False
    
    def cleanup(self):
        """Clean up connection resources"""
        if self.jlink:
            try:
                self.jlink.close()
            except:
                pass
            self.jlink = None
        self.connected = False
        print("🔌 J-Link connection closed")
    
    def __enter__(self):
        """Context manager entry"""
        if not self.connect():
            raise RuntimeError("Failed to establish J-Link connection")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.cleanup()


3.2 Binary Loading and Flash Programming

def load_elf_binary(jlink, elf_path, reset_after_load=True):
    """Load ELF binary onto target with proper error handling"""
    try:
        if not os.path.exists(elf_path):
            raise FileNotFoundError(f"ELF file not found: {elf_path}")
        
        print(f"📥 Loading ELF binary: {elf_path}")
        
        # Reset and halt target
        jlink.reset(halt=True)
        time.sleep(0.2)  # Allow reset to complete
        
        # Flash the ELF file
        jlink.flash_file(elf_path, addr=0x0)
        print(f"✅ ELF binary loaded successfully")
        
        if reset_after_load:
            # Reset and start execution
            jlink.reset(halt=False)
            time.sleep(0.2)
        
        return True
        
    except Exception as e:
        print(f"❌ Failed to load ELF binary: {e}")
        return False

def load_binary_file(jlink, bin_path, flash_addr=0x0):
    """Load raw binary file to specific flash address"""
    try:
        with open(bin_path, 'rb') as f:
            binary_data = f.read()
        
        print(f"📥 Loading binary: {bin_path} to 0x{flash_addr:08x}")
        
        # Reset and halt
        jlink.reset(halt=True)
        time.sleep(0.2)
        
        # Write binary data
        jlink.flash(binary_data, flash_addr)
        print(f"✅ Binary loaded successfully")
        
        # Reset and run
        jlink.reset(halt=False)
        time.sleep(0.2)
        
        return True
        
    except Exception as e:
        print(f"❌ Failed to load binary: {e}")
        return False


3.3 Breakpoint Management

class BreakpointManager:
    """Manage breakpoints with automatic cleanup"""
    
    def __init__(self, jlink):
        self.jlink = jlink
        self.active_breakpoints = set()
    
    def set_breakpoint(self, address, description=""):
        """Set breakpoint at specific address"""
        try:
            self.jlink.breakpoint_set(address)
            self.active_breakpoints.add(address)
            print(f"🎯 Breakpoint set at 0x{address:08x} {description}")
            return True
        except Exception as e:
            print(f"❌ Failed to set breakpoint at 0x{address:08x}: {e}")
            return False
    
    def set_function_breakpoint(self, elf_path, function_name):
        """Set breakpoint at function entry"""
        address = find_function_address(elf_path, function_name)
        if address:
            return self.set_breakpoint(address, f"({function_name})")
        return False
    
    def clear_breakpoint(self, address):
        """Clear specific breakpoint"""
        try:
            self.jlink.breakpoint_clear(address)
            self.active_breakpoints.discard(address)
            print(f"🗑️  Cleared breakpoint at 0x{address:08x}")
            return True
        except Exception as e:
            print(f"❌ Failed to clear breakpoint: {e}")
            return False
    
    def clear_all_breakpoints(self):
        """Clear all active breakpoints"""
        try:
            self.jlink.breakpoint_clear_all()
            self.active_breakpoints.clear()
            print("🗑️  All breakpoints cleared")
            return True
        except Exception as e:
            print(f"❌ Failed to clear breakpoints: {e}")
            return False
    
    def wait_for_breakpoint(self, timeout=10):
        """Wait for breakpoint hit with timeout"""
        start_time = time.time()
        
        print(f"⏳ Waiting for breakpoint hit (timeout: {timeout}s)...")
        while time.time() - start_time < timeout:
            if self.jlink.halted():
                print("✅ Breakpoint hit!")
                return True
            time.sleep(0.1)
        
        print(f"⚠️  Timeout waiting for breakpoint")
        return False
    
    def __del__(self):
        """Cleanup breakpoints on destruction"""
        if hasattr(self, 'jlink') and self.jlink:
            try:
                self.clear_all_breakpoints()
            except:
                pass

def find_function_address(elf_path, function_name):
    """Find function address in ELF file using objdump"""
    try:
        # Use objdump to find function symbols
        cmd = ['objdump', '-t', elf_path]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        if result.returncode != 0:
            print(f"❌ objdump failed: {result.stderr}")
            return None
        
        # Parse objdump output
        for line in result.stdout.split('\n'):
            if function_name in line and ('F' in line or 'FUNC' in line):
                parts = line.split()
                if len(parts) >= 1:
                    try:
                        addr = int(parts[0], 16)
                        print(f"🎯 Found {function_name} at address: 0x{addr:08x}")
                        return addr
                    except ValueError:
                        continue
        
        print(f"⚠️  Function {function_name} not found in {elf_path}")
        return None
        
    except Exception as e:
        print(f"❌ Error finding function address: {e}")
        return None


3.4 Register Access

class RegisterMonitor:
    """ARM Cortex-M register monitoring utilities"""
    
    def __init__(self, jlink):
        self.jlink = jlink
    
    def read_core_registers(self):
        """Read ARM Cortex-M core registers"""
        try:
            registers = {}
            
            # Read general purpose registers (R0-R12)
            for i in range(13):
                reg_value = self.jlink.register_read(i)
                registers[f'R{i}'] = reg_value
            
            # Read special registers with correct naming
            registers['R13 (SP)'] = self.jlink.register_read(13)  # Stack Pointer
            registers['R14 (LR)'] = self.jlink.register_read(14)  # Link Register
            registers['R15 (PC)'] = self.jlink.register_read(15)  # Program Counter
            
            return registers
            
        except Exception as e:
            print(f"❌ Error reading registers: {e}")
            return {}
    
    def read_system_registers(self):
        """Read ARM Cortex-M system control registers"""
        try:
            system_regs = {}
            
            # Read common system control registers
            system_regs['MSP'] = self.jlink.register_read('MSP')    # Main Stack Pointer
            system_regs['PSP'] = self.jlink.register_read('PSP')    # Process Stack Pointer
            system_regs['PRIMASK'] = self.jlink.register_read('PRIMASK')
            system_regs['CONTROL'] = self.jlink.register_read('CONTROL')
            
            return system_regs
            
        except Exception as e:
            print(f"❌ Error reading system registers: {e}")
            return {}
    
    def print_registers(self, registers, title="Register Values"):
        """Pretty print register values"""
        print(f"\n📊 {title}:")
        print("-" * 50)
        
        for reg, value in registers.items():
            if isinstance(value, int):
                print(f"   {reg:12}: 0x{value:08x} ({value:>10})")
            else:
                print(f"   {reg:12}: {value}")
    
    def monitor_registers(self, duration=5, interval=0.5):
        """Monitor register changes over time"""
        print(f"📈 Monitoring registers for {duration}s (interval: {interval}s)")
        
        start_time = time.time()
        previous_regs = None
        
        while time.time() - start_time < duration:
            current_regs = self.read_core_registers()
            
            if previous_regs:
                # Check for changes
                changes = {}
                for reg, value in current_regs.items():
                    if reg in previous_regs and previous_regs[reg] != value:
                        changes[reg] = {
                            'old': previous_regs[reg],
                            'new': value
                        }
                
                if changes:
                    print(f"\n🔄 Register changes at {time.time() - start_time:.1f}s:")
                    for reg, change in changes.items():
                        print(f"   {reg}: 0x{change['old']:08x} → 0x{change['new']:08x}")
            
            previous_regs = current_regs.copy()
            time.sleep(interval)


3.5 Execution Control


def reset_target(jlink, halt=False, delay=0.2):
    """Reset target with configurable halt behavior"""
    try:
        print(f"🔄 Resetting target (halt={halt})...")
        jlink.reset(halt=halt)
        time.sleep(delay)
        
        if halt:
            print("⏸️  Target halted after reset")
        else:
            print("▶️  Target running after reset")
        
        return True
    except Exception as e:
        print(f"❌ Reset failed: {e}")
        return False

def halt_target(jlink):
    """Halt target execution"""
    try:
        jlink.halt()
        print("⏸️  Target halted")
        return True
    except Exception as e:
        print(f"❌ Halt failed: {e}")
        return False

def resume_target(jlink):
    """Resume target execution"""
    try:
        jlink.restart()
        print("▶️  Target resumed")
        return True
    except Exception as e:
        print(f"❌ Resume failed: {e}")
        return False

def step_execution(jlink, steps=1):
    """Single step execution"""
    try:
        for i in range(steps):
            jlink.step()
            print(f"👣 Step {i+1}/{steps} completed")
        return True
    except Exception as e:
        print(f"❌ Step execution failed: {e}")
        return False


4. Complete Working Example
4.1 Comprehensive Debugging Script


#!/usr/bin/env python3
"""
PyLink ARM Cortex-M Debugging Example
====================================

Complete example demonstrating PyLink usage for embedded ARM debugging.
Tested with MIMXRT798S ARM Cortex-M33 target.
#!/usr/bin/env python3
"""
PyLink ARM Cortex-M Debugging Example
====================================

Complete example demonstrating PyLink usage for embedded ARM debugging.
Tested with MIMXRT798S ARM Cortex-M33 target.
"""

import pylink
import time
import sys
import os
import subprocess
from typing import Dict, Optional

class EmbeddedDebugger:
    """Complete embedded debugging solution using PyLink"""
    
    def __init__(self, probe_serial: str, device_name: str):
        self.probe_serial = int(probe_serial)
        self.device_name = device_name
        self.jlink = None
        self.breakpoint_mgr = None
        self.register_mon = None
        self.connected = False
    
    def connect(self):
        """Establish connection to target"""
        try:
            print("🚀 STARTING EMBEDDED DEBUG SESSION")
            print("=" * 60)
            
            # Initialize PyLink
            print(f"🔌 Connecting to J-Link probe {self.probe_serial}...")
            self.jlink = pylink.JLink()
            
            # Open connection
            self.jlink.open(serial_no=self.probe_serial)
            print(f"✅ Connected to J-Link: {self.jlink.product_name}")
            
            # Configure interface for ARM Cortex-M
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            
            # Connect to target
            print(f"🎯 Connecting to target: {self.device_name}...")
            self.jlink.connect(self.device_name, verbose=True)
            print(f"✅ Connected to {self.device_name}")
            
            # Configure CoreSight
            self.jlink.coresight_configure()
            
            # Verify connection
            try:
                core_id = hex(self.jlink.core_id())
                cpu_speed = self.jlink.cpu_speed()
                print(f"📋 Target Core ID: {core_id}")
                print(f"📋 CPU Speed: {cpu_speed} Hz")
            except:
                print("📋 Target connected successfully")
            
            # Initialize managers
            self.breakpoint_mgr = BreakpointManager(self.jlink)
            self.register_mon = RegisterMonitor(self.jlink)
            
            self.connected = True
            return True
            
        except Exception as e:
            print(f"❌ Connection failed: {e}")
            self.cleanup()
            return False
    
    def load_firmware(self, elf_path: str):
        """Load firmware onto target"""
        if not self.connected:
            print("❌ Not connected to target")
            return False
        
        try:
            print(f"\n📥 LOADING FIRMWARE")
            print("-" * 40)
            
            if not os.path.exists(elf_path):
                print(f"❌ ELF file not found: {elf_path}")
                return False
            
            # Reset and halt
            print("🔄 Resetting target...")
            self.jlink.reset(halt=True)
            time.sleep(0.3)
            
            # Flash firmware
            print(f"⚡ Flashing: {os.path.basename(elf_path)}")
            self.jlink.flash_file(elf_path, addr=0x0)
            print("✅ Firmware loaded successfully")
            
            # Reset and start
            self.jlink.reset(halt=False)
            time.sleep(0.3)
            
            return True
            
        except Exception as e:
            print(f"❌ Failed to load firmware: {e}")
            return False
    
    def debug_function(self, elf_path: str, function_name: str, 
                      timeout: int = 15, monitor_duration: int = 5):
        """Debug specific function with comprehensive analysis"""
        try:
            print(f"\n🔍 DEBUGGING FUNCTION: {function_name}")
            print("-" * 50)
            
            # Find function address
            func_addr = self.find_function_address(elf_path, function_name)
            if not func_addr:
                return False
            
            # Set breakpoint
            if not self.breakpoint_mgr.set_breakpoint(func_addr, f"({function_name})"):
                return False
            
            # Reset and run to breakpoint
            print("🔄 Resetting and running to breakpoint...")
            self.jlink.reset(halt=False)
            time.sleep(0.5)
            
            # Wait for breakpoint
            if not self.breakpoint_mgr.wait_for_breakpoint(timeout):
                return False
            
            # Read and analyze registers
            print(f"\n📊 REGISTER ANALYSIS AT {function_name} ENTRY:")
            print("-" * 50)
            
            core_regs = self.register_mon.read_core_registers()
            self.register_mon.print_registers(core_regs, f"{function_name} Entry Registers")
            
            # Analyze function parameters (ARM AAPCS)
            self.analyze_function_parameters(core_regs, function_name)
            
            # Optional: Monitor execution for a period
            if monitor_duration > 0:
                print(f"\n📈 MONITORING EXECUTION ({monitor_duration}s):")
                print("-" * 40)
                self.jlink.restart()  # Continue execution
                self.register_mon.monitor_registers(monitor_duration, 0.5)
            
            return True
            
        except Exception as e:
            print(f"❌ Debug function failed: {e}")
            return False
        
        finally:
            # Clean up breakpoints
            self.breakpoint_mgr.clear_all_breakpoints()
    
    def analyze_function_parameters(self, registers: Dict, function_name: str):
        """Analyze function parameters according to ARM AAPCS"""
        print(f"\n🔬 PARAMETER ANALYSIS ({function_name}):")
        print("-" * 40)
        
        # ARM AAPCS: First 4 parameters in R0-R3
        param_regs = ['R0', 'R1', 'R2', 'R3']
        
        for i, reg in enumerate(param_regs):
            if reg in registers:
                value = registers[reg]
                print(f"   Parameter {i+1} ({reg}): 0x{value:08x} ({value})")
                
                # Check for common corruption patterns
                if value == 0xE0000BBB:
                    print(f"   ⚠️  CORRUPTION DETECTED in {reg}!")
                elif value == 0xDEADBEEF:
                    print(f"   ⚠️  DEBUG PATTERN in {reg}")
                elif value > 0x20000000 and value < 0x30000000:
                    print(f"   📍 Possible RAM address in {reg}")
                elif value > 0x08000000 and value < 0x10000000:
                    print(f"   📍 Possible Flash address in {reg}")
        
        # Check stack pointer
        if 'R13 (SP)' in registers:
            sp_value = registers['R13 (SP)']
            print(f"   Stack Pointer: 0x{sp_value:08x}")
            
            # Validate stack pointer range
            if sp_value < 0x20000000 or sp_value > 0x30000000:
                print(f"   ⚠️  Stack pointer outside expected RAM range!")
    
    def find_function_address(self, elf_path: str, function_name: str) -> Optional[int]:
        """Find function address using objdump"""
        try:
            # Try different objdump variants
            objdump_commands = [
                'arm-none-eabi-objdump',
                'objdump',
                '/usr/bin/objdump'
            ]
            
            for cmd in objdump_commands:
                try:
                    result = subprocess.run(
                        [cmd, '-t', elf_path],
                        capture_output=True, text=True, timeout=30
                    )
                    
                    if result.returncode == 0:
                        break
                except FileNotFoundError:
                    continue
            else:
                print("❌ No suitable objdump found")
                return None
            
            # Parse output
            for line in result.stdout.split('\n'):
                if function_name in line and ('F' in line or 'FUNC' in line):
                    parts = line.split()
                    if len(parts) >= 1:
                        try:
                            addr = int(parts[0], 16)
                            print(f"🎯 Found {function_name} at: 0x{addr:08x}")
                            return addr
                        except ValueError:
                            continue
            
            print(f"⚠️  Function {function_name} not found")
            return None
            
        except Exception as e:
            print(f"❌ Error finding function: {e}")
            return None
    
    def memory_dump(self, start_addr: int, size: int):
        """Dump memory contents"""
        try:
            print(f"\n💾 MEMORY DUMP: 0x{start_addr:08x} - 0x{start_addr + size:08x}")
            print("-" * 60)
            
            data = self.jlink.memory_read(start_addr, size)
            
            # Format as hex dump
            for i in range(0, len(data), 16):
                addr = start_addr + i
                chunk = data[i:i+16]
                
                # Hex representation
                hex_str = ' '.join(f'{b:02x}' for b in chunk)
                hex_str = hex_str.ljust(48)  # Pad to fixed width
                
                # ASCII representation
                ascii_str = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in chunk)
                
                print(f"0x{addr:08x}: {hex_str} |{ascii_str}|")
            
            return data
            
        except Exception as e:
            print(f"❌ Memory dump failed: {e}")
            return None
    
    def run_comprehensive_test(self, elf_path: str, test_functions: list):
        """Run comprehensive debugging test suite"""
        try:
            print("\n🧪 COMPREHENSIVE DEBUG TEST SUITE")
            print("=" * 60)
            
            # Load firmware
            if not self.load_firmware(elf_path):
                return False
            
            results = {}
            
            # Test each function
            for func_name in test_functions:
                print(f"\n{'='*20} TESTING {func_name} {'='*20}")
                
                success = self.debug_function(elf_path, func_name, timeout=10)
                results[func_name] = success
                
                if success:
                    print(f"✅ {func_name} test completed")
                else:
                    print(f"❌ {func_name} test failed")
                
                time.sleep(1)  # Brief pause between tests
            
            # Summary
            print(f"\n📋 TEST SUMMARY:")
            print("-" * 30)
            passed = sum(results.values())
            total = len(results)
            
            for func, result in results.items():
                status = "✅ PASS" if result else "❌ FAIL"
                print(f"   {func}: {status}")
            
            print(f"\nOverall: {passed}/{total} tests passed")
            
            return passed == total
            
        except Exception as e:
            print(f"❌ Comprehensive test failed: {e}")
            return False
    
    def cleanup(self):
        """Clean up resources"""
        if self.breakpoint_mgr:
            self.breakpoint_mgr.clear_all_breakpoints()
        
        if self.jlink:
            try:
                self.jlink.close()
            except:
                pass
            self.jlink = None
        
        self.connected = False
        print("🔌 Debug session closed")
    
    def __enter__(self):
        """Context manager entry"""
        if not self.connect():
            raise RuntimeError("Failed to establish debug connection")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.cleanup()

# Supporting classes (BreakpointManager and RegisterMonitor from previous sections)
class BreakpointManager:
    """Manage breakpoints with automatic cleanup"""
    
    def __init__(self, jlink):
        self.jlink = jlink
        self.active_breakpoints = set()
    
    def set_breakpoint(self, address, description=""):
        """Set breakpoint at specific address"""
        try:
            self.jlink.breakpoint_set(address)
            self.active_breakpoints.add(address)
            print(f"🎯 Breakpoint set at 0x{address:08x} {description}")
            return True
        except Exception as e:
            print(f"❌ Failed to set breakpoint at 0x{address:08x}: {e}")
            return False
    
    def clear_all_breakpoints(self):
        """Clear all active breakpoints"""
        try:
            self.jlink.breakpoint_clear_all()
            self.active_breakpoints.clear()
            return True
        except Exception as e:
            print(f"❌ Failed to clear breakpoints: {e}")
            return False
    
    def wait_for_breakpoint(self, timeout=10):
        """Wait for breakpoint hit with timeout"""
        start_time = time.time()
        
        print(f"⏳ Waiting for breakpoint hit (timeout: {timeout}s)...")
        while time.time() - start_time < timeout:
            if self.jlink.halted():
                print("✅ Breakpoint hit!")
                return True
            time.sleep(0.1)
        
        print(f"⚠️  Timeout waiting for breakpoint")
        return False

class RegisterMonitor:
    """ARM Cortex-M register monitoring utilities"""
    
    def __init__(self, jlink):
        self.jlink = jlink
    
    def read_core_registers(self):
        """Read ARM Cortex-M core registers"""
        try:
            registers = {}
            
            # Read general purpose registers (R0-R12)
            for i in range(13):
                reg_value = self.jlink.register_read(i)
                registers[f'R{i}'] = reg_value
            
            # Read special registers with correct naming
            registers['R13 (SP)'] = self.jlink.register_read(13)  # Stack Pointer
            registers['R14 (LR)'] = self.jlink.register_read(14)  # Link Register
            registers['R15 (PC)'] = self.jlink.register_read(15)  # Program Counter
            
            return registers
            
        except Exception as e:
            print(f"❌ Error reading registers: {e}")
            return {}
    
    def print_registers(self, registers, title="Register Values"):
        """Pretty print register values"""
        print(f"\n📊 {title}:")
        print("-" * 50)
        
        for reg, value in registers.items():
            if isinstance(value, int):
                print(f"   {reg:12}: 0x{value:08x} ({value:>10})")
            else:
                print(f"   {reg:12}: {value}")
    
    def monitor_registers(self, duration=5, interval=0.5):
        """Monitor register changes over time"""
        print(f"📈 Monitoring registers for {duration}s (interval: {interval}s)")
        
        start_time = time.time()
        previous_regs = None
        
        while time.time() - start_time < duration:
            if not self.jlink.halted():
                # Target is running, halt it temporarily to read registers
                self.jlink.halt()
                time.sleep(0.1)
                
                current_regs = self.read_core_registers()
                
                # Resume execution
                self.jlink.restart()
            else:
                current_regs = self.read_core_registers()
            
            if previous_regs:
                # Check for changes
                changes = {}
                for reg, value in current_regs.items():
                    if reg in previous_regs and previous_regs[reg] != value:
                        changes[reg] = {
                            'old': previous_regs[reg],
                            'new': value
                        }
                
                if changes:
                    print(f"\n🔄 Register changes at {time.time() - start_time:.1f}s:")
                    for reg, change in changes.items():
                        print(f"   {reg}: 0x{change['old']:08x} → 0x{change['new']:08x}")
            
            previous_regs = current_regs.copy()
            time.sleep(interval)

def main():
    """Example usage of the embedded debugger"""
    
    # Configuration
    PROBE_SERIAL = "1065679149"  # Your J-Link probe serial
    DEVICE_NAME = "MIMXRT798S_M33_CORE0"  # Target device
    ELF_PATH = "firmware.elf"  # Path to your ELF file
    
    # Functions to test
    TEST_FUNCTIONS = [
        "main",
        "CLOCK_EnableSysOscClk",
        "CLOCK_SetClkDiv",
        "SystemInit"
    ]
    
    try:
        # Use context manager for automatic cleanup
        with EmbeddedDebugger(PROBE_SERIAL, DEVICE_NAME) as debugger:
            
            # Run comprehensive test
            success = debugger.run_comprehensive_test(ELF_PATH, TEST_FUNCTIONS)
            
            if success:
                print("\n🎉 All tests passed!")
                
                # Optional: Additional debugging
                print("\n🔍 Additional Analysis:")
                debugger.memory_dump(0x20000000, 256)  # Dump RAM start
                
            else:
                print("\n❌ Some tests failed!")
                return 1
    
    except KeyboardInterrupt:
        print("\n⚠️  Debug session interrupted by user")
        return 1
    except Exception as e:
        print(f"\n❌ Debug session failed: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())



4.2 Quick Start Script

#!/usr/bin/env python3
"""
Quick PyLink Debug Script
========================

Minimal example for quick debugging tasks.
"""

import pylink
import time

def quick_debug(probe_serial, device_name, elf_path, function_name):
    """Quick function debugging"""
    
    jlink = None
    try:
        # Connect
        print(f"Connecting to {device_name} via probe {probe_serial}...")
        jlink = pylink.JLink()
        jlink.open(serial_no=int(probe_serial))
        jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        jlink.connect(device_name)
        jlink.coresight_configure()
        
        # Load firmware
        print(f"Loading {elf_path}...")
        jlink.reset(halt=True)
        jlink.flash_file(elf_path, addr=0x0)
        jlink.reset(halt=False)
        
        # Set breakpoint (you'll need to find the address)
        # For demo, using a placeholder address
        breakpoint_addr = 0x08001000  # Replace with actual function address
        jlink.breakpoint_set(breakpoint_addr)
        
        # Wait for breakpoint
        print("Waiting for breakpoint...")
        start_time = time.time()
        while time.time() - start_time < 10:
            if jlink.halted():
                print("Breakpoint hit!")
                
                # Read registers
                regs = {}
                for i in range(4):
                    regs[f'R{i}'] = jlink.register_read

