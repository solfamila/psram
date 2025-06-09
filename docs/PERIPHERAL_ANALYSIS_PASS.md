# MIMXRT700 XSPI PSRAM Peripheral Analysis Pass

## Overview

The Peripheral Analysis Pass is a custom LLVM analysis pass designed to identify and document all peripheral register accesses in the MIMXRT700 XSPI PSRAM project. This pass provides comprehensive analysis of memory-mapped I/O (MMIO) operations, helping developers understand peripheral usage patterns, optimize performance, and ensure security compliance.

## Features

### Analysis Capabilities

1. **Memory-Mapped I/O Detection**
   - Direct pointer dereferences to peripheral base addresses
   - Volatile memory accesses in peripheral address space
   - Register access patterns through SDK macros and functions
   - Bitfield operations on peripheral registers

2. **Access Classification**
   - **Read Operations**: Load instructions from peripheral registers
   - **Write Operations**: Store instructions to peripheral registers  
   - **Read-Modify-Write**: Atomic operations and complex bit manipulations

3. **Detailed Information Capture**
   - **Register Address**: Full 32-bit address and symbolic name
   - **Data Size**: 8-bit, 16-bit, or 32-bit access width
   - **Bit Manipulation**: Specific bits being set, cleared, or modified
   - **Source Location**: File name, function name, and line number
   - **Context Analysis**: Purpose and intent of each access

### Supported Peripherals

The pass currently supports analysis of the following MIMXRT700 peripherals:

- **XSPI0** (Secure): Base address 0x50184000
- **XSPI0_NS** (Non-Secure): Base address 0x40184000  
- **XSPI1**: Base address 0x40185000
- **XSPI2**: Base address 0x40411000
- **GPIO0**: Base address 0x40100000
- **POWER**: Base address 0x40020000

### Register Coverage

For XSPI peripherals, the pass recognizes these key registers:
- **MCR**: Module Configuration Register
- **IPCR**: IP Configuration Register
- **FLSHCR**: Flash Memory Configuration Register
- **BUFCR0-3**: Buffer Configuration Registers
- **BFGENCR**: Buffer Generic Configuration Register
- **SOCCR**: SOC Configuration Register
- **SFAR**: Serial Flash Memory Address Register
- **SFACR**: Serial Flash Memory Address Configuration Register
- **SMPR**: Sampling Register
- **LUTKEY**: LUT Key Register
- **LCKCR**: LUT Lock Configuration Register

## Build Instructions

### Prerequisites

- LLVM 19.1.6 (built and installed in workspace)
- CMake 3.16 or later
- C++17 compatible compiler

### Building the Pass

```bash
cd llvm_analysis_pass/build
export LLVM_INSTALL_DIR="/path/to/llvm-install"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

This creates:
- `lib/libPeripheralAnalysisPass.so`: Shared library for the pass
- `bin/peripheral-analyzer`: Standalone analysis tool

## Usage

### Standalone Tool

```bash
./peripheral-analyzer input.ll -o analysis_results.json -v
```

**Parameters:**
- `input.ll`: LLVM IR file to analyze
- `-o filename`: Output JSON file (default: peripheral_analysis.json)
- `-v`: Enable verbose output

### LLVM Pass Integration

```bash
# Using opt tool
opt -load-pass-plugin=./lib/libPeripheralAnalysisPass.so \
    -passes="peripheral-analysis" \
    input.ll -disable-output

# Using legacy pass manager
opt -load ./lib/libPeripheralAnalysisPass.so \
    -peripheral-analysis \
    input.ll -disable-output
```

### Generating LLVM IR

To analyze C source files, first generate LLVM IR:

```bash
clang -S -emit-llvm -g -O1 source.c -o source.ll
```

## Output Format

The pass generates structured JSON output with the following schema:

```json
{
  "peripheral_accesses": [
    {
      "peripheral_name": "XSPI0",
      "base_address": "0x50184000",
      "accesses": [
        {
          "register_name": "MCR",
          "address": "0x50184000",
          "access_type": "write",
          "data_size": 32,
          "bits_modified": ["bit_0", "bit_4-7"],
          "source_location": {
            "file": "fsl_xspi.c",
            "function": "XSPI_Init",
            "line": 245
          },
          "purpose": "Initialize XSPI controller"
        }
      ]
    }
  ]
}
```

### Field Descriptions

- **peripheral_name**: Name of the peripheral (e.g., "XSPI0", "GPIO0")
- **base_address**: Base address of the peripheral in hexadecimal
- **register_name**: Symbolic name of the register or offset-based name
- **address**: Full 32-bit register address
- **access_type**: "read", "write", or "read-modify-write"
- **data_size**: Access width in bits (8, 16, or 32)
- **bits_modified**: Array of affected bit positions or ranges
- **source_location**: Debug information about the access location
- **purpose**: Inferred purpose based on context analysis

## Analysis Reports

### Report Generation

Use the included Python script to generate human-readable reports:

```bash
python3 peripheral_analysis_report.py analysis_results.json report.txt
```

### Report Sections

1. **Executive Summary**: High-level statistics
2. **Peripheral Access Summary**: Breakdown by peripheral
3. **Most Accessed Registers**: Frequency analysis
4. **Detailed Analysis**: Complete access information
5. **Security Analysis**: TrustZone and secure access patterns
6. **Performance Analysis**: Optimization recommendations

## Integration with Build System

### CMake Integration

Add to your CMakeLists.txt:

```cmake
include(cmake/PeripheralAnalysis.cmake)
add_peripheral_analysis(your_target_name)
```

This automatically:
- Generates LLVM IR during build
- Runs peripheral analysis
- Creates JSON and text reports

### Manual Integration

For existing projects, add analysis as a post-build step:

```bash
# Generate IR
clang -S -emit-llvm [compile_flags] source.c -o source.ll

# Run analysis
./peripheral-analyzer source.ll -o analysis.json

# Generate report
python3 peripheral_analysis_report.py analysis.json report.txt
```

## Security Considerations

### TrustZone Analysis

The pass distinguishes between secure and non-secure peripheral accesses:
- Secure peripherals (0x50000000-0x5FFFFFFF)
- Non-secure peripherals (0x40000000-0x4FFFFFFF)

### Security Recommendations

1. **Access Pattern Review**: Validate that secure peripherals are only accessed from secure code
2. **Permission Verification**: Ensure proper TrustZone configuration
3. **Audit Trail**: Use analysis results for security audits

## Performance Optimization

### Identified Patterns

The pass helps identify:
- **Excessive Read-Modify-Write Operations**: Potential for optimization
- **Frequent Register Access**: Candidates for caching
- **Inefficient Bit Manipulation**: Opportunities for batching

### Optimization Strategies

1. **Register Caching**: Cache frequently read registers
2. **Batch Updates**: Combine multiple bit operations
3. **Access Ordering**: Optimize register access sequences

## Limitations

1. **Static Analysis Only**: Cannot detect runtime-computed addresses
2. **SDK Function Calls**: Limited visibility into library functions
3. **Indirect Access**: May miss function pointer-based accesses
4. **Compiler Optimizations**: Some accesses may be optimized away

## Future Enhancements

1. **Extended Peripheral Support**: Add more MIMXRT700 peripherals
2. **Dynamic Analysis**: Runtime access tracking
3. **Performance Metrics**: Timing and frequency analysis
4. **Visualization**: Graphical access pattern representation
5. **Integration**: IDE plugins and CI/CD integration

## Troubleshooting

### Common Issues

1. **Missing Headers**: Ensure all include paths are specified
2. **Compilation Errors**: Check LLVM version compatibility
3. **Empty Results**: Verify peripheral address ranges
4. **Debug Info**: Use `-g` flag for source location information

### Debug Mode

Enable verbose output for troubleshooting:

```bash
./peripheral-analyzer input.ll -o output.json -v
```

## Contributing

To extend the pass for additional peripherals:

1. Add peripheral definitions in `initializePeripheralDefinitions()`
2. Update address range checks in `isPeripheralAddress()`
3. Add register mappings and symbolic names
4. Test with representative code samples

## License

This peripheral analysis pass is provided under the same license terms as the MIMXRT700 XSPI PSRAM project.
