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
    
    // Clear previous results
    registerAccesses.clear();
    
    // Analyze each function in the module
    for (Function &F : M) {
        if (!F.isDeclaration()) {
            analyzeFunction(F);
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
    // XSPI0 Peripheral (Secure)
    PeripheralInfo xspi0;
    xspi0.name = "XSPI0";
    xspi0.baseAddress = 0x50184000;
    xspi0.registers[0x50184000] = "MCR";      // Module Configuration Register
    xspi0.registers[0x50184008] = "IPCR";     // IP Configuration Register
    xspi0.registers[0x5018400C] = "FLSHCR";   // Flash Memory Configuration Register
    xspi0.registers[0x50184010] = "BUFCR0";   // Buffer Configuration Register 0
    xspi0.registers[0x50184014] = "BUFCR1";   // Buffer Configuration Register 1
    xspi0.registers[0x50184018] = "BUFCR2";   // Buffer Configuration Register 2
    xspi0.registers[0x5018401C] = "BUFCR3";   // Buffer Configuration Register 3
    xspi0.registers[0x50184020] = "BFGENCR";  // Buffer Generic Configuration Register
    xspi0.registers[0x50184024] = "SOCCR";    // SOC Configuration Register
    xspi0.registers[0x50184100] = "SFAR";     // Serial Flash Memory Address Register
    xspi0.registers[0x50184104] = "SFACR";    // Serial Flash Memory Address Configuration Register
    xspi0.registers[0x50184108] = "SMPR";     // Sampling Register
    xspi0.registers[0x50184300] = "LUTKEY";   // LUT Key Register
    xspi0.registers[0x50184304] = "LCKCR";    // LUT Lock Configuration Register
    peripherals["XSPI0"] = xspi0;
    
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
    
    // XSPI2 Peripheral
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
    
    // Add GPIO peripherals
    PeripheralInfo gpio0;
    gpio0.name = "GPIO0";
    gpio0.baseAddress = 0x40100000;
    gpio0.registers[0x40100000] = "PDOR";  // Port Data Output Register
    gpio0.registers[0x40100004] = "PSOR";  // Port Set Output Register
    gpio0.registers[0x40100008] = "PCOR";  // Port Clear Output Register
    gpio0.registers[0x4010000C] = "PTOR";  // Port Toggle Output Register
    gpio0.registers[0x40100010] = "PDIR";  // Port Data Input Register
    gpio0.registers[0x40100014] = "PDDR";  // Port Data Direction Register
    peripherals["GPIO0"] = gpio0;
    
    // Add POWER peripheral
    PeripheralInfo power;
    power.name = "POWER";
    power.baseAddress = 0x40020000;
    power.registers[0x40020000] = "PDRUNCFG0";
    power.registers[0x40020004] = "PDRUNCFG1";
    power.registers[0x40020008] = "PDRUNCFG2";
    power.registers[0x4002000C] = "PDRUNCFG3";
    power.registers[0x40020010] = "PDRUNCFG4";
    power.registers[0x40020014] = "PDRUNCFG5";
    power.registers[0x40020018] = "PDRUNCFG6";
    power.registers[0x4002001C] = "PDRUNCFG7";
    power.registers[0x40020020] = "PDSLEEPCFG0";
    power.registers[0x40020024] = "PDSLEEPCFG1";
    power.registers[0x40020028] = "PDSLEEPCFG2";
    power.registers[0x4002002C] = "PDSLEEPCFG3";
    peripherals["POWER"] = power;

    // Add SYSCON0 peripheral (System Control)
    PeripheralInfo syscon0;
    syscon0.name = "SYSCON0";
    syscon0.baseAddress = 0x40000000;
    syscon0.registers[0x40000000] = "MEMORYREMAP";
    syscon0.registers[0x40000004] = "AHBMATPRIO";
    syscon0.registers[0x40000008] = "CPU0STCKCAL";
    syscon0.registers[0x4000000C] = "CPU0NMISRC";
    syscon0.registers[0x40000010] = "CPU0NMISTAT";
    syscon0.registers[0x40000014] = "CPU0NMIEN";
    syscon0.registers[0x40000018] = "CPU1STCKCAL";
    syscon0.registers[0x4000001C] = "CPU1NMISRC";
    syscon0.registers[0x40000020] = "CPU1NMISTAT";
    syscon0.registers[0x40000024] = "CPU1NMIEN";
    syscon0.registers[0x40000100] = "SEC_CLK_CTRL";  // Important for TRNG
    syscon0.registers[0x40000104] = "SEC_VIO_INFO_VALID";
    syscon0.registers[0x40000108] = "SEC_VIO_INFO";
    syscon0.registers[0x4000010C] = "SEC_GPIO_MASK0";
    syscon0.registers[0x40000110] = "SEC_GPIO_MASK1";
    syscon0.registers[0x40000114] = "SEC_DSP_INT_MASK";
    peripherals["SYSCON0"] = syscon0;

    // Add SYSCON1 peripheral
    PeripheralInfo syscon1;
    syscon1.name = "SYSCON1";
    syscon1.baseAddress = 0x40001000;
    syscon1.registers[0x40001000] = "UPDATELCKOUT";
    syscon1.registers[0x40001004] = "FCCTRLSEL";
    syscon1.registers[0x40001008] = "SHAREDCTRLSET";
    syscon1.registers[0x4000100C] = "SHAREDCTRLCLR";
    peripherals["SYSCON1"] = syscon1;

    // Add CLKCTL0 peripheral (Clock Control)
    PeripheralInfo clkctl0;
    clkctl0.name = "CLKCTL0";
    clkctl0.baseAddress = 0x40002000;
    clkctl0.registers[0x40002000] = "PSCCTL0";
    clkctl0.registers[0x40002004] = "PSCCTL1";
    clkctl0.registers[0x40002008] = "PSCCTL2";
    clkctl0.registers[0x4000200C] = "PSCCTL3";
    clkctl0.registers[0x40002010] = "PSCCTL4";
    clkctl0.registers[0x40002020] = "XTAL32MCTRL";
    clkctl0.registers[0x40002024] = "XTAL32MSTAT";
    clkctl0.registers[0x40002030] = "RTCOSC32KCTRL";
    clkctl0.registers[0x40002034] = "RTCOSC32KSTAT";
    clkctl0.registers[0x40002040] = "FRO0CLKSEL";
    clkctl0.registers[0x40002044] = "FRO0CLKSTAT";
    clkctl0.registers[0x40002048] = "FRO0CLKDIV";
    clkctl0.registers[0x40002100] = "MAINCLKSELB";
    clkctl0.registers[0x40002104] = "MAINCLKSTAT";
    clkctl0.registers[0x40002108] = "MAINCLKDIV";
    clkctl0.registers[0x40002200] = "XSPI0CLKSEL";
    clkctl0.registers[0x40002204] = "XSPI0CLKSTAT";
    clkctl0.registers[0x40002208] = "XSPI0CLKDIV";
    clkctl0.registers[0x40002210] = "XSPI1CLKSEL";
    clkctl0.registers[0x40002214] = "XSPI1CLKSTAT";
    clkctl0.registers[0x40002218] = "XSPI1CLKDIV";
    clkctl0.registers[0x40002220] = "XSPI2CLKSEL";
    clkctl0.registers[0x40002224] = "XSPI2CLKSTAT";
    clkctl0.registers[0x40002228] = "XSPI2CLKDIV";
    peripherals["CLKCTL0"] = clkctl0;

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

    // Add RSTCTL0 peripheral (Reset Control)
    PeripheralInfo rstctl0;
    rstctl0.name = "RSTCTL0";
    rstctl0.baseAddress = 0x40004000;
    rstctl0.registers[0x40004000] = "PRSTCTL0";
    rstctl0.registers[0x40004004] = "PRSTCTL1";
    rstctl0.registers[0x40004008] = "PRSTCTL2";
    rstctl0.registers[0x4000400C] = "PRSTCTL3";
    rstctl0.registers[0x40004010] = "PRSTCTL4";
    rstctl0.registers[0x40004020] = "PRSTCTLSET0";
    rstctl0.registers[0x40004024] = "PRSTCTLSET1";
    rstctl0.registers[0x40004028] = "PRSTCTLSET2";
    rstctl0.registers[0x4000402C] = "PRSTCTLSET3";
    rstctl0.registers[0x40004030] = "PRSTCTLSET4";
    rstctl0.registers[0x40004040] = "PRSTCTLCLR0";
    rstctl0.registers[0x40004044] = "PRSTCTLCLR1";
    rstctl0.registers[0x40004048] = "PRSTCTLCLR2";
    rstctl0.registers[0x4000404C] = "PRSTCTLCLR3";
    rstctl0.registers[0x40004050] = "PRSTCTLCLR4";
    peripherals["RSTCTL0"] = rstctl0;

    // Add RSTCTL1 peripheral
    PeripheralInfo rstctl1;
    rstctl1.name = "RSTCTL1";
    rstctl1.baseAddress = 0x40005000;
    rstctl1.registers[0x40005000] = "PRSTCTL0";
    rstctl1.registers[0x40005004] = "PRSTCTL1";
    rstctl1.registers[0x40005008] = "PRSTCTL2";
    rstctl1.registers[0x4000500C] = "PRSTCTL3";
    rstctl1.registers[0x40005010] = "PRSTCTL4";
    rstctl1.registers[0x40005020] = "PRSTCTLSET0";
    rstctl1.registers[0x40005024] = "PRSTCTLSET1";
    rstctl1.registers[0x40005028] = "PRSTCTLSET2";
    rstctl1.registers[0x4000502C] = "PRSTCTLSET3";
    rstctl1.registers[0x40005030] = "PRSTCTLSET4";
    rstctl1.registers[0x40005040] = "PRSTCTLCLR0";
    rstctl1.registers[0x40005044] = "PRSTCTLCLR1";
    rstctl1.registers[0x40005048] = "PRSTCTLCLR2";
    rstctl1.registers[0x4000504C] = "PRSTCTLCLR3";
    rstctl1.registers[0x40005050] = "PRSTCTLCLR4";
    peripherals["RSTCTL1"] = rstctl1;

    // Add IOPCTL0 peripheral (I/O Pin Control)
    PeripheralInfo iopctl0;
    iopctl0.name = "IOPCTL0";
    iopctl0.baseAddress = 0x40140000;
    iopctl0.registers[0x40140000] = "PIO0_0";
    iopctl0.registers[0x40140004] = "PIO0_1";
    iopctl0.registers[0x40140008] = "PIO0_2";
    iopctl0.registers[0x4014000C] = "PIO0_3";
    iopctl0.registers[0x40140010] = "PIO0_4";
    iopctl0.registers[0x40140014] = "PIO0_5";
    iopctl0.registers[0x40140018] = "PIO0_6";
    iopctl0.registers[0x4014001C] = "PIO0_7";
    // Add more PIO registers as needed
    peripherals["IOPCTL0"] = iopctl0;

    // Add IOPCTL1 peripheral
    PeripheralInfo iopctl1;
    iopctl1.name = "IOPCTL1";
    iopctl1.baseAddress = 0x40141000;
    iopctl1.registers[0x40141000] = "PIO1_0";
    iopctl1.registers[0x40141004] = "PIO1_1";
    iopctl1.registers[0x40141008] = "PIO1_2";
    iopctl1.registers[0x4014100C] = "PIO1_3";
    iopctl1.registers[0x40141010] = "PIO1_4";
    iopctl1.registers[0x40141014] = "PIO1_5";
    iopctl1.registers[0x40141018] = "PIO1_6";
    iopctl1.registers[0x4014101C] = "PIO1_7";
    peripherals["IOPCTL1"] = iopctl1;

    // Add IOPCTL2 peripheral
    PeripheralInfo iopctl2;
    iopctl2.name = "IOPCTL2";
    iopctl2.baseAddress = 0x40142000;
    iopctl2.registers[0x40142000] = "PIO2_0";
    iopctl2.registers[0x40142004] = "PIO2_1";
    iopctl2.registers[0x40142008] = "PIO2_2";
    iopctl2.registers[0x4014200C] = "PIO2_3";
    iopctl2.registers[0x40142010] = "PIO2_4";
    iopctl2.registers[0x40142014] = "PIO2_5";
    iopctl2.registers[0x40142018] = "PIO2_6";
    iopctl2.registers[0x4014201C] = "PIO2_7";
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

    // Add TRNG peripheral (True Random Number Generator)
    PeripheralInfo trng;
    trng.name = "TRNG";
    trng.baseAddress = 0x40070000;
    trng.registers[0x40070000] = "MCTL";
    trng.registers[0x40070004] = "SCMISC";
    trng.registers[0x40070008] = "PKRRNG";
    trng.registers[0x4007000C] = "PKRMAX";
    trng.registers[0x40070010] = "PKRCNT10";
    trng.registers[0x40070014] = "PKRCNT32";
    trng.registers[0x40070018] = "PKRCNT54";
    trng.registers[0x4007001C] = "PKRCNT76";
    trng.registers[0x40070020] = "PKRCNT98";
    trng.registers[0x40070024] = "PKRCNTBA";
    trng.registers[0x40070028] = "PKRCNTDC";
    trng.registers[0x4007002C] = "PKRCNTFE";
    trng.registers[0x40070030] = "SEC_CFG";
    trng.registers[0x40070034] = "INT_CTRL";
    trng.registers[0x40070038] = "INT_MASK";
    trng.registers[0x4007003C] = "INT_STATUS";
    trng.registers[0x40070080] = "VID1";
    trng.registers[0x40070084] = "VID2";
    trng.registers[0x40070088] = "ST_STATUS";
    trng.registers[0x4007008C] = "ENT0";
    trng.registers[0x40070090] = "ENT1";
    trng.registers[0x40070094] = "ENT2";
    trng.registers[0x40070098] = "ENT3";
    trng.registers[0x4007009C] = "ENT4";
    trng.registers[0x400700A0] = "ENT5";
    trng.registers[0x400700A4] = "ENT6";
    trng.registers[0x400700A8] = "ENT7";
    trng.registers[0x400700AC] = "ENT8";
    trng.registers[0x400700B0] = "ENT9";
    trng.registers[0x400700B4] = "ENT10";
    trng.registers[0x400700B8] = "ENT11";
    trng.registers[0x400700BC] = "ENT12";
    trng.registers[0x400700C0] = "ENT13";
    trng.registers[0x400700C4] = "ENT14";
    trng.registers[0x400700C8] = "ENT15";
    peripherals["TRNG"] = trng;
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
            }
        }
    }
}

void PeripheralAnalysisPass::analyzeLoadInstruction(LoadInst *LI) {
    uint64_t address = getEffectiveAddress(LI->getPointerOperand());
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
    access.accessType = "read";
    access.dataSize = getDataSizeFromType(LI->getType());
    access.bitsModified = {}; // No bits modified for reads
    
    auto [fileName, functionName, lineNumber] = getDebugInfo(LI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = determinePurpose(LI, peripheralName, registerName);
    
    registerAccesses.push_back(access);
    peripherals[peripheralName].accessedAddresses.insert(address);
}

void PeripheralAnalysisPass::analyzeStoreInstruction(StoreInst *SI) {
    uint64_t address = getEffectiveAddress(SI->getPointerOperand());
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
    access.accessType = "write";
    access.dataSize = getDataSizeFromType(SI->getValueOperand()->getType());
    access.bitsModified = analyzeBitfieldOperations(SI);
    
    auto [fileName, functionName, lineNumber] = getDebugInfo(SI);
    access.fileName = fileName;
    access.functionName = functionName;
    access.lineNumber = lineNumber;
    access.purpose = determinePurpose(SI, peripheralName, registerName);
    
    registerAccesses.push_back(access);
    peripherals[peripheralName].accessedAddresses.insert(address);
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
    
    registerAccesses.push_back(access);
    peripherals[peripheralName].accessedAddresses.insert(address);
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
        uint64_t baseAddr = getEffectiveAddress(GEP->getPointerOperand());
        if (baseAddr != 0) {
            // Calculate offset from GEP indices
            uint64_t offset = 0;
            for (unsigned i = 1; i < GEP->getNumOperands(); ++i) {
                if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(i))) {
                    offset += CI->getZExtValue() * 4; // Assume 32-bit registers
                }
            }
            return baseAddr + offset;
        }
    }

    return 0; // Unknown address
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
            peripheralObj["base_address"] = "0x" + std::to_string(it->second.baseAddress);
        }

        json::Array accessArray;
        for (const auto* access : accesses) {
            json::Object accessObj;
            accessObj["register_name"] = access->registerName;
            accessObj["address"] = "0x" + std::to_string(access->address);
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
