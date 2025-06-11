#!/usr/bin/env python3
"""
MIMXRT798S Register Decoder and Bit Field Analysis Tool

This module provides comprehensive register decoding capabilities for the MIMXRT798S
microcontroller, including bit field analysis, peripheral mapping, and register
value interpretation for the chronological sequence correction tool.

Author: Augment Agent
Date: 2024
"""

import json
import re
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass
from enum import Enum

@dataclass
class BitField:
    """Represents a single bit field within a register"""
    name: str
    bit_position: int
    bit_width: int
    description: str
    reset_value: int = 0
    access: str = "RW"  # RW, RO, WO, etc.

@dataclass
class RegisterDefinition:
    """Complete register definition with bit fields"""
    name: str
    address: int
    offset: int
    description: str
    bit_fields: List[BitField]
    reset_value: int = 0
    access_width: int = 32

@dataclass
class PeripheralDefinition:
    """Peripheral definition with all registers"""
    name: str
    base_address: int
    registers: Dict[str, RegisterDefinition]
    description: str = ""

class AccessType(Enum):
    """Register access types"""
    VOLATILE_READ = "volatile_read"
    VOLATILE_WRITE = "volatile_write"
    FUNCTION_CALL_WRITE = "function_call_write"
    READ_MODIFY_WRITE = "read_modify_write"

class MIMXRT798SRegisterDecoder:
    """
    Comprehensive register decoder for MIMXRT798S microcontroller
    
    Provides bit-level analysis, register mapping, and value interpretation
    for peripheral register accesses in the chronological sequence.
    """
    
    def __init__(self):
        """Initialize the register decoder with MIMXRT798S definitions"""
        self.peripherals: Dict[str, PeripheralDefinition] = {}
        self.address_to_register: Dict[int, Tuple[str, str]] = {}  # address -> (peripheral, register)
        self._initialize_peripheral_definitions()
    
    def _initialize_peripheral_definitions(self):
        """Initialize all peripheral and register definitions"""
        self._init_clkctl0_registers()
        self._init_xspi2_registers()
        self._init_iopctl_registers()
        self._init_rstctl_registers()
        self._init_syscon0_registers()
        self._init_mpu_registers()
        self._init_xcache_registers()
        self._init_gpio_registers()
        self._build_address_mapping()
    
    def _init_clkctl0_registers(self):
        """Initialize CLKCTL0 peripheral registers"""
        clkctl0_base = 0x40001000
        
        # PSCCTL registers (Peripheral Clock Control)
        pscctl_registers = {}
        for i in range(6):  # PSCCTL0-PSCCTL5
            offset = 0x10 + (i * 4)
            pscctl_registers[f"PSCCTL{i}"] = RegisterDefinition(
                name=f"PSCCTL{i}",
                address=clkctl0_base + offset,
                offset=offset,
                description=f"VDD2_COMP Peripheral Clock Control {i}",
                bit_fields=self._get_pscctl_bit_fields(i),
                reset_value=0x00000000
            )
        
        # Clock selection and divider registers
        clkctl0_registers = {
            **pscctl_registers,
            "CLKDIV": RegisterDefinition(
                name="CLKDIV",
                address=clkctl0_base + 0x400,
                offset=0x400,
                description="VDD2_COMP Main Clock Divider",
                bit_fields=[
                    BitField("CLKDIV", 0, 8, "Clock divider value"),
                    BitField("RESET", 29, 1, "Reset divider"),
                    BitField("HALT", 30, 1, "Halt divider"),
                    BitField("REQFLAG", 31, 1, "Request flag")
                ],
                reset_value=0x00000000
            ),
            "CLKSEL": RegisterDefinition(
                name="CLKSEL",
                address=clkctl0_base + 0x434,
                offset=0x434,
                description="VDD2_COMP Main Clock Source Select",
                bit_fields=[
                    BitField("SEL", 0, 3, "Clock source selection"),
                    BitField("FREQMEAS_REF", 8, 1, "Frequency measurement reference"),
                    BitField("FREQMEAS_TARGET", 9, 1, "Frequency measurement target")
                ],
                reset_value=0x00000000
            )
        }
        
        self.peripherals["CLKCTL0"] = PeripheralDefinition(
            name="CLKCTL0",
            base_address=clkctl0_base,
            registers=clkctl0_registers,
            description="Clock Control 0 - VDD2_COMP domain"
        )
    
    def _get_pscctl_bit_fields(self, pscctl_num: int) -> List[BitField]:
        """Get bit field definitions for PSCCTL registers"""
        if pscctl_num == 0:
            return [
                BitField("CODE_CACHE", 1, 1, "Code Cache Clock (XCACHE 1)"),
                BitField("SYSTEM_CACHE", 2, 1, "System Cache Clock (XCACHE 0)"),
                BitField("VDD2_OTP0", 5, 1, "VDD2_OTP_CTRL0 Clock"),
                BitField("SLEEPCON0", 12, 1, "SLEEPCON0 Clock"),
                BitField("SYSCON0", 13, 1, "SYSCON0 Clock"),
                BitField("GLIKEY0", 14, 1, "GLIKEY0 Clock"),
                BitField("GLIKEY3", 15, 1, "GLIKEY3 Clock")
            ]
        elif pscctl_num == 1:
            return [
                BitField("TPIU_TRACE_CLKIN", 2, 1, "TPIU_TRACE_CLKIN Clock"),
                BitField("SWO_TRACECLKIN", 3, 1, "SWO_TRACECLKIN Clock"),
                BitField("TSCLK", 4, 1, "TS Clock"),
                BitField("EDMA0", 5, 1, "eDMA0 Clock"),
                BitField("EDMA1", 6, 1, "eDMA1 Clock"),
                BitField("PKC_RAM_CTRL", 7, 1, "RAM Control Clock of PKC"),
                BitField("PKC", 8, 1, "PKC Clock"),
                BitField("ROMCP", 9, 1, "ROMCP Clock"),
                BitField("XSPI0", 10, 1, "XSPI0 Clock"),
                BitField("XSPI1", 11, 1, "XSPI1 Clock"),
                BitField("CACHE64_0", 12, 1, "CACHE64_0 Clock"),
                BitField("CACHE64_1", 13, 1, "CACHE64_1 Clock"),
                BitField("QK_SUBSYS", 14, 1, "QK SUBSYS (PUF_CTRL) Clock"),
                BitField("MMU0", 16, 1, "MMU0 Clock"),
                BitField("MMU1", 17, 1, "MMU1 Clock"),
                BitField("GPIO0", 18, 1, "GPIO0 Clock"),
                BitField("GPIO1", 19, 1, "GPIO1 Clock"),
                BitField("GPIO2", 20, 1, "GPIO2 Clock"),
                BitField("GPIO3", 21, 1, "GPIO3 Clock"),
                BitField("GPIO4", 22, 1, "GPIO4 Clock"),
                BitField("GPIO5", 23, 1, "GPIO5 Clock"),
                BitField("GPIO6", 24, 1, "GPIO6 Clock"),
                BitField("GPIO7", 25, 1, "GPIO7 Clock"),
                BitField("SCT0", 26, 1, "SCT0 Clock"),
                BitField("CDOG0", 27, 1, "CDOG0 Clock"),
                BitField("CDOG1", 28, 1, "CDOG1 Clock"),
                BitField("CDOG2", 29, 1, "CDOG2 Clock"),
                BitField("LP_FLEXCOMM0", 30, 1, "LP_FLEXCOMM0 Clock"),
                BitField("LP_FLEXCOMM1", 31, 1, "LP_FLEXCOMM1 Clock")
            ]
        elif pscctl_num == 4:
            return [
                BitField("XSPI2", 0, 1, "XSPI2 Clock Enable")
            ]
        elif pscctl_num == 5:
            return [
                BitField("IOPCTL0", 2, 1, "IOPCTL0 Clock Enable"),
                BitField("IOPCTL1", 3, 1, "IOPCTL1 Clock Enable"),
                BitField("IOPCTL2", 5, 1, "IOPCTL2 Clock Enable")
            ]
        else:
            return [BitField(f"BIT_{i}", i, 1, f"Bit {i}") for i in range(32)]
    
    def _init_xspi2_registers(self):
        """Initialize XSPI2 peripheral registers"""
        xspi2_base = 0x40411000
        
        xspi2_registers = {
            "MCR": RegisterDefinition(
                name="MCR",
                address=xspi2_base + 0x0,
                offset=0x0,
                description="Module Configuration Register",
                bit_fields=[
                    BitField("SWRSTSD", 0, 1, "Software Reset for Serial Flash Domain"),
                    BitField("SWRSTAHB", 1, 1, "Software Reset for AHB Domain"),
                    BitField("END_CFG", 2, 2, "Endianness Configuration"),
                    BitField("DQS_EN", 6, 1, "DQS Enable"),
                    BitField("DQS_LAT_EN", 7, 1, "DQS Latency Enable"),
                    BitField("DQS_FA_SEL", 8, 2, "DQS Flash A Selection"),
                    BitField("DQS_FB_SEL", 10, 2, "DQS Flash B Selection"),
                    BitField("MDIS", 14, 1, "Module Disable"),
                    BitField("ISD2FA", 16, 1, "Idle Signal Drive for Flash A Data[2]"),
                    BitField("ISD3FA", 17, 1, "Idle Signal Drive for Flash A Data[3]"),
                    BitField("ISD2FB", 18, 1, "Idle Signal Drive for Flash B Data[2]"),
                    BitField("ISD3FB", 19, 1, "Idle Signal Drive for Flash B Data[3]")
                ],
                reset_value=0x072F01DC
            )
        }
        
        self.peripherals["XSPI2"] = PeripheralDefinition(
            name="XSPI2",
            base_address=xspi2_base,
            registers=xspi2_registers,
            description="Extended Serial Peripheral Interface 2"
        )
    
    def _init_iopctl_registers(self):
        """Initialize IOPCTL peripheral registers"""
        # IOPCTL0 base address
        iopctl0_base = 0x40004000
        iopctl0_registers = {}
        
        # Generate PIO pin registers for IOPCTL0
        for port in range(2):  # Port 0 and 1
            for pin in range(32):
                if port == 0 and pin == 31:  # PIO0_31
                    offset = 0x7C
                elif port == 1 and pin == 0:   # PIO1_0
                    offset = 0x80
                else:
                    continue  # Only include the specific pins we see in the analysis
                
                reg_name = f"PIO{port}_{pin}"
                iopctl0_registers[reg_name] = RegisterDefinition(
                    name=reg_name,
                    address=iopctl0_base + offset,
                    offset=offset,
                    description=f"Pin {port}.{pin} Configuration",
                    bit_fields=self._get_iopctl_bit_fields(),
                    reset_value=0x00000000
                )
        
        self.peripherals["IOPCTL0"] = PeripheralDefinition(
            name="IOPCTL0",
            base_address=iopctl0_base,
            registers=iopctl0_registers,
            description="I/O Pin Control 0"
        )
        
        # IOPCTL2 base address
        iopctl2_base = 0x400A5000
        iopctl2_registers = {}
        
        # Generate PIO pin registers for IOPCTL2 (Port 5)
        for pin in range(32):
            offset = 0x80 + (pin * 4)  # PIO5_0 starts at offset 0x80
            reg_name = f"PIO5_{pin}"
            iopctl2_registers[reg_name] = RegisterDefinition(
                name=reg_name,
                address=iopctl2_base + offset,
                offset=offset,
                description=f"Pin 5.{pin} Configuration",
                bit_fields=self._get_iopctl_bit_fields(),
                reset_value=0x00000000
            )
        
        self.peripherals["IOPCTL2"] = PeripheralDefinition(
            name="IOPCTL2",
            base_address=iopctl2_base,
            registers=iopctl2_registers,
            description="I/O Pin Control 2"
        )
    
    def _get_iopctl_bit_fields(self) -> List[BitField]:
        """Get bit field definitions for IOPCTL pin registers"""
        return [
            BitField("FUNC", 0, 4, "Pin function selection"),
            BitField("PUPDENA", 4, 1, "Pull-up/down enable"),
            BitField("PUPDSEL", 5, 1, "Pull-up/down selection"),
            BitField("IBENA", 6, 1, "Input buffer enable"),
            BitField("SLEW", 7, 1, "Slew rate control"),
            BitField("FULLDRIVE", 8, 1, "Full drive enable"),
            BitField("AMENA", 9, 1, "Analog mux enable"),
            BitField("ODENA", 10, 1, "Open drain enable"),
            BitField("IIENA", 11, 1, "Input invert enable")
        ]

    def _init_rstctl_registers(self):
        """Initialize RSTCTL peripheral registers"""
        rstctl_base = 0x40000000

        rstctl_registers = {
            "PRSTCTL_CLR": RegisterDefinition(
                name="PRSTCTL_CLR",
                address=rstctl_base + 0x70,
                offset=0x70,
                description="Peripheral Reset Control Clear",
                bit_fields=[
                    BitField("IOPCTL0_RST", 2, 1, "IOPCTL0 Reset Clear"),
                    BitField("IOPCTL1_RST", 3, 1, "IOPCTL1 Reset Clear"),
                    BitField("IOPCTL2_RST", 5, 1, "IOPCTL2 Reset Clear"),
                    BitField("GPIO0_RST", 18, 1, "GPIO0 Reset Clear"),
                    BitField("GPIO1_RST", 19, 1, "GPIO1 Reset Clear"),
                    BitField("GPIO2_RST", 20, 1, "GPIO2 Reset Clear"),
                    BitField("GPIO3_RST", 21, 1, "GPIO3 Reset Clear"),
                    BitField("GPIO4_RST", 22, 1, "GPIO4 Reset Clear"),
                    BitField("GPIO5_RST", 23, 1, "GPIO5 Reset Clear"),
                    BitField("GPIO6_RST", 24, 1, "GPIO6 Reset Clear"),
                    BitField("GPIO7_RST", 25, 1, "GPIO7 Reset Clear")
                ],
                reset_value=0x00000000
            )
        }

        self.peripherals["RSTCTL"] = PeripheralDefinition(
            name="RSTCTL",
            base_address=rstctl_base,
            registers=rstctl_registers,
            description="Reset Control"
        )

    def _init_syscon0_registers(self):
        """Initialize SYSCON0 peripheral registers"""
        syscon0_base = 0x40020000

        syscon0_registers = {
            "AHBMATPRIO": RegisterDefinition(
                name="AHBMATPRIO",
                address=syscon0_base + 0x30,
                offset=0x30,
                description="AHB Matrix Priority Control",
                bit_fields=[
                    BitField("PRI_CPU0", 0, 2, "CPU0 Priority"),
                    BitField("PRI_DMA0", 2, 2, "DMA0 Priority"),
                    BitField("PRI_DMA1", 4, 2, "DMA1 Priority"),
                    BitField("PRI_DSP", 6, 2, "DSP Priority"),
                    BitField("PRI_PKC", 8, 2, "PKC Priority"),
                    BitField("PRI_XSPI0", 10, 2, "XSPI0 Priority"),
                    BitField("PRI_XSPI1", 12, 2, "XSPI1 Priority"),
                    BitField("PRI_XSPI2", 14, 2, "XSPI2 Priority")
                ],
                reset_value=0x0000001C
            )
        }

        self.peripherals["SYSCON0"] = PeripheralDefinition(
            name="SYSCON0",
            base_address=syscon0_base,
            registers=syscon0_registers,
            description="System Configuration 0"
        )

    def _init_mpu_registers(self):
        """Initialize MPU peripheral registers"""
        mpu_base = 0xE000ED90

        mpu_registers = {
            "CTRL": RegisterDefinition(
                name="CTRL",
                address=mpu_base + 0x4,
                offset=0x4,
                description="MPU Control Register",
                bit_fields=[
                    BitField("ENABLE", 0, 1, "MPU Enable"),
                    BitField("HFNMIENA", 1, 1, "HardFault and NMI Enable"),
                    BitField("PRIVDEFENA", 2, 1, "Privileged Default Enable")
                ],
                reset_value=0x00000000
            ),
            "RNR": RegisterDefinition(
                name="RNR",
                address=mpu_base + 0x8,
                offset=0x8,
                description="MPU Region Number Register",
                bit_fields=[
                    BitField("REGION", 0, 8, "Region Number")
                ],
                reset_value=0x00000000
            ),
            "RBAR": RegisterDefinition(
                name="RBAR",
                address=mpu_base + 0xC,
                offset=0xC,
                description="MPU Region Base Address Register",
                bit_fields=[
                    BitField("REGION", 0, 4, "Region Number"),
                    BitField("VALID", 4, 1, "Valid"),
                    BitField("ADDR", 5, 27, "Base Address")
                ],
                reset_value=0x00000000
            ),
            "RASR": RegisterDefinition(
                name="RASR",
                address=mpu_base + 0x10,
                offset=0x10,
                description="MPU Region Attribute and Size Register",
                bit_fields=[
                    BitField("ENABLE", 0, 1, "Region Enable"),
                    BitField("SIZE", 1, 5, "Region Size"),
                    BitField("SRD", 8, 8, "Subregion Disable"),
                    BitField("B", 16, 1, "Bufferable"),
                    BitField("C", 17, 1, "Cacheable"),
                    BitField("S", 18, 1, "Shareable"),
                    BitField("TEX", 19, 3, "Type Extension"),
                    BitField("AP", 24, 3, "Access Permission"),
                    BitField("XN", 28, 1, "Execute Never")
                ],
                reset_value=0x00000000
            )
        }

        self.peripherals["MPU"] = PeripheralDefinition(
            name="MPU",
            base_address=mpu_base,
            registers=mpu_registers,
            description="Memory Protection Unit"
        )

    def _init_xcache_registers(self):
        """Initialize XCACHE peripheral registers"""
        xcache0_base = 0x40033000

        xcache_registers = {
            "CCR": RegisterDefinition(
                name="CCR",
                address=xcache0_base + 0x0,
                offset=0x0,
                description="Cache Control Register",
                bit_fields=[
                    BitField("ENCACHE", 0, 1, "Enable Cache"),
                    BitField("ENWRBUF", 1, 1, "Enable Write Buffer"),
                    BitField("INVW0", 24, 1, "Invalidate Way 0"),
                    BitField("INVW1", 25, 1, "Invalidate Way 1"),
                    BitField("PUSHW0", 26, 1, "Push Way 0"),
                    BitField("PUSHW1", 27, 1, "Push Way 1"),
                    BitField("GO", 31, 1, "Initiate Command")
                ],
                reset_value=0x00000000
            )
        }

        self.peripherals["XCACHE0"] = PeripheralDefinition(
            name="XCACHE0",
            base_address=xcache0_base,
            registers=xcache_registers,
            description="Extended Cache 0"
        )

    def _init_gpio_registers(self):
        """Initialize GPIO peripheral registers"""
        gpio0_base = 0x40100000

        gpio_registers = {
            "DIR": RegisterDefinition(
                name="DIR",
                address=gpio0_base + 0x0,
                offset=0x0,
                description="Direction Register",
                bit_fields=[BitField(f"DIR{i}", i, 1, f"Pin {i} Direction") for i in range(32)],
                reset_value=0x00000000
            ),
            "MASK": RegisterDefinition(
                name="MASK",
                address=gpio0_base + 0x80,
                offset=0x80,
                description="Mask Register",
                bit_fields=[BitField(f"MASK{i}", i, 1, f"Pin {i} Mask") for i in range(32)],
                reset_value=0x00000000
            ),
            "PIN": RegisterDefinition(
                name="PIN",
                address=gpio0_base + 0x100,
                offset=0x100,
                description="Pin Register",
                bit_fields=[BitField(f"PIN{i}", i, 1, f"Pin {i} Value") for i in range(32)],
                reset_value=0x00000000
            )
        }

        self.peripherals["GPIO0"] = PeripheralDefinition(
            name="GPIO0",
            base_address=gpio0_base,
            registers=gpio_registers,
            description="General Purpose I/O 0"
        )

    def _build_address_mapping(self):
        """Build address to peripheral/register mapping"""
        for peripheral_name, peripheral in self.peripherals.items():
            for register_name, register in peripheral.registers.items():
                self.address_to_register[register.address] = (peripheral_name, register_name)

    def get_register_info(self, address: int) -> Optional[Tuple[str, str, RegisterDefinition]]:
        """
        Get peripheral name, register name, and definition for an address

        Args:
            address: Register address

        Returns:
            Tuple of (peripheral_name, register_name, register_definition) or None
        """
        if address in self.address_to_register:
            peripheral_name, register_name = self.address_to_register[address]
            register_def = self.peripherals[peripheral_name].registers[register_name]
            return peripheral_name, register_name, register_def
        return None

    def analyze_bit_changes(self, address: int, value_before: int, value_after: int) -> List[Dict[str, Any]]:
        """
        Analyze bit-level changes between two register values

        Args:
            address: Register address
            value_before: Previous register value
            value_after: New register value

        Returns:
            List of bit change dictionaries
        """
        register_info = self.get_register_info(address)
        if not register_info:
            return self._analyze_generic_bit_changes(value_before, value_after)

        peripheral_name, register_name, register_def = register_info
        bit_changes = []

        # Calculate XOR to find changed bits
        changed_bits = value_before ^ value_after

        for bit_field in register_def.bit_fields:
            # Create mask for this bit field
            mask = ((1 << bit_field.bit_width) - 1) << bit_field.bit_position

            # Check if any bits in this field changed
            if changed_bits & mask:
                before_field = (value_before & mask) >> bit_field.bit_position
                after_field = (value_after & mask) >> bit_field.bit_position

                bit_changes.append({
                    "bit_field_name": bit_field.name,
                    "bit_position": bit_field.bit_position,
                    "bit_width": bit_field.bit_width,
                    "value_before": before_field,
                    "value_after": after_field,
                    "description": bit_field.description,
                    "functional_description": f"{bit_field.name} - {bit_field.description}"
                })

        return bit_changes

    def _analyze_generic_bit_changes(self, value_before: int, value_after: int) -> List[Dict[str, Any]]:
        """Analyze bit changes for unknown registers"""
        bit_changes = []
        changed_bits = value_before ^ value_after

        for bit_pos in range(32):
            if changed_bits & (1 << bit_pos):
                before_bit = (value_before >> bit_pos) & 1
                after_bit = (value_after >> bit_pos) & 1

                bit_changes.append({
                    "bit_field_name": f"BIT_{bit_pos}",
                    "bit_position": bit_pos,
                    "bit_width": 1,
                    "value_before": before_bit,
                    "value_after": after_bit,
                    "description": f"Bit {bit_pos}",
                    "functional_description": f"BIT_{bit_pos} - Bit {bit_pos} changed from {before_bit} to {after_bit}"
                })

        return bit_changes

    def decode_register_value(self, address: int, value: int) -> Dict[str, Any]:
        """
        Decode a register value into human-readable format

        Args:
            address: Register address
            value: Register value

        Returns:
            Dictionary with decoded information
        """
        register_info = self.get_register_info(address)
        if not register_info:
            return {
                "address": f"0x{address:08x}",
                "value": f"0x{value:08x}",
                "peripheral": "UNKNOWN",
                "register": "UNKNOWN",
                "decoded_fields": {}
            }

        peripheral_name, register_name, register_def = register_info
        decoded_fields = {}

        for bit_field in register_def.bit_fields:
            mask = ((1 << bit_field.bit_width) - 1) << bit_field.bit_position
            field_value = (value & mask) >> bit_field.bit_position

            decoded_fields[bit_field.name] = {
                "value": field_value,
                "hex": f"0x{field_value:x}",
                "description": bit_field.description,
                "bit_position": bit_field.bit_position,
                "bit_width": bit_field.bit_width
            }

        return {
            "address": f"0x{address:08x}",
            "value": f"0x{value:08x}",
            "peripheral": peripheral_name,
            "register": register_name,
            "description": register_def.description,
            "decoded_fields": decoded_fields
        }

    def get_peripheral_base_address(self, peripheral_name: str) -> Optional[int]:
        """Get base address for a peripheral"""
        if peripheral_name in self.peripherals:
            return self.peripherals[peripheral_name].base_address
        return None

    def get_register_offset(self, address: int) -> Optional[int]:
        """Get register offset from peripheral base address"""
        register_info = self.get_register_info(address)
        if register_info:
            _, _, register_def = register_info
            return register_def.offset
        return None

    def is_clock_gated_register(self, address: int, clock_status: Dict[str, Any]) -> bool:
        """
        Check if a register is clock-gated based on PSCCTL status

        Args:
            address: Register address
            clock_status: Clock status from hardware monitoring

        Returns:
            True if register is clock-gated
        """
        register_info = self.get_register_info(address)
        if not register_info:
            return False

        peripheral_name, _, _ = register_info

        # Check specific clock gating conditions
        if peripheral_name == "IOPCTL2":
            # IOPCTL2 is controlled by PSCCTL2
            pscctl2_value = clock_status.get("PSCCTL2", {}).get("value", "0x00000000")
            return pscctl2_value == "0x00000000"
        elif peripheral_name == "XSPI2":
            # XSPI2 is controlled by PSCCTL4 bit 0
            pscctl4_value = clock_status.get("PSCCTL4", {}).get("value", "0x00000000")
            pscctl4_int = int(pscctl4_value, 16) if isinstance(pscctl4_value, str) else pscctl4_value
            return (pscctl4_int & 0x1) == 0
        elif peripheral_name == "IOPCTL0":
            # IOPCTL0 is controlled by PSCCTL5 bit 2
            pscctl5_value = clock_status.get("PSCCTL5", {}).get("value", "0x00000000")
            pscctl5_int = int(pscctl5_value, 16) if isinstance(pscctl5_value, str) else pscctl5_value
            return (pscctl5_int & 0x4) == 0

        return False
