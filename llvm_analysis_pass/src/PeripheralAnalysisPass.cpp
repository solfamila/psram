//===-- PeripheralAnalysisPass.cpp - MIMXRT700 Peripheral Analysis -----===//
//
// MIMXRT700 XSPI PSRAM Peripheral Register Access Analysis Pass Implementation
//
//===----------------------------------------------------------------------===//

#include "PeripheralAnalysisPass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace llvm;

char PeripheralAnalysisLegacyPass::ID = 0;

// Register the pass
static RegisterPass<PeripheralAnalysisLegacyPass> X(
    "peripheral-analysis", 
    "MIMXRT700 Peripheral Register Access Analysis Pass",
    false, // Only looks at CFG
    true   // Analysis Pass
);

PreservedAnalyses PeripheralAnalysisPass::run(Module &M, ModuleAnalysisManager &AM) {
    // Initialize peripheral definitions
    initializePeripheralDefinitions();

    // Initialize execution order tracking (only if first module)
    if (registerAccesses.empty()) {
        initializeExecutionPhaseMapping();
        initializeFunctionExecutionOrder();
        globalSequenceCounter = 0;
    }

    // Don't clear previous results for multi-module analysis
    // registerAccesses.clear(); // Commented out for multi-module support

    // Analyze each function in the module in a specific order to maintain execution sequence
    // First analyze board initialization functions
    // Find main function and analyze in execution order
    Function *mainFunction = M.getFunction("main");
    if (mainFunction) {
        // Start from main and follow call graph in execution order
        analyzeFunctionInExecutionOrder(mainFunction);
    } else {
        // Fallback: analyze all functions if main not found
        for (Function &F : M) {
            if (!F.isDeclaration()) {
                analyzeFunction(F);
            }
        }
    }

    return PreservedAnalyses::all();
}

bool PeripheralAnalysisLegacyPass::runOnModule(Module &M) {
    ModuleAnalysisManager DummyMAM;
    Pass.run(M, DummyMAM);
    return false; // We don't modify the module
}

void PeripheralAnalysisPass::initializePeripheralDefinitions() {
    // REAL MIMXRT798S PERIPHERAL DEFINITIONS
    // Extracted from official NXP device header files
    // Base addresses verified from MIMXRT798S_cm33_core0_COMMON.h

    // CLKCTL0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo clkctl0;
    clkctl0.name = "CLKCTL0";
    clkctl0.baseAddress = 0x40001000;
    clkctl0.registers[0x40001010] = "PSCCTL0";
    clkctl0.registers[0x40001014] = "PSCCTL1";
    clkctl0.registers[0x40001018] = "PSCCTL2";
    clkctl0.registers[0x4000101C] = "PSCCTL3";
    clkctl0.registers[0x40001020] = "PSCCTL4";
    clkctl0.registers[0x40001024] = "PSCCTL5";
    clkctl0.registers[0x40001040] = "PSCCTL0_SET";
    clkctl0.registers[0x40001044] = "PSCCTL1_SET";
    clkctl0.registers[0x40001048] = "PSCCTL2_SET";
    clkctl0.registers[0x4000104C] = "PSCCTL3_SET";
    clkctl0.registers[0x40001050] = "PSCCTL4_SET";
    clkctl0.registers[0x40001054] = "PSCCTL5_SET";
    clkctl0.registers[0x40001070] = "PSCCTL0_CLR";
    clkctl0.registers[0x40001074] = "PSCCTL1_CLR";
    clkctl0.registers[0x40001078] = "PSCCTL2_CLR";
    clkctl0.registers[0x4000107C] = "PSCCTL3_CLR";
    clkctl0.registers[0x40001080] = "PSCCTL4_CLR";
    clkctl0.registers[0x40001084] = "PSCCTL5_CLR";
    clkctl0.registers[0x40001400] = "MAINCLKDIV";
    clkctl0.registers[0x40001434] = "MAINCLKSEL";
    clkctl0.registers[0x40001600] = "XSPI0FCLKSEL";
    clkctl0.registers[0x40001604] = "XSPI0FCLKDIV";
    clkctl0.registers[0x40001620] = "XSPI1FCLKSEL";
    clkctl0.registers[0x40001624] = "XSPI1FCLKDIV";
    peripherals["CLKCTL0"] = clkctl0;

    // SYSCON0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo syscon0;
    syscon0.name = "SYSCON0";
    syscon0.baseAddress = 0x40002000;
    syscon0.registers[0x40002000] = "AHBMATPRIO";
    syscon0.registers[0x40002010] = "SYSTCKCAL";
    syscon0.registers[0x40002020] = "NMISRC";
    syscon0.registers[0x40002024] = "ASYNCAPBCTRL";
    syscon0.registers[0x40002030] = "PIOPORCAP0";
    syscon0.registers[0x40002034] = "PIOPORCAP1";
    syscon0.registers[0x40002040] = "PIORESCAP0";
    syscon0.registers[0x40002044] = "PIORESCAP1";
    syscon0.registers[0x40002050] = "PRESETCTRL0";
    syscon0.registers[0x40002054] = "PRESETCTRL1";
    syscon0.registers[0x40002058] = "PRESETCTRL2";
    peripherals["SYSCON0"] = syscon0;

    // RSTCTL0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo rstctl0;
    rstctl0.name = "RSTCTL0";
    rstctl0.baseAddress = 0x40000000;
    rstctl0.registers[0x40000000] = "PRSTCTL0";
    rstctl0.registers[0x40000004] = "PRSTCTL1";
    rstctl0.registers[0x40000008] = "PRSTCTL2";
    rstctl0.registers[0x4000000C] = "PRSTCTL3";
    rstctl0.registers[0x40000010] = "PRSTCTL4";
    rstctl0.registers[0x40000020] = "PRSTCTLSET0";
    rstctl0.registers[0x40000024] = "PRSTCTLSET1";
    rstctl0.registers[0x40000028] = "PRSTCTLSET2";
    rstctl0.registers[0x4000002C] = "PRSTCTLSET3";
    rstctl0.registers[0x40000030] = "PRSTCTLSET4";
    rstctl0.registers[0x40000040] = "PRSTCTLCLR0";
    rstctl0.registers[0x40000044] = "PRSTCTLCLR1";
    rstctl0.registers[0x40000048] = "PRSTCTLCLR2";
    rstctl0.registers[0x4000004C] = "PRSTCTLCLR3";
    rstctl0.registers[0x40000050] = "PRSTCTLCLR4";
    peripherals["RSTCTL0"] = rstctl0;

    // GPIO0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo gpio0;
    gpio0.name = "GPIO0";
    gpio0.baseAddress = 0x40100000;
    gpio0.registers[0x40100000] = "PDOR";
    gpio0.registers[0x40100004] = "PSOR";
    gpio0.registers[0x40100008] = "PCOR";
    gpio0.registers[0x4010000C] = "PTOR";
    gpio0.registers[0x40100010] = "PDIR";
    gpio0.registers[0x40100014] = "PDDR";
    gpio0.registers[0x40100018] = "PIDR";
    peripherals["GPIO0"] = gpio0;

    // XSPI2 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo xspi2;
    xspi2.name = "XSPI2";
    xspi2.baseAddress = 0x40411000;
    xspi2.registers[0x40411000] = "MCR";
    xspi2.registers[0x40411008] = "IPCR";
    xspi2.registers[0x4041100C] = "FLSHCR";
    xspi2.registers[0x40411010] = "BUFCR0";
    xspi2.registers[0x40411014] = "BUFCR1";
    xspi2.registers[0x40411018] = "BUFCR2";
    xspi2.registers[0x4041101C] = "BUFCR3";
    xspi2.registers[0x40411020] = "BFGENCR";
    xspi2.registers[0x40411024] = "SOCCR";
    xspi2.registers[0x40411100] = "SFAR";
    xspi2.registers[0x40411104] = "SFACR";
    xspi2.registers[0x40411108] = "SMPR";
    xspi2.registers[0x40411300] = "LUTKEY";
    xspi2.registers[0x40411304] = "LCKCR";
    peripherals["XSPI2"] = xspi2;

    // XSPI0 Non-Secure
    PeripheralInfo xspi0_ns;
    xspi0_ns.name = "XSPI0_NS";
    xspi0_ns.baseAddress = 0x40184000;
    xspi0_ns.registers[0x40184000] = "MCR";
    xspi0_ns.registers[0x40184008] = "IPCR";
    xspi0_ns.registers[0x4018400C] = "FLSHCR";
    xspi0_ns.registers[0x40184010] = "BUFCR0";
    xspi0_ns.registers[0x40184014] = "BUFCR1";
    xspi0_ns.registers[0x40184018] = "BUFCR2";
    xspi0_ns.registers[0x4018401C] = "BUFCR3";
    xspi0_ns.registers[0x40184020] = "BFGENCR";
    xspi0_ns.registers[0x40184024] = "SOCCR";
    xspi0_ns.registers[0x40184100] = "SFAR";
    xspi0_ns.registers[0x40184104] = "SFACR";
    xspi0_ns.registers[0x40184108] = "SMPR";
    xspi0_ns.registers[0x40184300] = "LUTKEY";
    xspi0_ns.registers[0x40184304] = "LCKCR";
    peripherals["XSPI0_NS"] = xspi0_ns;
    
    // XSPI1 Peripheral
    PeripheralInfo xspi1;
    xspi1.name = "XSPI1";
    xspi1.baseAddress = 0x40185000;
    xspi1.registers[0x40185000] = "MCR";
    xspi1.registers[0x40185008] = "IPCR";
    xspi1.registers[0x4018500C] = "FLSHCR";
    xspi1.registers[0x40185010] = "BUFCR0";
    xspi1.registers[0x40185014] = "BUFCR1";
    xspi1.registers[0x40185018] = "BUFCR2";
    xspi1.registers[0x4018501C] = "BUFCR3";
    xspi1.registers[0x40185020] = "BFGENCR";
    xspi1.registers[0x40185024] = "SOCCR";
    xspi1.registers[0x40185100] = "SFAR";
    xspi1.registers[0x40185104] = "SFACR";
    xspi1.registers[0x40185108] = "SMPR";
    xspi1.registers[0x40185300] = "LUTKEY";
    xspi1.registers[0x40185304] = "LCKCR";
    peripherals["XSPI1"] = xspi1;
    
    // XSPI2 already defined above with real MIMXRT798S definitions

    // GPIO0 already defined above with real MIMXRT798S definitions

    PeripheralInfo gpio1;
    gpio1.name = "GPIO1";
    gpio1.baseAddress = 0x40102000;
    gpio1.registers[0x40102000] = "PDOR";
    gpio1.registers[0x40102004] = "PSOR";
    gpio1.registers[0x40102008] = "PCOR";
    gpio1.registers[0x4010200C] = "PTOR";
    gpio1.registers[0x40102010] = "PDIR";
    gpio1.registers[0x40102014] = "PDDR";
    gpio1.registers[0x40102018] = "PIDR";
    peripherals["GPIO1"] = gpio1;

    PeripheralInfo gpio2;
    gpio2.name = "GPIO2";
    gpio2.baseAddress = 0x40104000;
    gpio2.registers[0x40104000] = "PDOR";
    gpio2.registers[0x40104004] = "PSOR";
    gpio2.registers[0x40104008] = "PCOR";
    gpio2.registers[0x4010400C] = "PTOR";
    gpio2.registers[0x40104010] = "PDIR";
    gpio2.registers[0x40104014] = "PDDR";
    gpio2.registers[0x40104018] = "PIDR";
    peripherals["GPIO2"] = gpio2;

    PeripheralInfo gpio3;
    gpio3.name = "GPIO3";
    gpio3.baseAddress = 0x40106000;
    gpio3.registers[0x40106000] = "PDOR";
    gpio3.registers[0x40106004] = "PSOR";
    gpio3.registers[0x40106008] = "PCOR";
    gpio3.registers[0x4010600C] = "PTOR";
    gpio3.registers[0x40106010] = "PDIR";
    gpio3.registers[0x40106014] = "PDDR";
    gpio3.registers[0x40106018] = "PIDR";
    peripherals["GPIO3"] = gpio3;

    // CLKCTL0 already defined above with real MIMXRT798S definitions

    // SYSCON0 already defined above with real MIMXRT798S definitions

    // UART Peripherals (LP_FLEXCOMM)
    PeripheralInfo lpuart0;
    lpuart0.name = "LP_FLEXCOMM0";
    lpuart0.baseAddress = 0x40110000;
    lpuart0.registers[0x40110000] = "VERID";     // Version ID Register
    lpuart0.registers[0x40110004] = "PARAM";     // Parameter Register
    lpuart0.registers[0x40110008] = "GLOBAL";    // Global Register
    lpuart0.registers[0x4011000C] = "PINCFG";    // Pin Configuration Register
    lpuart0.registers[0x40110010] = "BAUD";      // Baud Rate Register
    lpuart0.registers[0x40110014] = "STAT";      // Status Register
    lpuart0.registers[0x40110018] = "CTRL";      // Control Register
    lpuart0.registers[0x4011001C] = "DATA";      // Data Register
    lpuart0.registers[0x40110020] = "MATCH";     // Match Address Register
    lpuart0.registers[0x40110024] = "MODIR";     // Modem IrDA Register
    lpuart0.registers[0x40110028] = "FIFO";      // FIFO Register
    lpuart0.registers[0x4011002C] = "WATER";     // Watermark Register
    peripherals["LP_FLEXCOMM0"] = lpuart0;

    PeripheralInfo lpuart1;
    lpuart1.name = "LP_FLEXCOMM1";
    lpuart1.baseAddress = 0x40111000;
    lpuart1.registers[0x40111000] = "VERID";
    lpuart1.registers[0x40111004] = "PARAM";
    lpuart1.registers[0x40111008] = "GLOBAL";
    lpuart1.registers[0x4011100C] = "PINCFG";
    lpuart1.registers[0x40111010] = "BAUD";
    lpuart1.registers[0x40111014] = "STAT";
    lpuart1.registers[0x40111018] = "CTRL";
    lpuart1.registers[0x4011101C] = "DATA";
    lpuart1.registers[0x40111020] = "MATCH";
    lpuart1.registers[0x40111024] = "MODIR";
    lpuart1.registers[0x40111028] = "FIFO";
    lpuart1.registers[0x4011102C] = "WATER";
    peripherals["LP_FLEXCOMM1"] = lpuart1;

    // DMA Peripherals
    PeripheralInfo dma0;
    dma0.name = "DMA0";
    dma0.baseAddress = 0x40140000;
    dma0.registers[0x40140000] = "CTRL";         // Control Register
    dma0.registers[0x40140004] = "INTSTAT";      // Interrupt Status Register
    dma0.registers[0x40140008] = "SRAMBASE";     // SRAM Base Address Register
    dma0.registers[0x40140100] = "ENABLESET0";   // Channel Enable Set Register 0
    dma0.registers[0x40140104] = "ENABLECLR0";   // Channel Enable Clear Register 0
    dma0.registers[0x40140108] = "ACTIVE0";      // Channel Active Register 0
    dma0.registers[0x4014010C] = "BUSY0";        // Channel Busy Register 0
    dma0.registers[0x40140110] = "ERRINT0";      // Error Interrupt Register 0
    dma0.registers[0x40140114] = "INTENSET0";    // Interrupt Enable Set Register 0
    dma0.registers[0x40140118] = "INTENCLR0";    // Interrupt Enable Clear Register 0
    dma0.registers[0x4014011C] = "INTA0";        // Interrupt A Register 0
    dma0.registers[0x40140120] = "INTB0";        // Interrupt B Register 0
    peripherals["DMA0"] = dma0;

    PeripheralInfo dma1;
    dma1.name = "DMA1";
    dma1.baseAddress = 0x40160000;
    dma1.registers[0x40160000] = "CTRL";
    dma1.registers[0x40160004] = "INTSTAT";
    dma1.registers[0x40160008] = "SRAMBASE";
    dma1.registers[0x40160100] = "ENABLESET0";
    dma1.registers[0x40160104] = "ENABLECLR0";
    dma1.registers[0x40160108] = "ACTIVE0";
    dma1.registers[0x4016010C] = "BUSY0";
    dma1.registers[0x40160110] = "ERRINT0";
    dma1.registers[0x40160114] = "INTENSET0";
    dma1.registers[0x40160118] = "INTENCLR0";
    dma1.registers[0x4016011C] = "INTA0";
    dma1.registers[0x40160120] = "INTB0";
    peripherals["DMA1"] = dma1;

    // Timer Peripherals
    PeripheralInfo ctimer0;
    ctimer0.name = "CTIMER0";
    ctimer0.baseAddress = 0x40028000;
    ctimer0.registers[0x40028000] = "IR";        // Interrupt Register
    ctimer0.registers[0x40028004] = "TCR";       // Timer Control Register
    ctimer0.registers[0x40028008] = "TC";        // Timer Counter
    ctimer0.registers[0x4002800C] = "PR";        // Prescale Register
    ctimer0.registers[0x40028010] = "PC";        // Prescale Counter
    ctimer0.registers[0x40028014] = "MCR";       // Match Control Register
    ctimer0.registers[0x40028018] = "MR0";       // Match Register 0
    ctimer0.registers[0x4002801C] = "MR1";       // Match Register 1
    ctimer0.registers[0x40028020] = "MR2";       // Match Register 2
    ctimer0.registers[0x40028024] = "MR3";       // Match Register 3
    ctimer0.registers[0x40028028] = "CCR";       // Capture Control Register
    ctimer0.registers[0x4002802C] = "CR0";       // Capture Register 0
    ctimer0.registers[0x40028030] = "CR1";       // Capture Register 1
    peripherals["CTIMER0"] = ctimer0;

    // ADC Peripherals
    PeripheralInfo adc0;
    adc0.name = "ADC0";
    adc0.baseAddress = 0x4020C000;
    adc0.registers[0x4020C000] = "VERID";        // Version ID Register
    adc0.registers[0x4020C004] = "PARAM";        // Parameter Register
    adc0.registers[0x4020C008] = "CTRL";         // Control Register
    adc0.registers[0x4020C00C] = "STAT";         // Status Register
    adc0.registers[0x4020C010] = "IE";           // Interrupt Enable Register
    adc0.registers[0x4020C014] = "DE";           // DMA Enable Register
    adc0.registers[0x4020C018] = "CFG";          // Configuration Register
    adc0.registers[0x4020C01C] = "PAUSE";        // Pause Register
    adc0.registers[0x4020C020] = "SWTRIG";       // Software Trigger Register
    adc0.registers[0x4020C024] = "TSTAT";        // Trigger Status Register
    adc0.registers[0x4020C040] = "OFSTRIM";      // Offset Trim Register
    adc0.registers[0x4020C100] = "TCTRL0";       // Trigger Control Register 0
    adc0.registers[0x4020C104] = "TCTRL1";       // Trigger Control Register 1
    adc0.registers[0x4020C200] = "FCTRL0";       // FIFO Control Register 0
    adc0.registers[0x4020C204] = "FCTRL1";       // FIFO Control Register 1
    adc0.registers[0x4020C300] = "GCC0";         // Gain Calibration Control 0
    adc0.registers[0x4020C304] = "GCC1";         // Gain Calibration Control 1
    adc0.registers[0x4020C400] = "GCR0";         // Gain Calibration Result 0
    adc0.registers[0x4020C404] = "GCR1";         // Gain Calibration Result 1
    peripherals["ADC0"] = adc0;

    // USB Peripherals
    PeripheralInfo usb0;
    usb0.name = "USB0";
    usb0.baseAddress = 0x40418000;
    usb0.registers[0x40418000] = "GPTIMER0LD";   // General Purpose Timer 0 Load
    usb0.registers[0x40418004] = "GPTIMER0CTRL"; // General Purpose Timer 0 Control
    usb0.registers[0x40418008] = "GPTIMER1LD";   // General Purpose Timer 1 Load
    usb0.registers[0x4041800C] = "GPTIMER1CTRL"; // General Purpose Timer 1 Control
    usb0.registers[0x40418010] = "SBUSCFG";      // System Bus Config
    usb0.registers[0x40418080] = "HCIVERSION";   // Host Controller Interface Version
    usb0.registers[0x40418084] = "HCSPARAMS";    // Host Controller Structural Parameters
    usb0.registers[0x40418088] = "HCCPARAMS";    // Host Controller Capability Parameters
    usb0.registers[0x40418100] = "DCIVERSION";   // Device Controller Interface Version
    usb0.registers[0x40418104] = "DCCPARAMS";    // Device Controller Capability Parameters
    usb0.registers[0x40418140] = "USBCMD";       // USB Command Register
    usb0.registers[0x40418144] = "USBSTS";       // USB Status Register
    usb0.registers[0x40418148] = "USBINTR";      // USB Interrupt Enable Register
    usb0.registers[0x4041814C] = "FRINDEX";      // USB Frame Index Register
    peripherals["USB0"] = usb0;

    // CRC Peripheral
    PeripheralInfo crc;
    crc.name = "CRC";
    crc.baseAddress = 0x40151000;
    crc.registers[0x40151000] = "MODE";          // CRC Mode Register
    crc.registers[0x40151004] = "SEED";          // CRC Seed Register
    crc.registers[0x40151008] = "SUM";           // CRC Checksum Register
    crc.registers[0x4015100C] = "WR_DATA";       // CRC Data Register
    peripherals["CRC"] = crc;

    // TRNG (True Random Number Generator)
    PeripheralInfo trng;
    trng.name = "TRNG";
    trng.baseAddress = 0x40187000;
    trng.registers[0x40187000] = "MCTL";         // Miscellaneous Control Register
    trng.registers[0x40187004] = "SCMISC";       // Statistical Check Miscellaneous Register
    trng.registers[0x40187008] = "PKRRNG";       // Poker Range Register
    trng.registers[0x4018700C] = "PKRMAX";       // Poker Maximum Limit Register
    trng.registers[0x40187010] = "PKRSQ";        // Poker Square Calculation Result Register
    trng.registers[0x40187014] = "SDCTL";        // Seed Control Register
    trng.registers[0x40187018] = "SBLIM";        // Sparse Bit Limit Register
    trng.registers[0x4018701C] = "TOTSAM";       // Total Samples Register
    trng.registers[0x40187020] = "FRQMIN";       // Frequency Count Minimum Limit Register
    trng.registers[0x40187024] = "FRQCNT";       // Frequency Count Register
    trng.registers[0x40187028] = "FRQMAX";       // Frequency Count Maximum Limit Register
    trng.registers[0x4018702C] = "SCMC";         // Statistical Check Monobit Count Register
    trng.registers[0x40187030] = "SCML";         // Statistical Check Monobit Limit Register
    trng.registers[0x40187034] = "SCR1C";        // Statistical Check Run Length 1 Count Register
    trng.registers[0x40187038] = "SCR1L";        // Statistical Check Run Length 1 Limit Register
    peripherals["TRNG"] = trng;

    // Add MPU (Memory Protection Unit) peripheral
    PeripheralInfo mpu;
    mpu.name = "MPU";
    mpu.baseAddress = 0xE000ED90;  // ARM Cortex-M33 MPU base address
    mpu.registers[0xE000ED90] = "TYPE";      // MPU Type Register
    mpu.registers[0xE000ED94] = "CTRL";      // MPU Control Register
    mpu.registers[0xE000ED98] = "RNR";       // MPU Region Number Register
    mpu.registers[0xE000ED9C] = "RBAR";      // MPU Region Base Address Register
    mpu.registers[0xE000EDA0] = "RLAR";      // MPU Region Limit Address Register
    mpu.registers[0xE000EDA4] = "RBAR_A1";   // MPU Region Base Address Register Alias 1
    mpu.registers[0xE000EDA8] = "RLAR_A1";   // MPU Region Limit Address Register Alias 1
    mpu.registers[0xE000EDAC] = "RBAR_A2";   // MPU Region Base Address Register Alias 2
    mpu.registers[0xE000EDB0] = "RLAR_A2";   // MPU Region Limit Address Register Alias 2
    mpu.registers[0xE000EDB4] = "RBAR_A3";   // MPU Region Base Address Register Alias 3
    mpu.registers[0xE000EDB8] = "RLAR_A3";   // MPU Region Limit Address Register Alias 3
    mpu.registers[0xE000EDBC] = "MAIR0";     // MPU Memory Attribute Indirection Register 0
    mpu.registers[0xE000EDC0] = "MAIR1";     // MPU Memory Attribute Indirection Register 1
    peripherals["MPU"] = mpu;

    // Add XCACHE0 peripheral
    PeripheralInfo xcache0;
    xcache0.name = "XCACHE0";
    xcache0.baseAddress = 0x40180000;
    xcache0.registers[0x40180000] = "CCR";       // Cache Control Register
    xcache0.registers[0x40180004] = "CLCR";      // Cache Line Control Register
    xcache0.registers[0x40180008] = "CSAR";      // Cache Search Address Register
    xcache0.registers[0x4018000C] = "CCVR";      // Cache Read/Write Value Register
    xcache0.registers[0x40180020] = "CCIR";      // Cache Invalidate Register
    xcache0.registers[0x40180024] = "CCLR";      // Cache Clear Register
    peripherals["XCACHE0"] = xcache0;

    // Add XCACHE1 peripheral
    PeripheralInfo xcache1;
    xcache1.name = "XCACHE1";
    xcache1.baseAddress = 0x40181000;
    xcache1.registers[0x40181000] = "CCR";       // Cache Control Register
    xcache1.registers[0x40181004] = "CLCR";      // Cache Line Control Register
    xcache1.registers[0x40181008] = "CSAR";      // Cache Search Address Register
    xcache1.registers[0x4018100C] = "CCVR";      // Cache Read/Write Value Register
    xcache1.registers[0x40181020] = "CCIR";      // Cache Invalidate Register
    xcache1.registers[0x40181024] = "CCLR";      // Cache Clear Register
    peripherals["XCACHE1"] = xcache1;

    // Add SYSCON1 peripheral
    PeripheralInfo syscon1;
    syscon1.name = "SYSCON1";
    syscon1.baseAddress = 0x40001000;
    syscon1.registers[0x40001000] = "UPDATELCKOUT";
    syscon1.registers[0x40001004] = "FCCTRLSEL";
    syscon1.registers[0x40001008] = "SHAREDCTRLSET";
    syscon1.registers[0x4000100C] = "SHAREDCTRLCLR";
    peripherals["SYSCON1"] = syscon1;



    // Add CLKCTL1 peripheral
    PeripheralInfo clkctl1;
    clkctl1.name = "CLKCTL1";
    clkctl1.baseAddress = 0x40003000;
    clkctl1.registers[0x40003000] = "PSCCTL0";
    clkctl1.registers[0x40003004] = "PSCCTL1";
    clkctl1.registers[0x40003008] = "PSCCTL2";
    clkctl1.registers[0x4000300C] = "PSCCTL3";
    clkctl1.registers[0x40003010] = "PSCCTL4";
    peripherals["CLKCTL1"] = clkctl1;

    // Add RSTCTL1 peripheral - REAL MIMXRT798S ADDRESSES
    PeripheralInfo rstctl1;
    rstctl1.name = "RSTCTL1";
    rstctl1.baseAddress = 0x40061000;  // Corrected from SDK headers
    rstctl1.registers[0x40061010] = "PRSTCTL0";
    rstctl1.registers[0x40061040] = "PRSTCTL0_SET";
    rstctl1.registers[0x40061070] = "PRSTCTL0_CLR";
    peripherals["RSTCTL1"] = rstctl1;

    // Add RSTCTL2 peripheral - REAL MIMXRT798S ADDRESSES
    PeripheralInfo rstctl2;
    rstctl2.name = "RSTCTL2";
    rstctl2.baseAddress = 0x40067000;  // Corrected from SDK headers
    rstctl2.registers[0x40067010] = "PRSTCTL0";
    rstctl2.registers[0x40067040] = "PRSTCTL0_SET";
    rstctl2.registers[0x40067070] = "PRSTCTL0_CLR";
    peripherals["RSTCTL2"] = rstctl2;

    // Add RSTCTL3 peripheral - REAL MIMXRT798S ADDRESSES
    PeripheralInfo rstctl3;
    rstctl3.name = "RSTCTL3";
    rstctl3.baseAddress = 0x40060000;  // Corrected from SDK headers
    rstctl3.registers[0x40060000] = "SYSRSTSTAT";
    rstctl3.registers[0x40060004] = "DOMRSTSTAT";
    rstctl3.registers[0x40060010] = "PRSTCTL0";
    rstctl3.registers[0x40060014] = "PRSTCTL1";
    rstctl3.registers[0x40060040] = "PRSTCTL0_SET";
    rstctl3.registers[0x40060044] = "PRSTCTL1_SET";
    rstctl3.registers[0x40060070] = "PRSTCTL0_CLR";
    rstctl3.registers[0x40060074] = "PRSTCTL1_CLR";
    peripherals["RSTCTL3"] = rstctl3;

    // Add RSTCTL4 peripheral - REAL MIMXRT798S ADDRESSES
    PeripheralInfo rstctl4;
    rstctl4.name = "RSTCTL4";
    rstctl4.baseAddress = 0x400A0000;  // Corrected from SDK headers
    rstctl4.registers[0x400A0010] = "PRSTCTL0";
    rstctl4.registers[0x400A0014] = "PRSTCTL1";
    rstctl4.registers[0x400A0040] = "PRSTCTL0_SET";
    rstctl4.registers[0x400A0044] = "PRSTCTL1_SET";
    rstctl4.registers[0x400A0070] = "PRSTCTL0_CLR";
    rstctl4.registers[0x400A0074] = "PRSTCTL1_CLR";
    peripherals["RSTCTL4"] = rstctl4;

    // Add IOPCTL0 peripheral (I/O Pin Control) - REAL MIMXRT798S ADDRESSES
    PeripheralInfo iopctl0;
    iopctl0.name = "IOPCTL0";
    iopctl0.baseAddress = 0x40004000;  // Corrected from SDK headers
    // IOPCTL0 has 4 ports with 32 pins each, offset by 0x80 per port
    for (int port = 0; port < 4; port++) {
        for (int pin = 0; pin < 32; pin++) {
            uint32_t regAddr = 0x40004000 + (port * 0x80) + (pin * 0x4);
            iopctl0.registers[regAddr] = "PIO" + std::to_string(port) + "_" + std::to_string(pin);
        }
    }
    peripherals["IOPCTL0"] = iopctl0;

    // Add IOPCTL1 peripheral - REAL MIMXRT798S ADDRESSES
    PeripheralInfo iopctl1;
    iopctl1.name = "IOPCTL1";
    iopctl1.baseAddress = 0x40064000;  // Corrected from SDK headers
    // IOPCTL1 has 3 ports, different pin counts per port
    for (int port = 0; port < 3; port++) {
        int maxPins = (port == 0) ? 32 : (port == 1) ? 3 : 18;
        for (int pin = 0; pin < maxPins; pin++) {
            uint32_t regAddr = 0x40064000 + (port * 0x80) + (pin * 0x4);
            iopctl1.registers[regAddr] = "PIO" + std::to_string(port + 8) + "_" + std::to_string(pin);
        }
    }
    // Add special PMIC I2C registers
    iopctl1.registers[0x40064180] = "PMIC_I2C_SDA";
    iopctl1.registers[0x40064184] = "PMIC_I2C_SCL";
    peripherals["IOPCTL1"] = iopctl1;

    // Add IOPCTL2 peripheral - REAL MIMXRT798S ADDRESSES
    PeripheralInfo iopctl2;
    iopctl2.name = "IOPCTL2";
    iopctl2.baseAddress = 0x400A5000;  // Corrected from SDK headers
    // IOPCTL2 has 4 ports with different pin counts
    int iopctl2_pins[] = {21, 21, 13, 26};
    for (int port = 0; port < 4; port++) {
        for (int pin = 0; pin < iopctl2_pins[port]; pin++) {
            uint32_t regAddr = 0x400A5000 + (port * 0x80) + (pin * 0x4);
            iopctl2.registers[regAddr] = "PIO" + std::to_string(port + 4) + "_" + std::to_string(pin);
        }
    }
    peripherals["IOPCTL2"] = iopctl2;

    // Add GLIKEY peripheral (Global Key Registers)
    PeripheralInfo glikey;
    glikey.name = "GLIKEY";
    glikey.baseAddress = 0x40008000;
    glikey.registers[0x40008000] = "GLIKEY0";
    glikey.registers[0x40008004] = "GLIKEY1";
    glikey.registers[0x40008008] = "GLIKEY2";
    glikey.registers[0x4000800C] = "GLIKEY3";  // Used for SYSCON0_SEC_CLK_CTRL
    glikey.registers[0x40008010] = "GLIKEY4";
    glikey.registers[0x40008014] = "GLIKEY5";
    glikey.registers[0x40008018] = "GLIKEY6";
    glikey.registers[0x4000801C] = "GLIKEY7";
    peripherals["GLIKEY"] = glikey;

    // Add AHBSC peripheral (AHB Secure Controller)
    PeripheralInfo ahbsc0;
    ahbsc0.name = "AHBSC0";
    ahbsc0.baseAddress = 0x40009000;
    ahbsc0.registers[0x40009000] = "MISC_CTRL_DP_REG";
    ahbsc0.registers[0x40009004] = "MISC_CTRL_REG";
    ahbsc0.registers[0x40009100] = "COMPUTE_ARB0RAM_ACCESS_ENABLE";
    ahbsc0.registers[0x40009104] = "SENSE_ARB0RAM_ACCESS_ENABLE";
    ahbsc0.registers[0x40009108] = "MEDIA_ARB0RAM_ACCESS_ENABLE";
    ahbsc0.registers[0x4000910C] = "NPU_ARB0RAM_ACCESS_ENABLE";
    ahbsc0.registers[0x40009110] = "HIFI4_ARB0RAM_ACCESS_ENABLE";
    peripherals["AHBSC0"] = ahbsc0;

    // Add CACHE64_CTRL peripherals
    PeripheralInfo cache64_ctrl0;
    cache64_ctrl0.name = "CACHE64_CTRL0";
    cache64_ctrl0.baseAddress = 0x40170000;
    cache64_ctrl0.registers[0x40170000] = "CCR";         // Cache Control Register
    cache64_ctrl0.registers[0x40170004] = "CLCR";        // Cache Line Control Register
    cache64_ctrl0.registers[0x40170008] = "CSAR";        // Cache Search Address Register
    cache64_ctrl0.registers[0x4017000C] = "CCVR";        // Cache Control Value Register
    cache64_ctrl0.registers[0x40170010] = "CRMR";        // Cache Region Mask Register
    peripherals["CACHE64_CTRL0"] = cache64_ctrl0;

    PeripheralInfo cache64_ctrl1;
    cache64_ctrl1.name = "CACHE64_CTRL1";
    cache64_ctrl1.baseAddress = 0x40171000;
    cache64_ctrl1.registers[0x40171000] = "CCR";
    cache64_ctrl1.registers[0x40171004] = "CLCR";
    cache64_ctrl1.registers[0x40171008] = "CSAR";
    cache64_ctrl1.registers[0x4017100C] = "CCVR";
    cache64_ctrl1.registers[0x40171010] = "CRMR";
    peripherals["CACHE64_CTRL1"] = cache64_ctrl1;

    // XCACHE peripherals already defined above

    // Add SYSCON3 peripheral (for Silicon Revision ID)
    PeripheralInfo syscon3;
    syscon3.name = "SYSCON3";
    syscon3.baseAddress = 0x40007000;
    syscon3.registers[0x40007000] = "SILICONREV_ID";     // Silicon Revision ID
    syscon3.registers[0x40007004] = "DEVICE_ID0";        // Device ID 0
    syscon3.registers[0x40007008] = "DEVICE_ID1";        // Device ID 1
    syscon3.registers[0x4000700C] = "DEVICE_ID2";        // Device ID 2
    peripherals["SYSCON3"] = syscon3;


}

void PeripheralAnalysisPass::analyzeFunctionInExecutionOrder(Function *F) {
    // Use instance variable instead of static to avoid cross-module issues
    if (visitedFunctions.count(F) || F->isDeclaration()) {
        return;
    }
    visitedFunctions.insert(F);

    // Analyze this function's instructions in execution order
    for (BasicBlock &BB : *F) {
        for (Instruction &I : BB) {
            // Handle all instruction types in execution order
            if (auto *CI = dyn_cast<CallInst>(&I)) {
                // First analyze the function call itself
                analyzeFunctionCall(CI);

                // Then recursively analyze the called function
                Function *calledFunction = CI->getCalledFunction();
                if (calledFunction && !calledFunction->isDeclaration()) {
                    analyzeFunctionInExecutionOrder(calledFunction);
                }
            } else if (auto *LI = dyn_cast<LoadInst>(&I)) {
                analyzeLoadInstruction(LI);
            } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                analyzeStoreInstruction(SI);
            } else if (auto *RMWI = dyn_cast<AtomicRMWInst>(&I)) {
                analyzeAtomicRMWInstruction(RMWI);
            } else if (auto *CXI = dyn_cast<AtomicCmpXchgInst>(&I)) {
                analyzeCmpXchgInstruction(CXI);
            }
        }
    }
}

void PeripheralAnalysisPass::analyzeFunction(Function &F) {
    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
            if (auto *LI = dyn_cast<LoadInst>(&I)) {
                analyzeLoadInstruction(LI);
            } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                analyzeStoreInstruction(SI);
            } else if (auto *RMWI = dyn_cast<AtomicRMWInst>(&I)) {
                analyzeAtomicRMWInstruction(RMWI);
            } else if (auto *CXI = dyn_cast<AtomicCmpXchgInst>(&I)) {
                analyzeCmpXchgInstruction(CXI);
            } else if (auto *CI = dyn_cast<CallInst>(&I)) {
                analyzeFunctionCall(CI);
            }
        }
    }
}

void PeripheralAnalysisPass::analyzeLoadInstruction(LoadInst *LI) {
    uint64_t address = getEffectiveAddress(LI->getPointerOperand());

    // Enhanced detection: also check if this could be a peripheral access even if address is 0
    // This handles cases where we couldn't resolve the address but it might still be peripheral-related
    bool isVolatile = LI->isVolatile();
    bool couldBePeripheral = false;

    if (address != 0 && isPeripheralAddress(address)) {
        couldBePeripheral = true;
    } else if (isVolatile && address == 0) {
        // Volatile access with unresolved address - might be peripheral
        // Try to infer from context (function name, etc.)
        std::string functionName = LI->getFunction()->getName().str();
        if (functionName.find("XSPI") != std::string::npos ||
            functionName.find("xspi") != std::string::npos ||
            functionName.find("GPIO") != std::string::npos ||
            functionName.find("CLOCK") != std::string::npos ||
            functionName.find("clock") != std::string::npos) {
            couldBePeripheral = true;
            // Try to infer peripheral type from function name
            address = inferPeripheralAddressFromContext(LI);
        }
    }

    if (!couldBePeripheral) {
        return;
    }

    // Try to identify register using struct member mapping first
    std::string peripheralName, registerName;
    bool foundRegister = false;

    // Check if this is a GEP-based access that we can map to a struct member
    if (auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand())) {
        uint64_t baseAddr = getEffectiveAddress(GEP->getPointerOperand());
        if (isPeripheralAddress(baseAddr)) {
            uint32_t memberIndex = extractStructMemberIndex(GEP);
            auto [pName, rName] = identifyPeripheralRegisterFromStructMember(baseAddr, memberIndex);
            if (!pName.empty()) {
                peripheralName = pName;
                registerName = rName;
                foundRegister = true;
            }
        }
    }

    // Fall back to address-based identification
    if (!foundRegister) {
        auto [pName, rName] = identifyPeripheralRegister(address);
        peripheralName = pName;
        registerName = rName;
        if (peripheralName.empty() && address != 0) {
            return;
        }
    }

    // If we couldn't identify the peripheral but it's volatile, create a generic entry
    if (peripheralName.empty() && isVolatile) {
        peripheralName = "UNKNOWN_PERIPHERAL";
        registerName = "UNKNOWN_REG";
    }

    RegisterAccess access;
    access.peripheralName = peripheralName;
    access.registerName = registerName;
    access.address = address;
    access.accessType = isVolatile ? "volatile_read" : "read";
    access.dataSize = getDataSizeFromType(LI->getType());
    access.bitsModified = {}; // No bits modified for reads

    auto [fileName, functionName, lineNumber] = getDebugInfo(LI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = determinePurpose(LI, peripheralName, registerName);

    // Assign execution order information
    assignExecutionOrder(access, LI);

    registerAccesses.push_back(access);
    if (!peripheralName.empty() && peripherals.find(peripheralName) != peripherals.end()) {
        peripherals[peripheralName].accessedAddresses.insert(address);
    }
}

void PeripheralAnalysisPass::analyzeStoreInstruction(StoreInst *SI) {
    uint64_t address = getEffectiveAddress(SI->getPointerOperand());

    // Enhanced detection: also check if this could be a peripheral access even if address is 0
    bool isVolatile = SI->isVolatile();
    bool couldBePeripheral = false;

    if (address != 0 && isPeripheralAddress(address)) {
        couldBePeripheral = true;
    } else if (isVolatile && address == 0) {
        // Volatile access with unresolved address - might be peripheral
        std::string functionName = SI->getFunction()->getName().str();
        if (functionName.find("XSPI") != std::string::npos ||
            functionName.find("xspi") != std::string::npos ||
            functionName.find("GPIO") != std::string::npos ||
            functionName.find("CLOCK") != std::string::npos ||
            functionName.find("clock") != std::string::npos) {
            couldBePeripheral = true;
            address = inferPeripheralAddressFromContext(SI);
        }
    }

    if (!couldBePeripheral) {
        return;
    }

    // Try to identify register using struct member mapping first
    std::string peripheralName, registerName;
    bool foundRegister = false;

    // Check if this is a GEP-based access that we can map to a struct member
    if (auto *GEP = dyn_cast<GetElementPtrInst>(SI->getPointerOperand())) {
        uint64_t baseAddr = getEffectiveAddress(GEP->getPointerOperand());
        if (isPeripheralAddress(baseAddr)) {
            uint32_t memberIndex = extractStructMemberIndex(GEP);
            auto [pName, rName] = identifyPeripheralRegisterFromStructMember(baseAddr, memberIndex);
            if (!pName.empty()) {
                peripheralName = pName;
                registerName = rName;
                foundRegister = true;
            }
        }
    }

    // Fall back to address-based identification
    if (!foundRegister) {
        auto [pName, rName] = identifyPeripheralRegister(address);
        peripheralName = pName;
        registerName = rName;
        if (peripheralName.empty() && address != 0) {
            return;
        }
    }

    // If we couldn't identify the peripheral but it's volatile, create a generic entry
    if (peripheralName.empty() && isVolatile) {
        peripheralName = "UNKNOWN_PERIPHERAL";
        registerName = "UNKNOWN_REG";
    }

    RegisterAccess access;
    access.peripheralName = peripheralName;
    access.registerName = registerName;
    access.address = address;
    access.accessType = isVolatile ? "volatile_write" : "write";
    access.dataSize = getDataSizeFromType(SI->getValueOperand()->getType());
    access.bitsModified = analyzeBitfieldOperations(SI);

    // Enhanced value extraction for written values
    Value *valueOperand = SI->getValueOperand();
    if (auto *CI = dyn_cast<ConstantInt>(valueOperand)) {
        access.valueWritten = CI->getZExtValue();
        access.hasValueWritten = true;
        access.valueWrittenStr = "0x" + std::to_string(access.valueWritten);
    } else if (auto *CE = dyn_cast<ConstantExpr>(valueOperand)) {
        // Handle constant expressions like OR, AND operations
        if (CE->getOpcode() == Instruction::Or || CE->getOpcode() == Instruction::And) {
            // Try to evaluate simple constant expressions
            if (CE->getNumOperands() == 2) {
                if (auto *Op1 = dyn_cast<ConstantInt>(CE->getOperand(0))) {
                    if (auto *Op2 = dyn_cast<ConstantInt>(CE->getOperand(1))) {
                        uint64_t val1 = Op1->getZExtValue();
                        uint64_t val2 = Op2->getZExtValue();
                        uint64_t result = 0;

                        if (CE->getOpcode() == Instruction::Or) {
                            result = val1 | val2;
                        } else if (CE->getOpcode() == Instruction::And) {
                            result = val1 & val2;
                        }

                        access.valueWritten = result;
                        access.hasValueWritten = true;
                        access.valueWrittenStr = "0x" + std::to_string(result);
                    }
                }
            }
        }
    } else {
        access.hasValueWritten = false;
        access.valueWrittenStr = "RUNTIME_VALUE";
    }

    auto [fileName, functionName, lineNumber] = getDebugInfo(SI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = determinePurpose(SI, peripheralName, registerName);

    // Enhanced call stack tracking
    access.callStack = buildCallStackContext(SI);

    // Assign execution order information
    assignExecutionOrder(access, SI);

    registerAccesses.push_back(access);
    if (!peripheralName.empty() && peripherals.find(peripheralName) != peripherals.end()) {
        peripherals[peripheralName].accessedAddresses.insert(address);
    }
}

void PeripheralAnalysisPass::analyzeAtomicRMWInstruction(AtomicRMWInst *RMWI) {
    uint64_t address = getEffectiveAddress(RMWI->getPointerOperand());
    if (!isPeripheralAddress(address)) {
        return;
    }
    
    auto [peripheralName, registerName] = identifyPeripheralRegister(address);
    if (peripheralName.empty()) {
        return;
    }
    
    RegisterAccess access;
    access.peripheralName = peripheralName;
    access.registerName = registerName;
    access.address = address;
    access.accessType = "read-modify-write";
    access.dataSize = getDataSizeFromType(RMWI->getType());
    access.bitsModified = {}; // TODO: Analyze atomic operation type
    
    auto [fileName, functionName, lineNumber] = getDebugInfo(RMWI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = determinePurpose(RMWI, peripheralName, registerName);

    // Assign execution order information
    assignExecutionOrder(access, RMWI);

    registerAccesses.push_back(access);
    peripherals[peripheralName].accessedAddresses.insert(address);
}

void PeripheralAnalysisPass::analyzeCmpXchgInstruction(AtomicCmpXchgInst *CXI) {
    uint64_t address = getEffectiveAddress(CXI->getPointerOperand());
    if (!isPeripheralAddress(address)) {
        return;
    }
    
    auto [peripheralName, registerName] = identifyPeripheralRegister(address);
    if (peripheralName.empty()) {
        return;
    }
    
    RegisterAccess access;
    access.peripheralName = peripheralName;
    access.registerName = registerName;
    access.address = address;
    access.accessType = "read-modify-write";
    access.dataSize = getDataSizeFromType(CXI->getNewValOperand()->getType());
    access.bitsModified = {}; // TODO: Analyze compare-and-swap operation
    
    auto [fileName, functionName, lineNumber] = getDebugInfo(CXI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = determinePurpose(CXI, peripheralName, registerName);

    // Assign execution order information
    assignExecutionOrder(access, CXI);

    registerAccesses.push_back(access);
    peripherals[peripheralName].accessedAddresses.insert(address);
}

void PeripheralAnalysisPass::analyzeFunctionCall(CallInst *CI) {
    Function *calledFunction = CI->getCalledFunction();
    if (!calledFunction) {
        return; // Indirect call, can't analyze
    }

    std::string functionName = calledFunction->getName().str();

    // Analyze IOPCTL_PinMuxSet calls
    if (functionName == "IOPCTL_PinMuxSet") {
        analyzeIOPCTLPinMuxSet(CI);
    }
    // Analyze RESET_ClearPeripheralReset calls
    else if (functionName == "RESET_ClearPeripheralReset") {
        analyzeRESETClearPeripheralReset(CI);
    }
    // Analyze CLOCK_AttachClk calls
    else if (functionName == "CLOCK_AttachClk") {
        analyzeCLOCKAttachClk(CI);
    }
    // Analyze CLOCK_SetClkDiv calls
    else if (functionName == "CLOCK_SetClkDiv") {
        analyzeCLOCKSetClkDiv(CI);
    }
    // Analyze ARM_MPU_SetRegion calls
    else if (functionName == "ARM_MPU_SetRegion") {
        analyzeARMMPUSetRegion(CI);
    }
    // Analyze ARM_MPU_Enable calls
    else if (functionName == "ARM_MPU_Enable") {
        analyzeARMMPUEnable(CI);
    }
    // Analyze XCACHE_EnableCache calls
    else if (functionName == "XCACHE_EnableCache") {
        analyzeXCACHEEnableCache(CI);
    }
    // Analyze XCACHE_DisableCache calls
    else if (functionName == "XCACHE_DisableCache") {
        analyzeXCACHEDisableCache(CI);
    }
    // Analyze ARM_MPU_Disable calls
    else if (functionName == "ARM_MPU_Disable") {
        analyzeARMMPUDisable(CI);
    }
    // Analyze ARM_MPU_SetMemAttr calls
    else if (functionName == "ARM_MPU_SetMemAttr") {
        analyzeARMMPUSetMemAttr(CI);
    }
    // Analyze GPIO_PinWrite calls
    else if (functionName == "GPIO_PinWrite") {
        analyzeGPIOPinWrite(CI);
    }
    // Analyze GPIO_PinRead calls
    else if (functionName == "GPIO_PinRead") {
        analyzeGPIOPinRead(CI);
    }
    // Analyze GPIO_PinInit calls
    else if (functionName == "GPIO_PinInit") {
        analyzeGPIOPinInit(CI);
    }
    // Analyze XCACHE_DisableCache calls
    else if (functionName == "XCACHE_DisableCache") {
        analyzeXCACHEDisableCache(CI);
    }
}

uint64_t PeripheralAnalysisPass::getEffectiveAddress(Value *Ptr) {
    // Try to get constant address
    if (auto *CI = dyn_cast<ConstantInt>(Ptr)) {
        return CI->getZExtValue();
    }

    // Handle constant expressions (like inttoptr)
    if (auto *CE = dyn_cast<ConstantExpr>(Ptr)) {
        if (CE->getOpcode() == Instruction::IntToPtr) {
            if (auto *CI = dyn_cast<ConstantInt>(CE->getOperand(0))) {
                return CI->getZExtValue();
            }
        }
        // Handle GEP constant expressions
        if (CE->getOpcode() == Instruction::GetElementPtr) {
            return getEffectiveAddressFromGEP(CE);
        }
    }

    // Handle global variables (peripheral base addresses)
    if (auto *GV = dyn_cast<GlobalVariable>(Ptr)) {
        if (GV->hasInitializer()) {
            if (auto *CI = dyn_cast<ConstantInt>(GV->getInitializer())) {
                return CI->getZExtValue();
            }
        }
    }

    // Handle GEP instructions for register offsets
    if (auto *GEP = dyn_cast<GetElementPtrInst>(Ptr)) {
        return getEffectiveAddressFromGEP(GEP);
    }

    // Handle function arguments that might be peripheral base addresses
    if (auto *Arg = dyn_cast<Argument>(Ptr)) {
        return tracePeripheralBaseFromArgument(Arg);
    }

    // Handle load instructions that load peripheral base addresses
    if (auto *LI = dyn_cast<LoadInst>(Ptr)) {
        return getEffectiveAddress(LI->getPointerOperand());
    }

    return 0; // Unknown address
}

uint64_t PeripheralAnalysisPass::getEffectiveAddressFromGEP(Value *GEP) {
    GetElementPtrInst *GEPI = nullptr;
    ConstantExpr *GEPCE = nullptr;

    if ((GEPI = dyn_cast<GetElementPtrInst>(GEP))) {
        // Handle GEP instruction
        uint64_t baseAddr = getEffectiveAddress(GEPI->getPointerOperand());
        if (baseAddr != 0) {
            return calculateGEPOffset(baseAddr, GEPI);
        }
    } else if ((GEPCE = dyn_cast<ConstantExpr>(GEP))) {
        // Handle GEP constant expression
        if (GEPCE->getOpcode() == Instruction::GetElementPtr) {
            uint64_t baseAddr = getEffectiveAddress(GEPCE->getOperand(0));
            if (baseAddr != 0) {
                return calculateGEPOffsetFromConstantExpr(baseAddr, GEPCE);
            }
        }
    }

    return 0;
}

uint64_t PeripheralAnalysisPass::calculateGEPOffset(uint64_t baseAddr, GetElementPtrInst *GEP) {
    uint64_t offset = 0;

    // For struct-based access patterns like in fsl_xspi.c
    // Handle: %16 = getelementptr inbounds %struct.XSPI_Type, ptr %15, i32 0, i32 134
    if (GEP->getNumOperands() >= 4) {
        // Skip the first index (usually 0 for struct access)
        for (unsigned i = 2; i < GEP->getNumOperands(); ++i) {
            if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(i))) {
                uint64_t index = CI->getZExtValue();

                // For XSPI struct members, each register is typically 4 bytes apart
                // This is a simplified calculation - in reality we'd need the struct layout
                if (i == 3) { // Last index is usually the struct member
                    offset += index * 4; // Assume 32-bit registers
                } else {
                    offset += index;
                }
            }
        }
    } else {
        // Simple byte offset calculation
        for (unsigned i = 1; i < GEP->getNumOperands(); ++i) {
            if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(i))) {
                offset += CI->getZExtValue();
            }
        }
    }

    return baseAddr + offset;
}

uint32_t PeripheralAnalysisPass::extractStructMemberIndex(GetElementPtrInst *GEP) {
    // For struct access patterns: %16 = getelementptr inbounds %struct.XSPI_Type, ptr %15, i32 0, i32 134
    // The last operand (index 3) is the struct member index
    if (GEP->getNumOperands() >= 4) {
        if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(3))) {
            return CI->getZExtValue();
        }
    }
    return 0; // Unknown member
}

uint32_t PeripheralAnalysisPass::extractStructMemberIndexFromConstantExpr(ConstantExpr *GEPCE) {
    // Similar logic for constant expressions
    if (GEPCE->getNumOperands() >= 4) {
        if (auto *CI = dyn_cast<ConstantInt>(GEPCE->getOperand(3))) {
            return CI->getZExtValue();
        }
    }
    return 0; // Unknown member
}

uint64_t PeripheralAnalysisPass::calculateGEPOffsetFromConstantExpr(uint64_t baseAddr, ConstantExpr *GEPCE) {
    uint64_t offset = 0;

    // Similar logic for constant expressions
    for (unsigned i = 2; i < GEPCE->getNumOperands(); ++i) {
        if (auto *CI = dyn_cast<ConstantInt>(GEPCE->getOperand(i))) {
            uint64_t index = CI->getZExtValue();
            if (i == GEPCE->getNumOperands() - 1) { // Last index
                offset += index * 4; // Assume 32-bit registers
            } else {
                offset += index;
            }
        }
    }

    return baseAddr + offset;
}

uint64_t PeripheralAnalysisPass::tracePeripheralBaseFromArgument(Argument *Arg) {
    // Look for function calls that pass known peripheral base addresses
    Function *F = Arg->getParent();

    // Check all call sites of this function
    for (User *U : F->users()) {
        if (auto *CI = dyn_cast<CallInst>(U)) {
            if (CI->getCalledFunction() == F) {
                // Get the argument value at this call site
                if (Arg->getArgNo() < CI->getNumOperands()) {
                    Value *ArgValue = CI->getOperand(Arg->getArgNo());
                    uint64_t addr = getEffectiveAddress(ArgValue);
                    if (isPeripheralAddress(addr)) {
                        return addr;
                    }
                }
            }
        }
    }

    return 0;
}

uint64_t PeripheralAnalysisPass::inferPeripheralAddressFromContext(Instruction *I) {
    std::string functionName = I->getFunction()->getName().str();

    // Try to infer peripheral base address from function name patterns
    if (functionName.find("XSPI") != std::string::npos || functionName.find("xspi") != std::string::npos) {
        // Default to XSPI2 which is commonly used in MIMXRT700
        return 0x40411000;
    } else if (functionName.find("GPIO") != std::string::npos) {
        return 0x40100000; // GPIO0
    } else if (functionName.find("CLOCK") != std::string::npos || functionName.find("clock") != std::string::npos) {
        return 0x40002000; // CLKCTL0
    }

    return 0; // Unknown
}

std::pair<std::string, std::string> PeripheralAnalysisPass::identifyPeripheralRegister(uint64_t address) {
    for (const auto &[name, peripheral] : peripherals) {
        // Check if address is within peripheral range (4KB typical)
        if (address >= peripheral.baseAddress &&
            address < peripheral.baseAddress + 0x1000) {

            // Look for exact register match
            auto it = peripheral.registers.find(address);
            if (it != peripheral.registers.end()) {
                return {name, it->second};
            }

            // Calculate register offset
            uint64_t offset = address - peripheral.baseAddress;
            return {name, "REG_0x" + std::to_string(offset)};
        }
    }
    return {"", ""};
}

std::pair<std::string, std::string> PeripheralAnalysisPass::identifyPeripheralRegisterFromStructMember(
    uint64_t baseAddress, uint32_t memberIndex) {

    for (const auto& [name, peripheral] : peripherals) {
        if (baseAddress == peripheral.baseAddress) {
            auto it = peripheral.structMemberToRegister.find(memberIndex);
            if (it != peripheral.structMemberToRegister.end()) {
                return {name, it->second};
            }

            // If exact member not found, return generic register name
            return {name, "MEMBER_" + std::to_string(memberIndex)};
        }
    }

    return {"", ""};
}

std::vector<std::string> PeripheralAnalysisPass::analyzeBitfieldOperations(StoreInst *SI) {
    std::vector<std::string> bitsModified;

    Value *storedValue = SI->getValueOperand();

    // Check for bitwise operations that indicate specific bit manipulation
    if (auto *BinOp = dyn_cast<BinaryOperator>(storedValue)) {
        switch (BinOp->getOpcode()) {
            case Instruction::Or:
                // Setting bits: reg |= mask
                if (auto *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) {
                    uint64_t mask = CI->getZExtValue();
                    for (int i = 0; i < 32; ++i) {
                        if (mask & (1ULL << i)) {
                            bitsModified.push_back("bit_" + std::to_string(i));
                        }
                    }
                }
                break;

            case Instruction::And:
                // Clearing bits: reg &= ~mask
                if (auto *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) {
                    uint64_t mask = CI->getZExtValue();
                    // Inverted mask indicates cleared bits
                    uint64_t clearedMask = ~mask;
                    for (int i = 0; i < 32; ++i) {
                        if (clearedMask & (1ULL << i)) {
                            bitsModified.push_back("bit_" + std::to_string(i));
                        }
                    }
                }
                break;

            case Instruction::Xor:
                // Toggling bits: reg ^= mask
                if (auto *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1))) {
                    uint64_t mask = CI->getZExtValue();
                    for (int i = 0; i < 32; ++i) {
                        if (mask & (1ULL << i)) {
                            bitsModified.push_back("bit_" + std::to_string(i));
                        }
                    }
                }
                break;

            default:
                break;
        }
    }

    // If no specific bits identified, assume all bits could be modified
    if (bitsModified.empty()) {
        unsigned dataSize = getDataSizeFromType(storedValue->getType());
        bitsModified.push_back("bit_0-" + std::to_string(dataSize - 1));
    }

    return bitsModified;
}

std::tuple<std::string, std::string, unsigned> PeripheralAnalysisPass::getDebugInfo(Instruction *I) {
    std::string fileName = "unknown";
    std::string functionName = "unknown";
    unsigned lineNumber = 0;

    // Get function name
    if (Function *F = I->getFunction()) {
        functionName = F->getName().str();
    }

    // Get debug location
    if (DILocation *Loc = I->getDebugLoc()) {
        fileName = Loc->getFilename().str();
        lineNumber = Loc->getLine();
    }

    return {fileName, functionName, lineNumber};
}

std::string PeripheralAnalysisPass::determinePurpose(Instruction *I,
                                                   const std::string& peripheralName,
                                                   const std::string& registerName) {
    std::string functionName = I->getFunction()->getName().str();

    // Analyze function name patterns
    if (functionName.find("init") != std::string::npos ||
        functionName.find("Init") != std::string::npos) {
        return "Initialize " + peripheralName + " controller";
    }

    if (functionName.find("config") != std::string::npos ||
        functionName.find("Config") != std::string::npos) {
        return "Configure " + peripheralName + " settings";
    }

    if (functionName.find("enable") != std::string::npos ||
        functionName.find("Enable") != std::string::npos) {
        return "Enable " + peripheralName + " functionality";
    }

    if (functionName.find("disable") != std::string::npos ||
        functionName.find("Disable") != std::string::npos) {
        return "Disable " + peripheralName + " functionality";
    }

    if (functionName.find("read") != std::string::npos ||
        functionName.find("Read") != std::string::npos) {
        return "Read data from " + peripheralName;
    }

    if (functionName.find("write") != std::string::npos ||
        functionName.find("Write") != std::string::npos) {
        return "Write data to " + peripheralName;
    }

    if (functionName.find("transfer") != std::string::npos ||
        functionName.find("Transfer") != std::string::npos) {
        return "Transfer data via " + peripheralName;
    }

    // Analyze register name patterns
    if (registerName == "MCR") {
        return "Module configuration";
    } else if (registerName == "IPCR") {
        return "IP command configuration";
    } else if (registerName == "SFAR") {
        return "Set flash address";
    } else if (registerName.find("BUF") != std::string::npos) {
        return "Buffer configuration";
    }

    return "Access " + registerName + " register";
}

bool PeripheralAnalysisPass::isPeripheralAddress(uint64_t address) {
    // MIMXRT700 peripheral address ranges
    // Secure peripherals: 0x50000000 - 0x5FFFFFFF
    // Non-secure peripherals: 0x40000000 - 0x4FFFFFFF
    return (address >= 0x40000000 && address <= 0x5FFFFFFF);
}

unsigned PeripheralAnalysisPass::getDataSizeFromType(Type *Ty) {
    if (Ty->isIntegerTy()) {
        return Ty->getIntegerBitWidth();
    } else if (Ty->isPointerTy()) {
        return 32; // Assume 32-bit pointers
    }
    return 32; // Default to 32-bit
}

void PeripheralAnalysisPass::exportToJSON(const std::string& filename) const {
    json::Object root;
    json::Array peripheralArray;

    // Group accesses by peripheral
    std::map<std::string, std::vector<const RegisterAccess*>> accessesByPeripheral;
    for (const auto& access : registerAccesses) {
        accessesByPeripheral[access.peripheralName].push_back(&access);
    }

    // Create JSON structure
    for (const auto& [peripheralName, accesses] : accessesByPeripheral) {
        json::Object peripheralObj;
        peripheralObj["peripheral_name"] = peripheralName;

        // Find base address
        auto it = peripherals.find(peripheralName);
        if (it != peripherals.end()) {
            std::stringstream base_ss;
            base_ss << "0x" << std::hex << std::uppercase << it->second.baseAddress;
            peripheralObj["base_address"] = base_ss.str();
        }

        json::Array accessArray;
        for (const auto* access : accesses) {
            json::Object accessObj;
            accessObj["register_name"] = access->registerName;
            std::stringstream addr_ss;
            addr_ss << "0x" << std::hex << std::uppercase << access->address;
            accessObj["address"] = addr_ss.str();
            accessObj["access_type"] = access->accessType;
            accessObj["data_size"] = static_cast<int64_t>(access->dataSize);

            json::Array bitsArray;
            for (const auto& bit : access->bitsModified) {
                bitsArray.push_back(bit);
            }
            accessObj["bits_modified"] = std::move(bitsArray);

            json::Object sourceObj;
            sourceObj["file"] = access->fileName;
            sourceObj["function"] = access->functionName;
            sourceObj["line"] = static_cast<int64_t>(access->lineNumber);
            accessObj["source_location"] = std::move(sourceObj);

            accessObj["purpose"] = access->purpose;

            accessArray.push_back(std::move(accessObj));
        }

        peripheralObj["accesses"] = std::move(accessArray);
        peripheralArray.push_back(std::move(peripheralObj));
    }

    root["peripheral_accesses"] = std::move(peripheralArray);

    // Write to file
    std::error_code EC;
    raw_fd_ostream OS(filename, EC);
    if (EC) {
        errs() << "Error opening file " << filename << ": " << EC.message() << "\n";
        return;
    }

    OS << json::Value(std::move(root)) << "\n";
}

void PeripheralAnalysisPass::initializeExecutionPhaseMapping() {
    // Board initialization functions (executed first during system startup)
    functionToPhaseMap["BOARD_InitHardware"] = "board_init";
    functionToPhaseMap["BOARD_SetXspiClock"] = "board_init";
    functionToPhaseMap["BOARD_DeinitXspi"] = "board_init";
    functionToPhaseMap["BOARD_InitI2c2PinAsGpio"] = "board_init";
    functionToPhaseMap["BOARD_RestoreI2c2PinMux"] = "board_init";
    functionToPhaseMap["hardware_init"] = "board_init";
    functionToPhaseMap["board_init"] = "board_init";
    functionToPhaseMap["pin_mux_init"] = "board_init";
    functionToPhaseMap["clock_config"] = "board_init";
    functionToPhaseMap["CLOCK_SetupExtClocking"] = "board_init";
    functionToPhaseMap["CLOCK_SetupFROClocking"] = "board_init";
    functionToPhaseMap["POWER_DisablePD"] = "board_init";
    functionToPhaseMap["POWER_ApplyPD"] = "board_init";

    // Driver initialization functions (executed after board init)
    functionToPhaseMap["XSPI_Init"] = "driver_init";
    functionToPhaseMap["XSPI_SetFlashConfig"] = "driver_init";
    functionToPhaseMap["XSPI_UpdateLUT"] = "driver_init";
    functionToPhaseMap["GPIO_PinInit"] = "driver_init";
    functionToPhaseMap["CLOCK_AttachClk"] = "driver_init";
    functionToPhaseMap["CLOCK_SetClkDiv"] = "driver_init";
    functionToPhaseMap["CLOCK_EnableClock"] = "driver_init";
    functionToPhaseMap["RESET_PeripheralReset"] = "driver_init";

    // Runtime operation functions (executed during normal operation)
    functionToPhaseMap["XSPI_TransferBlocking"] = "runtime";
    functionToPhaseMap["XSPI_WriteBlocking"] = "runtime";
    functionToPhaseMap["XSPI_ReadBlocking"] = "runtime";
    functionToPhaseMap["GPIO_PinWrite"] = "runtime";
    functionToPhaseMap["GPIO_PinRead"] = "runtime";
    functionToPhaseMap["GPIO_PortSet"] = "runtime";
    functionToPhaseMap["GPIO_PortClear"] = "runtime";
    functionToPhaseMap["GPIO_PortToggle"] = "runtime";
}

std::string PeripheralAnalysisPass::determineExecutionPhase(const std::string& functionName, const std::string& fileName) {
    // Check exact function name match first
    auto it = functionToPhaseMap.find(functionName);
    if (it != functionToPhaseMap.end()) {
        return it->second;
    }

    // Check function name patterns for board initialization
    if (functionName.find("BOARD_") == 0 ||
        functionName.find("board_") == 0 ||
        functionName.find("hardware_init") != std::string::npos ||
        functionName.find("pin_mux") != std::string::npos ||
        functionName.find("clock_config") != std::string::npos ||
        functionName.find("CLOCK_Setup") != std::string::npos ||
        functionName.find("POWER_") == 0) {
        return "board_init";
    }

    // Check function name patterns for driver initialization
    if (functionName.find("_Init") != std::string::npos ||
        functionName.find("_Config") != std::string::npos ||
        functionName.find("CLOCK_Attach") != std::string::npos ||
        functionName.find("CLOCK_Enable") != std::string::npos ||
        functionName.find("RESET_") == 0) {
        return "driver_init";
    }

    // Check file name patterns
    if (fileName.find("board.c") != std::string::npos ||
        fileName.find("hardware_init.c") != std::string::npos ||
        fileName.find("pin_mux.c") != std::string::npos ||
        fileName.find("clock_config.c") != std::string::npos) {
        return "board_init";
    }

    // Default to runtime for other functions
    return "runtime";
}

std::string PeripheralAnalysisPass::buildCallStackContext(Instruction *I) {
    std::string callStack;
    Function *currentFunction = I->getFunction();

    // Start with the current function
    callStack = currentFunction->getName().str();

    // Try to trace back through call sites (simplified approach)
    // In a full implementation, we would need interprocedural analysis
    // For now, we'll just include the immediate function context

    return callStack;
}

std::string PeripheralAnalysisPass::generateBasicBlockId(BasicBlock *BB) {
    Function *F = BB->getParent();
    std::string functionName = F->getName().str();

    // Generate a unique identifier for the basic block
    // Use function name + basic block address as a simple identifier
    std::string bbId = functionName + "_BB_" + std::to_string(reinterpret_cast<uintptr_t>(BB));

    return bbId;
}

uint64_t PeripheralAnalysisPass::getInstructionIndex(Instruction *I) {
    BasicBlock *BB = I->getParent();
    uint64_t index = 0;

    // Count instructions in the basic block until we reach the target instruction
    for (auto &Inst : *BB) {
        if (&Inst == I) {
            return index;
        }
        index++;
    }

    return index;
}

void PeripheralAnalysisPass::assignExecutionOrder(RegisterAccess &access, Instruction *I) {
    // Assign function-based execution order instead of instruction-based
    access.sequenceNumber = getFunctionExecutionOrder(access.functionName);

    // Determine execution phase
    access.executionPhase = determineExecutionPhase(access.functionName, access.fileName);

    // Build call stack context
    access.callStack = buildCallStackContext(I);

    // Generate basic block identifier
    access.basicBlockId = generateBasicBlockId(I->getParent());

    // Get instruction index within basic block
    access.instructionIndex = getInstructionIndex(I);

    // Determine execution context based on phase and function
    if (access.executionPhase == "board_init") {
        if (access.functionName.find("Clock") != std::string::npos ||
            access.functionName.find("CLOCK") != std::string::npos) {
            access.executionContext = "clock_configuration";
        } else if (access.functionName.find("Pin") != std::string::npos ||
                   access.functionName.find("GPIO") != std::string::npos) {
            access.executionContext = "pin_configuration";
        } else if (access.functionName.find("Power") != std::string::npos ||
                   access.functionName.find("POWER") != std::string::npos) {
            access.executionContext = "power_management";
        } else if (access.functionName.find("MPU") != std::string::npos ||
                   access.functionName.find("BOARD_ConfigMPU") != std::string::npos) {
            access.executionContext = "mpu_configuration";
        } else {
            access.executionContext = "hardware_initialization";
        }
    } else if (access.executionPhase == "driver_init") {
        access.executionContext = "driver_initialization";
    } else {
        if (access.accessType.find("read") != std::string::npos) {
            access.executionContext = "status_monitoring";
        } else {
            access.executionContext = "runtime_operation";
        }
    }
}

std::vector<RegisterAccess> PeripheralAnalysisPass::getChronologicalAccesses() const {
    std::vector<RegisterAccess> chronologicalAccesses = registerAccesses;

    // Sort by sequence number to maintain chronological execution order
    std::sort(chronologicalAccesses.begin(), chronologicalAccesses.end(),
              [](const RegisterAccess& a, const RegisterAccess& b) {
                  return a.sequenceNumber < b.sequenceNumber;
              });

    return chronologicalAccesses;
}

void PeripheralAnalysisPass::exportChronologicalJSON(const std::string& filename) const {
    json::Object root;

    // Get accesses in chronological order
    std::vector<RegisterAccess> chronologicalAccesses = getChronologicalAccesses();

    // Add metadata
    root["analysis_type"] = "chronological_peripheral_access_sequence";
    root["total_accesses"] = static_cast<int64_t>(chronologicalAccesses.size());
    root["description"] = "Peripheral register accesses in chronological execution order";

    // Group by execution phase for summary
    std::map<std::string, int> phaseCount;
    for (const auto& access : chronologicalAccesses) {
        phaseCount[access.executionPhase]++;
    }

    json::Object phaseSummary;
    for (const auto& [phase, count] : phaseCount) {
        phaseSummary[phase] = count;
    }
    root["execution_phase_summary"] = std::move(phaseSummary);

    // Create chronological sequence array
    json::Array sequenceArray;

    for (const auto& access : chronologicalAccesses) {
        json::Object accessObj;

        // Basic access information
        accessObj["sequence_number"] = static_cast<int64_t>(access.sequenceNumber);
        accessObj["peripheral_name"] = access.peripheralName;
        accessObj["register_name"] = access.registerName;
        std::stringstream chrono_addr_ss;
        chrono_addr_ss << "0x" << std::hex << std::uppercase << access.address;
        accessObj["address"] = chrono_addr_ss.str();
        accessObj["access_type"] = access.accessType;
        accessObj["data_size"] = static_cast<int64_t>(access.dataSize);

        // Value information
        if (access.hasValueWritten) {
            std::stringstream value_ss;
            value_ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << access.valueWritten;
            accessObj["value_written"] = value_ss.str();
        } else {
            accessObj["value_written"] = nullptr;
        }

        if (access.hasValueRead) {
            std::stringstream value_ss;
            value_ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << access.valueRead;
            accessObj["value_read"] = value_ss.str();
        } else {
            accessObj["value_read"] = nullptr;
        }

        // Execution order information
        accessObj["execution_phase"] = access.executionPhase;
        accessObj["execution_context"] = access.executionContext;
        accessObj["call_stack"] = access.callStack;
        accessObj["basic_block_id"] = access.basicBlockId;
        accessObj["instruction_index"] = static_cast<int64_t>(access.instructionIndex);

        // Source location
        json::Object sourceObj;
        sourceObj["file"] = access.fileName;
        sourceObj["function"] = access.functionName;
        sourceObj["line"] = static_cast<int64_t>(access.lineNumber);
        accessObj["source_location"] = std::move(sourceObj);

        // Purpose and bit modifications
        accessObj["purpose"] = access.purpose;

        json::Array bitsArray;
        for (const auto& bit : access.bitsModified) {
            bitsArray.push_back(bit);
        }
        accessObj["bits_modified"] = std::move(bitsArray);

        sequenceArray.push_back(std::move(accessObj));
    }

    root["chronological_sequence"] = std::move(sequenceArray);

    // Write to file
    std::error_code EC;
    raw_fd_ostream OS(filename, EC);
    if (EC) {
        errs() << "Error opening file " << filename << ": " << EC.message() << "\n";
        return;
    }

    OS << json::Value(std::move(root)) << "\n";
}

void PeripheralAnalysisPass::analyzeIOPCTLPinMuxSet(CallInst *CI) {
    // IOPCTL_PinMuxSet(uint8_t port, uint8_t pin, uint32_t modefunc)
    if (CI->getNumOperands() < 4) return; // 3 args + function

    // Extract port and pin numbers
    uint32_t port = 0, pin = 0;
    if (auto *portConst = dyn_cast<ConstantInt>(CI->getOperand(0))) {
        port = portConst->getZExtValue();
    }
    if (auto *pinConst = dyn_cast<ConstantInt>(CI->getOperand(1))) {
        pin = pinConst->getZExtValue();
    }

    // Calculate IOPCTL register address based on port/pin
    uint64_t address = calculateIOPCTLRegisterAddress(port, pin);
    std::string peripheralName = getIOPCTLPeripheralName(port);
    std::string registerName = "PIO" + std::to_string(port) + "_" + std::to_string(pin);

    RegisterAccess access;
    access.peripheralName = peripheralName;
    access.registerName = registerName;
    access.address = address;
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {}; // Pin mux configuration affects multiple bits

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "Pin mux configuration for port " + std::to_string(port) + " pin " + std::to_string(pin);

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);

    if (!peripheralName.empty() && peripherals.find(peripheralName) != peripherals.end()) {
        peripherals[peripheralName].accessedAddresses.insert(address);
    }
}

void PeripheralAnalysisPass::analyzeRESETClearPeripheralReset(CallInst *CI) {
    // RESET_ClearPeripheralReset(reset_ip_name_t peripheral)
    if (CI->getNumOperands() < 2) return; // 1 arg + function

    RegisterAccess access;
    access.peripheralName = "RSTCTL";
    access.registerName = "PRSTCTL_CLR";
    access.address = 0x40000070; // Default to RSTCTL0 PRSTCTL0_CLR
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {};

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "Clear peripheral reset";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

uint64_t PeripheralAnalysisPass::calculateIOPCTLRegisterAddress(uint32_t port, uint32_t pin) {
    // Based on IOPCTL_PinMuxSet implementation in fsl_iopctl.h
    if (port >= 8U) { // IOPCTL1
        return 0x40064000 + ((port - 8U) * 0x80) + (pin * 4);
    } else if (port >= 4U) { // IOPCTL2
        return 0x400A5000 + ((port - 4U) * 0x80) + (pin * 4);
    } else { // IOPCTL0
        return 0x40004000 + (port * 0x80) + (pin * 4);
    }
}

std::string PeripheralAnalysisPass::getIOPCTLPeripheralName(uint32_t port) {
    if (port >= 8U) {
        return "IOPCTL1";
    } else if (port >= 4U) {
        return "IOPCTL2";
    } else {
        return "IOPCTL0";
    }
}

void PeripheralAnalysisPass::analyzeCLOCKAttachClk(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "CLKCTL0";
    access.registerName = "CLKSEL";
    access.address = 0x40001434; // MAINCLKSEL register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {};

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "Clock source attachment";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeCLOCKSetClkDiv(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "CLKCTL0";
    access.registerName = "CLKDIV";
    access.address = 0x40001400; // MAINCLKDIV register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {};

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "Clock divider configuration";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeARMMPUSetRegion(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "MPU";
    access.registerName = "RBAR";
    access.address = 0xE000ED9C; // MPU Region Base Address Register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {};

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "MPU region configuration";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeARMMPUEnable(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "MPU";
    access.registerName = "CTRL";
    access.address = 0xE000ED94; // MPU Control Register
    access.accessType = "function_call_write";
    access.dataSize = 32;

    // Extract the parameter value passed to ARM_MPU_Enable
    uint64_t inputMask = 0;
    if (CI->getNumOperands() > 1) { // First operand is the function, second is the first argument
        Value *arg = CI->getOperand(0);
        if (auto *constInt = dyn_cast<ConstantInt>(arg)) {
            inputMask = constInt->getZExtValue();
        } else {
            // Try to evaluate constant expressions
            if (auto *constExpr = dyn_cast<ConstantExpr>(arg)) {
                if (constExpr->getOpcode() == Instruction::Or) {
                    // Handle bitwise OR operations like MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk
                    uint64_t leftVal = 0, rightVal = 0;
                    if (auto *leftConst = dyn_cast<ConstantInt>(constExpr->getOperand(0))) {
                        leftVal = leftConst->getZExtValue();
                    }
                    if (auto *rightConst = dyn_cast<ConstantInt>(constExpr->getOperand(1))) {
                        rightVal = rightConst->getZExtValue();
                    }
                    inputMask = leftVal | rightVal;
                }
            }
        }
    }

    // ARM_MPU_Enable adds the ENABLE bit (bit 0) to the input mask
    // Final value = inputMask | MPU_CTRL_ENABLE_Msk (0x1)
    uint64_t finalValue = inputMask | 0x1;

    // Set the calculated value
    access.valueWritten = finalValue;
    access.hasValueWritten = true;

    // Analyze bit modifications
    std::vector<std::string> bitsModified;
    if (finalValue & 0x1) bitsModified.push_back("ENABLE");
    if (finalValue & 0x2) bitsModified.push_back("HFNMIENA");
    if (finalValue & 0x4) bitsModified.push_back("PRIVDEFENA");
    access.bitsModified = bitsModified;

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "MPU enable with control value 0x" + std::to_string(finalValue);

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeXCACHEEnableCache(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "XCACHE0";
    access.registerName = "CCR";
    access.address = 0x40180000; // Cache Control Register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {};

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "Cache enable";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeXCACHEDisableCache(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "XCACHE0";
    access.registerName = "CCR";
    access.address = 0x40180000; // Cache Control Register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {"ENCACHE"};

    // Extract cache instance parameter if available
    if (CI->getNumOperands() > 1) {
        Value *cacheInstance = CI->getOperand(0);
        if (auto *constInt = dyn_cast<ConstantInt>(cacheInstance)) {
            uint64_t instance = constInt->getZExtValue();
            if (instance == 1) {
                access.peripheralName = "XCACHE1";
                access.address = 0x40190000; // XCACHE1 base address
            }
        }
    }

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "Cache disable";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeARMMPUDisable(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "MPU";
    access.registerName = "CTRL";
    access.address = 0xE000ED94; // MPU Control Register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {"ENABLE"};

    // ARM_MPU_Disable clears the ENABLE bit (sets register to 0)
    access.valueWritten = 0x0;
    access.hasValueWritten = true;

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = "MPU disable";

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeARMMPUSetMemAttr(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "MPU";
    access.registerName = "MAIR0";
    access.address = 0xE000EDC0; // MPU Memory Attribute Indirection Register 0
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {"ATTR0", "ATTR1", "ATTR2", "ATTR3"};

    // Extract memory attribute index and value if available
    if (CI->getNumOperands() > 2) {
        Value *attrIndex = CI->getOperand(0);
        Value *attrValue = CI->getOperand(1);

        if (auto *indexConst = dyn_cast<ConstantInt>(attrIndex)) {
            uint64_t index = indexConst->getZExtValue();
            access.purpose = "MPU memory attribute " + std::to_string(index);

            if (auto *valueConst = dyn_cast<ConstantInt>(attrValue)) {
                uint64_t value = valueConst->getZExtValue();
                access.valueWritten = value;
                access.hasValueWritten = true;
            }
        }
    } else {
        access.purpose = "MPU memory attribute configuration";
    }

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeGPIOPinWrite(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "GPIO1";  // Default to GPIO1, could be enhanced to detect instance
    access.registerName = "PDOR";
    access.address = 0x40100000; // GPIO1 Port Data Output Register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {"PIN_DATA"};

    // Extract pin number and value if available
    if (CI->getNumOperands() > 2) {
        Value *pinValue = CI->getOperand(1);

        if (auto *valueConst = dyn_cast<ConstantInt>(pinValue)) {
            uint64_t value = valueConst->getZExtValue();
            access.valueWritten = value;
            access.hasValueWritten = true;
            access.purpose = std::string("GPIO pin write: ") + (value ? "HIGH" : "LOW");
        } else {
            access.purpose = "GPIO pin write";
        }
    } else {
        access.purpose = "GPIO pin write";
    }

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeGPIOPinRead(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "GPIO1";  // Default to GPIO1
    access.registerName = "PDIR";
    access.address = 0x40100010; // GPIO1 Port Data Input Register
    access.accessType = "function_call_read";
    access.dataSize = 32;
    access.bitsModified = {};
    access.purpose = "GPIO pin read";

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::analyzeGPIOPinInit(CallInst *CI) {
    RegisterAccess access;
    access.peripheralName = "GPIO1";  // Default to GPIO1
    access.registerName = "PDDR";
    access.address = 0x40100014; // GPIO1 Port Data Direction Register
    access.accessType = "function_call_write";
    access.dataSize = 32;
    access.bitsModified = {"PIN_DIRECTION"};
    access.purpose = "GPIO pin initialization";

    auto [fileName, functionName, lineNumber] = getDebugInfo(CI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;

    assignExecutionOrder(access, CI);
    registerAccesses.push_back(access);
}

void PeripheralAnalysisPass::initializeFunctionExecutionOrder() {
    // Define the correct execution order based on actual C code call sequence
    // This matches the order in BOARD_InitHardware() and main()

    functionExecutionOrder.clear();

    // Phase 1: main() function - highest priority
    functionExecutionOrder["main"] = 1000;

    // Phase 2: BOARD_InitHardware() - called from main at line 44
    functionExecutionOrder["BOARD_InitHardware"] = 2000;

    // Phase 3: BOARD_ConfigMPU() - FIRST call in BOARD_InitHardware (line 136)
    functionExecutionOrder["BOARD_ConfigMPU"] = 3000;
    functionExecutionOrder["ARM_MPU_Disable"] = 3010;
    functionExecutionOrder["ARM_MPU_SetMemAttr"] = 3020;
    functionExecutionOrder["ARM_MPU_SetRegion"] = 3030;
    functionExecutionOrder["ARM_MPU_Enable"] = 3040;

    // Phase 4: BOARD_InitPins() - SECOND call in BOARD_InitHardware (line 137)
    functionExecutionOrder["BOARD_InitPins"] = 4000;
    functionExecutionOrder["BOARD_InitPsRamPins_Xspi1"] = 4010;
    functionExecutionOrder["IOPCTL_PinMuxSet"] = 4020;

    // Phase 5: BOARD_BootClockRUN() - THIRD call in BOARD_InitHardware (line 138)
    functionExecutionOrder["BOARD_BootClockRUN"] = 5000;
    functionExecutionOrder["CLOCK_AttachClk"] = 5010;
    functionExecutionOrder["CLOCK_SetClkDiv"] = 5020;
    functionExecutionOrder["RESET_ClearPeripheralReset"] = 5030;

    // Phase 6: BOARD_InitDebugConsole() - FOURTH call in BOARD_InitHardware (line 139)
    functionExecutionOrder["BOARD_InitDebugConsole"] = 6000;

    // Phase 7: Cache operations - typically after board init
    functionExecutionOrder["XCACHE_EnableCache"] = 7000;
    functionExecutionOrder["XCACHE_DisableCache"] = 7010;

    // Phase 8: Application-specific initialization
    functionExecutionOrder["XSPI_Init"] = 8000;
    functionExecutionOrder["XSPI_SetFlashConfig"] = 8010;

    // Phase 9: Runtime operations
    functionExecutionOrder["GPIO_PinWrite"] = 9000;
    functionExecutionOrder["GPIO_PinRead"] = 9010;
    functionExecutionOrder["GPIO_PinInit"] = 9020;
}

uint64_t PeripheralAnalysisPass::getFunctionExecutionOrder(const std::string& functionName) {
    // Check if we have a specific order for this function
    auto it = functionExecutionOrder.find(functionName);
    if (it != functionExecutionOrder.end()) {
        return it->second;
    }

    // Check for partial matches for functions with similar names
    for (const auto& [orderFunc, order] : functionExecutionOrder) {
        if (functionName.find(orderFunc) != std::string::npos) {
            return order + 5; // Slightly later than the exact match
        }
    }

    // Default ordering based on function name patterns
    if (functionName.find("main") != std::string::npos) {
        return 1000;
    } else if (functionName.find("BOARD_InitHardware") != std::string::npos) {
        return 2000;
    } else if (functionName.find("BOARD_ConfigMPU") != std::string::npos ||
               functionName.find("MPU") != std::string::npos) {
        return 3000;
    } else if (functionName.find("BOARD_InitPins") != std::string::npos ||
               functionName.find("Pin") != std::string::npos ||
               functionName.find("IOPCTL") != std::string::npos) {
        return 4000;
    } else if (functionName.find("BOARD_BootClockRUN") != std::string::npos ||
               functionName.find("Clock") != std::string::npos ||
               functionName.find("CLOCK") != std::string::npos) {
        return 5000;
    } else if (functionName.find("BOARD_InitDebugConsole") != std::string::npos) {
        return 6000;
    } else if (functionName.find("XCACHE") != std::string::npos ||
               functionName.find("Cache") != std::string::npos) {
        return 7000;
    } else if (functionName.find("XSPI") != std::string::npos) {
        return 8000;
    } else if (functionName.find("GPIO") != std::string::npos) {
        return 9000;
    }

    // Default for unknown functions - place them at the end
    return 10000 + globalSequenceCounter++;
}
