# MIMXRT700 Peripheral Register Monitor

A comprehensive PyLink-based tool for real-time monitoring of peripheral register accesses on the MIMXRT700 embedded platform. This tool provides detailed bit-level analysis of register modifications during firmware execution, based on comprehensive peripheral analysis data.

## Features

- **Real-time Register Monitoring**: Captures register values before and after write operations
- **Bit-level Change Analysis**: Identifies exactly which bits were modified with detailed explanations
- **MIMXRT700-specific Register Definitions**: Comprehensive bit field interpretations for all major peripherals
- **Chronological Execution Tracking**: Maintains execution order matching the peripheral analysis
- **Human-readable Output**: Clear explanations of register modifications and their functional meanings
- **PyLink Integration**: Compatible with SEGGER J-Link probes for hardware-in-the-loop debugging

## Supported Peripherals

The monitor supports detailed bit field analysis for the following MIMXRT700 peripherals:

- **XSPI (XSPI0, XSPI1, XSPI2)**: External Serial Peripheral Interface
  - MCR (Module Configuration Register) with all control bits
- **CLKCTL0**: Clock Control Unit
  - PSCCTL0/1/2 (Peripheral Clock Control registers)
  - CLKSEL (Clock Selection registers)
  - CLKDIV (Clock Divider registers)
- **IOPCTL (IOPCTL0, IOPCTL1, IOPCTL2)**: I/O Pin Control
  - PIO registers with function selection, pull-up/down, drive strength
- **RSTCTL (RSTCTL0, RSTCTL1)**: Reset Control
  - PRSTCTL_CLR (Peripheral Reset Control Clear registers)
- **SYSCON0**: System Control
  - SEC_CLK_CTRL (Security Clock Control)

## Prerequisites

### Hardware Requirements
- SEGGER J-Link probe (any compatible model)
- MIMXRT700 evaluation board or custom target
- Proper SWD/JTAG connection between probe and target

### Software Requirements
```bash
# Install PyLink library
pip install pylink-square

# Install additional dependencies
pip install numpy matplotlib  # For data analysis (optional)
```

### J-Link Software
- Download and install SEGGER J-Link Software Pack v7.00 or later
- Verify installation: `JLinkExe -CommanderScript /dev/stdin`

## Installation

1. Clone or download the monitor script:
```bash
# Download the main script
wget https://path/to/mimxrt700_peripheral_monitor.py

# Make executable
chmod +x mimxrt700_peripheral_monitor.py
```

2. Ensure required files are present:
   - `mimxrt700_peripheral_monitor.py` - Main monitoring script
   - `complete_enhanced_peripheral_analysis.json` - Peripheral analysis data
   - Your firmware ELF file with debug symbols

## Usage

### Basic Usage

Monitor peripheral registers for 30 seconds:
```bash
python mimxrt700_peripheral_monitor.py --probe 1065679149 --elf firmware.elf
```

### Advanced Usage

Load firmware and monitor for 60 seconds:
```bash
python mimxrt700_peripheral_monitor.py \
    --probe 1065679149 \
    --elf firmware.elf \
    --duration 60 \
    --load-firmware \
    --start-execution
```

Monitor with custom analysis file:
```bash
python mimxrt700_peripheral_monitor.py \
    --probe 1065679149 \
    --elf firmware.elf \
    --analysis custom_peripheral_analysis.json \
    --verbose
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--probe, -p` | J-Link probe serial number (required) | - |
| `--device, -d` | Target device name | `MIMXRT798S_M33_CORE0` |
| `--elf, -e` | ELF firmware file path | - |
| `--analysis, -a` | Peripheral analysis JSON file | `complete_enhanced_peripheral_analysis.json` |
| `--duration, -t` | Monitoring duration in seconds | `30` |
| `--load-firmware, -l` | Load firmware before monitoring | `False` |
| `--start-execution, -s` | Start firmware execution | `False` |
| `--verbose, -v` | Enable verbose output | `False` |

## Output Format

The monitor provides detailed output for each register modification:

```
🔄 REGISTER CHANGE DETECTED #1
   Time: 2.45s
   Address: 0x40411000
   Peripheral: XSPI2
   Register: MCR
   Before: 0x00000000 (         0)
   After:  0x00004001 (     16385)
   📝 BIT CHANGES:
      Bit  0: 0 → 1 (SWRSTSD - Software Reset for Serial Flash Memory Domain (0=Deassert, 1=Reset))
      Bit 14: 0 → 1 (MDIS - Module Disable (0=Enable clocks, 1=Allow external logic to disable clocks))
```

## Analysis Data Format

The monitor uses JSON analysis data with the following structure:

```json
{
  "analysis_type": "complete_enhanced_peripheral_access_sequence",
  "total_accesses": 601,
  "chronological_sequence": [
    {
      "sequence_number": 0,
      "address": "0x40001434",
      "peripheral_name": "CLKCTL0",
      "register_name": "CLKSEL",
      "access_type": "function_call_write",
      "execution_phase": "board_init",
      "purpose": "Clock source attachment",
      "source_location": {
        "file": "board.c",
        "function": "BOARD_InitDebugConsole",
        "line": 53
      }
    }
  ]
}
```

## Troubleshooting

### Connection Issues
```bash
# List available J-Link probes
JLinkExe -CommanderScript /dev/stdin << EOF
ShowEmuList
qc
EOF

# Test basic connection
python -c "import pylink; j=pylink.JLink(); j.open(); print('Connected:', j.product_name)"
```

### Common Problems

1. **"No J-Link found"**
   - Verify J-Link software installation
   - Check USB connection
   - Try different USB port

2. **"Target connection failed"**
   - Verify SWD/JTAG wiring
   - Check target power supply
   - Ensure correct device name

3. **"Analysis file not found"**
   - Verify `complete_enhanced_peripheral_analysis.json` exists
   - Check file path and permissions

4. **"Limited register changes detected"**
   - Ensure firmware is actively running
   - Check if target is in low-power mode
   - Verify monitoring addresses are correct

## Limitations

- **Watchpoint Constraints**: ARM Cortex-M cores typically support 4 hardware watchpoints maximum
- **Performance Impact**: Continuous register polling may affect target performance
- **Memory Access**: Some registers may be write-only or have side effects when read
- **Timing Sensitivity**: Very fast register changes might be missed between polling intervals

## Integration with Development Workflow

### With MCUXpresso IDE
1. Build project with debug symbols
2. Export ELF file
3. Run monitor during debugging sessions

### With VS Code + Cortex-Debug
1. Configure launch.json for J-Link debugging
2. Run monitor in parallel terminal
3. Correlate register changes with source code

### Continuous Integration
```bash
# Automated testing script
#!/bin/bash
python mimxrt700_peripheral_monitor.py \
    --probe $JLINK_SERIAL \
    --elf build/firmware.elf \
    --duration 10 \
    --load-firmware \
    --start-execution > register_log.txt

# Analyze results
grep "REGISTER CHANGE" register_log.txt | wc -l
```

## Contributing

To add support for additional peripherals:

1. Add register bit field definitions to `REGISTER_BIT_FIELDS` dictionary
2. Update `get_register_bit_field_key()` method for new peripheral types
3. Test with actual hardware to verify bit field interpretations
4. Submit pull request with documentation updates

## License

This tool is provided as-is for educational and development purposes. Ensure compliance with your organization's policies regarding hardware debugging tools.
