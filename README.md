# MIMXRT700 Enhanced Peripheral Analysis

[![Analysis Status](https://img.shields.io/badge/analysis-601_accesses-brightgreen.svg)](https://github.com/solfamila/psram)
[![LLVM Version](https://img.shields.io/badge/LLVM-19.1.6-blue.svg)](https://llvm.org/)
[![MCU](https://img.shields.io/badge/MCU-MIMXRT798S-orange.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt700-crossover-mcu-with-arm-cortex-m33-and-dsp-cores:i.MX-RT700)

A comprehensive LLVM-based peripheral register access analysis for the MIMXRT700 XSPI PSRAM example project, providing detailed insights into embedded systems peripheral operations with **601 total peripheral accesses** captured.

## 🎯 Overview

Enhanced peripheral analysis that resolves all verification issues from previous analysis attempts, capturing complete peripheral register access patterns during XSPI PSRAM operations on the MIMXRT700 evaluation board.

## ✨ Key Features

- **✅ Complete Peripheral Coverage**: All 601 peripheral accesses captured
- **✅ Function Call Analysis**: High-level driver function tracking
- **✅ Chronological Execution**: Step-by-step access sequence
- **✅ Real Hardware Addresses**: Actual MIMXRT798S peripheral base addresses
- **✅ Source Traceability**: Complete file/function/line information
- **✅ Verification Issues Resolved**: All previous analysis gaps addressed

## 📊 Analysis Results

### Total Peripheral Accesses: 601
- **Board Initialization**: 130 accesses
- **Driver Initialization**: 68 accesses
- **Runtime Operations**: 403 accesses

### Peripherals Accessed (11 total):
1. **CLKCTL0**: 147 accesses (Clock Control 0)
2. **XSPI2**: 306 accesses (XSPI Controller 2)
3. **IOPCTL2**: 42 accesses (I/O Pin Control 2)
4. **SYSCON0**: 42 accesses (System Control 0)
5. **RSTCTL1**: 35 accesses (Reset Control 1)
6. **IOPCTL0**: 8 accesses (I/O Pin Control 0)
7. **CLKCTL1**: 7 accesses (Clock Control 1)
8. **RSTCTL**: 5 accesses (Reset Control 0)
9. **MPU**: 4 accesses (Memory Protection Unit)
10. **GPIO0**: 3 accesses (General Purpose I/O 0)
11. **XCACHE0**: 2 accesses (Cache Controller 0)

## 🚀 Quick Start

### Prerequisites
- **LLVM/Clang 19.1.6+**
- **ARM GCC toolchain**
- **CMake 3.16+**
- **Python 3.6+**

### Build Enhanced Analysis Pass
```bash
cd llvm_analysis_pass
mkdir build && cd build
cmake .. -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
make
```

### Run Analysis
```bash
# Compile to LLVM IR
clang -target arm-none-eabi -mcpu=cortex-m33 -emit-llvm -S source.c -o source.ll

# Run enhanced analysis
./llvm_analysis_pass/build/bin/peripheral-analyzer source.ll -o analysis.json --chronological

# Combine results
python3 generate_complete_enhanced_analysis.py
```

## 📁 Repository Structure

```
├── complete_enhanced_peripheral_analysis.json    # 601 peripheral accesses
├── enhanced_*_analysis.json                      # Individual analysis files
├── llvm_analysis_pass/                          # Enhanced analysis pass source
│   ├── src/PeripheralAnalysisPass.cpp           # Main analysis implementation
│   ├── include/PeripheralAnalysisPass.h         # Analysis pass header
│   └── CMakeLists.txt                           # Build configuration
├── generate_complete_enhanced_analysis.py       # Analysis combination script
├── mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/  # Project source
├── ENHANCED_ANALYSIS_USAGE.md                   # Complete usage guide
├── ENHANCED_PERIPHERAL_ANALYSIS_REPORT.md       # Technical report
└── VERIFICATION_RESPONSE.md                     # Verification issue resolution
```

## 🔧 Enhanced Features

### Function Call Analysis
- **IOPCTL_PinMuxSet()** - Pin multiplexing (50 accesses captured)
- **RESET_ClearPeripheralReset()** - Reset operations (40 accesses)
- **CLOCK_AttachClk()/CLOCK_SetClkDiv()** - Clock config (154 accesses)
- **ARM_MPU_SetRegion()/ARM_MPU_Enable()** - Memory protection (4 accesses)
- **XCACHE_EnableCache()** - Cache operations (2 accesses)

### Corrected Peripheral Addresses
- **IOPCTL0**: 0x40004000 (real MIMXRT798S address)
- **IOPCTL1**: 0x40064000 (real MIMXRT798S address)
- **IOPCTL2**: 0x400A5000 (real MIMXRT798S address)
- **All peripherals**: Verified against hardware specifications

### Chronological Execution Order
- **Sequence numbering** for all accesses
- **Execution phase classification** (board_init, driver_init, runtime)
- **Complete source traceability** with file/function/line information

## 📚 Documentation

- **[📖 Usage Guide](ENHANCED_ANALYSIS_USAGE.md)** - Complete setup and usage
- **[📋 Technical Report](ENHANCED_PERIPHERAL_ANALYSIS_REPORT.md)** - Detailed methodology
- **[✅ Verification Response](VERIFICATION_RESPONSE.md)** - Issue resolution
- **[⚡ Quick LLVM Guide](QUICK_LLVM_BUILD_GUIDE.md)** - Fast LLVM setup

## 🎯 Verification Issues Resolved

✅ **Title Accuracy**: Now correctly reports 601 accesses (not 524/430)
✅ **IOPCTL Coverage**: All pin mux operations captured
✅ **RESET Coverage**: All reset operations captured
✅ **Clock Coverage**: Comprehensive clock configuration captured
✅ **MPU/Cache Coverage**: System configuration operations captured
✅ **Address Accuracy**: Real MIMXRT798S peripheral addresses used
✅ **Chronological Order**: Proper execution sequence maintained

## 🔍 Output Format

The enhanced analysis generates JSON files with chronological peripheral access data:

```json
{
  "analysis_type": "complete_enhanced_peripheral_access_sequence",
  "description": "Complete peripheral register accesses - 601 total accesses",
  "total_accesses": 601,
  "execution_phase_summary": {
    "board_init": 130,
    "driver_init": 68,
    "runtime": 403
  },
  "peripheral_summary": {
    "CLKCTL0": 147,
    "XSPI2": 306,
    "IOPCTL2": 42,
    ...
  },
  "chronological_sequence": [...]
}
```

## 🤝 Contributing

Contributions welcome! Please open issues for bugs or feature requests.

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/improvement`)
3. Commit your changes (`git commit -am 'Add improvement'`)
4. Push to the branch (`git push origin feature/improvement`)
5. Create a Pull Request

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **LLVM Project** - For the excellent compiler infrastructure
- **NXP Semiconductors** - For the MIMXRT700 MCU and MCUXpresso SDK
- **ARM** - For the Cortex-M33 architecture

## 📞 Support

For questions about the MIMXRT700 analysis or LLVM peripheral analysis pass, please open an issue in this repository.

---

**Note**: This repository contains enhanced peripheral analysis tools and results. The analysis captures comprehensive peripheral register access patterns for embedded systems development and verification.
