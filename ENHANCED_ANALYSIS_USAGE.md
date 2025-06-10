# Enhanced Peripheral Analysis Usage Guide

## Overview

This repository contains an enhanced LLVM analysis pass that provides comprehensive peripheral register access analysis for embedded systems projects, specifically targeting the MIMXRT700 XSPI PSRAM example.

## Prerequisites

### Required Tools
- **LLVM/Clang 19.1.6** (or compatible version)
- **CMake 3.16+**
- **Python 3.6+**
- **ARM GCC toolchain** (for cross-compilation)

### LLVM Installation
You need to build LLVM from source or install a compatible version. The analysis pass requires LLVM development headers and libraries.

## Quick Start

### 1. Build the Enhanced Analysis Pass

```bash
cd llvm_analysis_pass
mkdir -p build
cd build
cmake .. -DLLVM_DIR=/path/to/your/llvm/lib/cmake/llvm
make
```

### 2. Compile Your Project to LLVM IR

For embedded projects, compile each source file to LLVM IR:

```bash
# Example for MIMXRT700 project
clang -target arm-none-eabi -mcpu=cortex-m33 \
      -I/path/to/sdk/headers \
      -emit-llvm -S -O2 \
      your_source.c -o your_source.ll
```

### 3. Run Enhanced Peripheral Analysis

```bash
# Analyze individual IR files
./llvm_analysis_pass/build/bin/peripheral-analyzer input.ll -o output.json --chronological

# For multiple files, analyze each separately then combine
./llvm_analysis_pass/build/bin/peripheral-analyzer board.ll -o board_analysis.json --chronological
./llvm_analysis_pass/build/bin/peripheral-analyzer pin_mux.ll -o pin_mux_analysis.json --chronological
./llvm_analysis_pass/build/bin/peripheral-analyzer clock_config.ll -o clock_analysis.json --chronological
```

### 4. Combine Analysis Results

```bash
# Use the provided Python script to combine all analysis files
python3 generate_complete_enhanced_analysis.py
```

## Enhanced Features

### Function Call Analysis
The enhanced analysis pass now captures high-level driver function calls:

- **IOPCTL_PinMuxSet()** - Pin multiplexing configuration
- **RESET_ClearPeripheralReset()** - Reset controller operations  
- **CLOCK_AttachClk()** / **CLOCK_SetClkDiv()** - Clock configuration
- **ARM_MPU_SetRegion()** / **ARM_MPU_Enable()** - Memory protection
- **XCACHE_EnableCache()** - Cache controller operations

### Corrected Peripheral Definitions
- **Real MIMXRT798S peripheral base addresses**
- **Comprehensive register mappings**
- **Accurate peripheral coverage**

### Chronological Execution Order
- **Sequence numbering** for all accesses
- **Execution phase classification** (board_init, driver_init, runtime)
- **Call stack information** for traceability

## Output Format

The enhanced analysis generates JSON files with the following structure:

```json
{
  "analysis_type": "complete_enhanced_peripheral_access_sequence",
  "description": "Complete peripheral register accesses in chronological execution order - N total accesses",
  "total_accesses": N,
  "execution_phase_summary": {
    "board_init": X,
    "driver_init": Y, 
    "runtime": Z
  },
  "peripheral_summary": {
    "PERIPHERAL_NAME": count,
    ...
  },
  "chronological_sequence": [
    {
      "access_type": "function_call_write|volatile_read|volatile_write",
      "address": "0xHEXADDR",
      "peripheral_name": "PERIPHERAL_NAME",
      "register_name": "REGISTER_NAME", 
      "purpose": "Description of operation",
      "sequence_number": N,
      "execution_phase": "board_init|driver_init|runtime",
      "source_location": {
        "file": "source_file.c",
        "function": "function_name", 
        "line": N
      },
      ...
    }
  ]
}
```

## Files in This Repository

### Core Analysis Pass
- `llvm_analysis_pass/src/PeripheralAnalysisPass.cpp` - Enhanced analysis implementation
- `llvm_analysis_pass/include/PeripheralAnalysisPass.h` - Analysis pass header
- `llvm_analysis_pass/CMakeLists.txt` - Build configuration

### Analysis Results
- `complete_enhanced_peripheral_analysis.json` - Complete 601-access analysis
- `enhanced_*_analysis.json` - Individual IR file analysis results

### Utilities
- `generate_complete_enhanced_analysis.py` - Script to combine analysis results
- `ENHANCED_PERIPHERAL_ANALYSIS_REPORT.md` - Detailed technical report
- `VERIFICATION_RESPONSE.md` - Response to verification issues

## Customization

### Adding New Peripheral Types
Edit `PeripheralAnalysisPass.cpp` and add peripheral definitions in `initializePeripherals()`:

```cpp
PeripheralInfo newPeripheral;
newPeripheral.name = "NEW_PERIPHERAL";
newPeripheral.baseAddress = 0x40000000;
newPeripheral.registers[0x40000000] = "CONTROL_REG";
peripherals["NEW_PERIPHERAL"] = newPeripheral;
```

### Adding New Function Analysis
Add function analyzers in `analyzeFunctionCall()`:

```cpp
else if (functionName == "YOUR_FUNCTION") {
    analyzeYourFunction(CI);
}
```

## Troubleshooting

### Build Issues
- Ensure LLVM_DIR points to your LLVM installation's cmake directory
- Verify LLVM development headers are installed
- Check CMake version compatibility

### Analysis Issues  
- Verify IR files are generated correctly with debug information
- Check that peripheral base addresses match your target device
- Ensure function names match exactly (case-sensitive)

## Support

For issues specific to the MIMXRT700 analysis:
- Check peripheral base addresses against your device datasheet
- Verify SDK header file paths in compilation
- Ensure cross-compilation targets match your device architecture

## License

This enhanced analysis pass builds upon LLVM infrastructure and follows the same licensing terms.
