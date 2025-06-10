//===-- PeripheralAnalysisPass.cpp - MIMXRT798S Peripheral Analysis Pass --===//
//
// REBUILT PERIPHERAL ANALYSIS PASS FOR MIMXRT798S
// Generated from real NXP device header files
// 
// This pass analyzes LLVM IR to identify peripheral register accesses
// using VERIFIED peripheral definitions from MIMXRT798S device specifications.
//
//===----------------------------------------------------------------------===//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace llvm;

// Command line options (using unique names to avoid conflicts)
static cl::opt<std::string> PeriphOutputFile("periph-output", cl::desc("Output file for peripheral analysis results"), cl::value_desc("filename"));
static cl::opt<bool> PeriphVerbose("periph-verbose", cl::desc("Enable verbose peripheral analysis output"));
static cl::opt<bool> PeriphChronological("periph-chronological", cl::desc("Enable chronological execution order tracking"));

namespace {

// Structure to hold peripheral register information
struct RegisterInfo {
    std::string name;
    uint64_t offset;
    std::string description;
    std::string access_type;
};

// Structure to hold peripheral information
struct PeripheralInfo {
    std::string name;
    uint64_t baseAddress;
    std::map<uint64_t, RegisterInfo> registers;
    std::string description;
};

// Structure to hold register access information
struct RegisterAccess {
    std::string peripheral_name;
    std::string register_name;
    uint64_t address;
    std::string access_type;
    std::string function_name;
    std::string source_file;
    int source_line;
    std::vector<std::string> bits_modified;
    std::string purpose;
    int sequence_number;
    std::string execution_phase;
};

class PeripheralAnalysisPass : public ModulePass {
public:
    static char ID;
    
    PeripheralAnalysisPass() : ModulePass(ID) {
        initializePeripheralDefinitions();
    }
    
    bool runOnModule(Module &M) override;
    
private:
    std::map<std::string, PeripheralInfo> peripherals;
    std::vector<RegisterAccess> register_accesses;
    int global_sequence_counter;
    
    void initializePeripheralDefinitions();
    bool analyzeInstruction(Instruction *I, Function *F);
    std::pair<std::string, std::string> identifyPeripheralRegister(uint64_t address);
    std::string getSourceLocation(Instruction *I, std::string &file, int &line);
    std::string determineAccessType(Instruction *I);
    std::vector<std::string> analyzeBitsModified(Instruction *I, uint64_t address);
    std::string determinePurpose(const std::string &peripheral, const std::string &reg, const std::string &func);
    std::string determineExecutionPhase(const std::string &function_name);
    void generateAnalysisReport();
    void generateChronologicalReport();
};

char PeripheralAnalysisPass::ID = 0;

void PeripheralAnalysisPass::initializePeripheralDefinitions() {
    // REAL MIMXRT798S PERIPHERAL DEFINITIONS
    // Extracted from official NXP device header files
    // Base addresses verified from MIMXRT798S_cm33_core0_COMMON.h
    
    global_sequence_counter = 0;
    
    // CLKCTL0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo clkctl0;
    clkctl0.name = "CLKCTL0";
    clkctl0.baseAddress = 0x40001000;
    clkctl0.description = "MIMXRT798S CLKCTL0 Peripheral";
    
    clkctl0.registers[0x40001010] = {
        "PSCCTL0",
        0x010,
        "VDD2_COMP Peripheral Clock Control 0",
        "RW"
    };
    clkctl0.registers[0x40001014] = {
        "PSCCTL1",
        0x014,
        "VDD2_COMP Peripheral Clock Control 1",
        "RW"
    };
    clkctl0.registers[0x40001018] = {
        "PSCCTL2",
        0x018,
        "VDD2_COMP Peripheral Clock Control 2",
        "RW"
    };
    clkctl0.registers[0x4000101C] = {
        "PSCCTL3",
        0x01C,
        "VDD2_COMP Peripheral Clock Control 3",
        "RW"
    };
    clkctl0.registers[0x40001020] = {
        "PSCCTL4",
        0x020,
        "VDD2_COMP Peripheral Clock Control 4",
        "RW"
    };
    clkctl0.registers[0x40001024] = {
        "PSCCTL5",
        0x024,
        "VDD2_COMP Peripheral Clock Control 5",
        "RW"
    };
    clkctl0.registers[0x40001040] = {
        "PSCCTL0_SET",
        0x040,
        "VDD2_COMP Peripheral Clock Control 0 Set",
        "RW"
    };
    clkctl0.registers[0x40001044] = {
        "PSCCTL1_SET",
        0x044,
        "VDD2_COMP Peripheral Clock Control 1 Set",
        "RW"
    };
    clkctl0.registers[0x40001048] = {
        "PSCCTL2_SET",
        0x048,
        "VDD2_COMP Peripheral Clock Control 2 Set",
        "RW"
    };
    clkctl0.registers[0x4000104C] = {
        "PSCCTL3_SET",
        0x04C,
        "VDD2_COMP Peripheral Clock Control 3 Set",
        "RW"
    };
    clkctl0.registers[0x40001050] = {
        "PSCCTL4_SET",
        0x050,
        "VDD2_COMP Peripheral Clock Control 4 Set",
        "RW"
    };
    clkctl0.registers[0x40001054] = {
        "PSCCTL5_SET",
        0x054,
        "VDD2_COMP Peripheral Clock Control 5 Set",
        "RW"
    };
    clkctl0.registers[0x40001070] = {
        "PSCCTL0_CLR",
        0x070,
        "VDD2_COMP Peripheral Clock Control 0 Clear",
        "RW"
    };
    clkctl0.registers[0x40001074] = {
        "PSCCTL1_CLR",
        0x074,
        "VDD2_COMP Peripheral Clock Control 1 Clear",
        "RW"
    };
    clkctl0.registers[0x40001078] = {
        "PSCCTL2_CLR",
        0x078,
        "VDD2_COMP Peripheral Clock Control 2 Clear",
        "RW"
    };
    clkctl0.registers[0x4000107C] = {
        "PSCCTL3_CLR",
        0x07C,
        "VDD2_COMP Peripheral Clock Control 3 Clear",
        "RW"
    };
    clkctl0.registers[0x40001080] = {
        "PSCCTL4_CLR",
        0x080,
        "VDD2_COMP Peripheral Clock Control 4 Clear",
        "RW"
    };
    clkctl0.registers[0x40001084] = {
        "PSCCTL5_CLR",
        0x084,
        "VDD2_COMP Peripheral Clock Control 5 Clear",
        "RW"
    };
    clkctl0.registers[0x40001090] = {
        "ONE_SRC_CLKSLICE_ENABLE",
        0x090,
        "One Source Clock Slice Enable",
        "RW"
    };
    clkctl0.registers[0x40001128] = {
        "FRO0MAXDOMAINEN",
        0x128,
        "FRO0MAX Clock Domain Enable",
        "RW"
    };
    clkctl0.registers[0x40001400] = {
        "MAINCLKDIV",
        0x400,
        "VDD2_COMP Main Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001420] = {
        "CMPTBASECLKSEL",
        0x420,
        "VDD2_COMP Base Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001424] = {
        "DSPBASECLKSEL",
        0x424,
        "VDD2_DSP Base Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001428] = {
        "VDD2COMBASECLKSEL",
        0x428,
        "VDD2_COM Base Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001434] = {
        "MAINCLKSEL",
        0x434,
        "VDD2_COMP Main Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001440] = {
        "DSPCPUCLKDIV",
        0x440,
        "VDD2_DSP Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001444] = {
        "DSPCPUCLKSEL",
        0x444,
        "VDD2_DSP Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001450] = {
        "RAMCLKSEL",
        0x450,
        "RAM Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x4000145C] = {
        "RAMCLKDIV",
        0x45C,
        "RAM Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001560] = {
        "TPIUFCLKSEL",
        0x560,
        "TPIU (TRACE_RT700) Functional Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001564] = {
        "TPIUCLKDIV",
        0x564,
        "TPIU (TRACE_RT700) Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001600] = {
        "XSPI0FCLKSEL",
        0x600,
        "XSPI0 Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001604] = {
        "XSPI0FCLKDIV",
        0x604,
        "XSPI0 Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001620] = {
        "XSPI1FCLKSEL",
        0x620,
        "XSPI1 Functional Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001624] = {
        "XSPI1FCLKDIV",
        0x624,
        "XSPI1 Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001640] = {
        "SCTFCLKSEL",
        0x640,
        "SCT Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001644] = {
        "SCTFCLKDIV",
        0x644,
        "SCT Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001700] = {
        "UTICK0FCLKSEL",
        0x700,
        "UTICK0 Functional Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001704] = {
        "UTICK0FCLKDIV",
        0x704,
        "UTICK0 Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001720] = {
        "WWDT0FCLKSEL",
        0x720,
        "WWDT0 Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001740] = {
        "WWDT1FCLKSEL",
        0x740,
        "WWDT1 Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001760] = {
        "SYSTICKFCLKSEL",
        0x760,
        "SYSTICK Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001764] = {
        "SYSTICKFCLKDIV",
        0x764,
        "SYSTICK Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001800] = {
        "FCCLKSEL",
        0x800,
        "LP_FLEXCOMM 0 to 13 Clock Source 0 Select..LP_FLEX...",
        "RW"
    };
    clkctl0.registers[0x40001804] = {
        "FCCLKDIV",
        0x804,
        "LP_FLEXCOMM 0 to 13 Clock Source 0 Divider..LP_FLE...",
        "RW"
    };
    clkctl0.registers[0x40001808] = {
        "FCFCLKSEL",
        0x808,
        "LP_FLEXCOMM0 Clock Source Select..LP_FLEXCOMM13 Cl...",
        "RW"
    };
    clkctl0.registers[0x400019C8] = {
        "SAI012FCLKSEL",
        0x9C8,
        "SAI0",
        "RW"
    };
    clkctl0.registers[0x400019CC] = {
        "SAI012CLKDIV",
        0x9CC,
        "SAI0",
        "RW"
    };
    clkctl0.registers[0x40001A00] = {
        "CTIMERCLKDIV",
        0xA00,
        "CTIMER0 Functional Clock Divider..CTIMER4 Function...",
        "RW"
    };
    clkctl0.registers[0x40001AA0] = {
        "CTIMERFCLKSEL",
        0xAA0,
        "CTIMER0 Functional Clock Source Select..CTIMER4 Fu...",
        "RW"
    };
    clkctl0.registers[0x40001AC0] = {
        "TRNGFCLKSEL",
        0xAC0,
        "TRNG Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001AC4] = {
        "TRNGFCLKDIV",
        0xAC4,
        "TRNG FCLK Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001B00] = {
        "I3C01FCLKSEL",
        0xB00,
        "I3C0 and I3C1 Functional Clock Source Select",
        "RW"
    };
    clkctl0.registers[0x40001B04] = {
        "I3C01PCLKSEL",
        0xB04,
        "I3C0 and I3C1 P-CLK Source Select",
        "RW"
    };
    clkctl0.registers[0x40001B08] = {
        "I3C01PCLKDIV",
        0xB08,
        "I3C0 and I3C1 P-CLK Divider",
        "RW"
    };
    clkctl0.registers[0x40001B10] = {
        "I3C01FCLKDIV",
        0xB10,
        "I3C0 and I3C1 Functional Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001B20] = {
        "CLKOUTCLKSEL",
        0xB20,
        "CLKOUT_VDD2 Clock Select Source",
        "RW"
    };
    clkctl0.registers[0x40001B24] = {
        "CLKOUTCLKDIV",
        0xB24,
        "CLKOUT_VDD2 Clock Divider",
        "RW"
    };
    clkctl0.registers[0x40001B30] = {
        "AUDIOVDD2CLKSEL",
        0xB30,
        "VDD2_COMP Audio Clock Source Select",
        "RW"
    };
    peripherals["CLKCTL0"] = clkctl0;
    
    // SYSCON0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo syscon0;
    syscon0.name = "SYSCON0";
    syscon0.baseAddress = 0x40002000;
    syscon0.description = "MIMXRT798S SYSCON0 Peripheral";
    
    syscon0.registers[0x40002000] = {
        "SEC_CLK_CTRL",
        0x000,
        "Security Clock Control",
        "RW"
    };
    syscon0.registers[0x4000200C] = {
        "GDET_CTRL",
        0x00C,
        "GDET0 Control",
        "RW"
    };
    syscon0.registers[0x40002014] = {
        "NMISRC",
        0x014,
        "NMI Source Select",
        "RW"
    };
    syscon0.registers[0x4000201C] = {
        "CTIMERGLOBALSTARTEN",
        0x01C,
        "CTIMER Global Start Enable",
        "RW"
    };
    syscon0.registers[0x40002078] = {
        "AHBMATPRIO",
        0x078,
        "Bus Matrix Priority",
        "RW"
    };
    syscon0.registers[0x40002084] = {
        "LATCHED_CM33_TXEV",
        0x084,
        "Latched Cortex-M33 Transmit Event",
        "RW"
    };
    syscon0.registers[0x40002090] = {
        "SYSTEM_STICK_CALIB",
        0x090,
        "System Secure Tick Calibration",
        "RW"
    };
    syscon0.registers[0x40002094] = {
        "SYSTEM_NSTICK_CALIB",
        0x094,
        "System Non-Secure Tick Calibration",
        "RW"
    };
    syscon0.registers[0x400020D0] = {
        "GPIO_PSYNC",
        0x0D0,
        "GPIO Synchronization Stages",
        "RW"
    };
    syscon0.registers[0x40002114] = {
        "AUTOCLKGATEOVERRIDE0",
        0x114,
        "Automatic Clock Gate Override",
        "RW"
    };
    syscon0.registers[0x40002118] = {
        "SRAM_CLKGATE_CTRL",
        0x118,
        "SRAM Clock Gating Control",
        "RW"
    };
    syscon0.registers[0x40002124] = {
        "OCOTP_MEM_CTL",
        0x124,
        "OCOTP Memory Control",
        "RW"
    };
    syscon0.registers[0x40002128] = {
        "ELS_MEM_CTL",
        0x128,
        "ELS Memory Control",
        "RW"
    };
    syscon0.registers[0x40002130] = {
        "MMU0_MEM_CTRL",
        0x130,
        "MMU0 Memory Control",
        "RW"
    };
    syscon0.registers[0x40002140] = {
        "EDMA0_MEM_CTRL",
        0x140,
        "eDMA0 Memory Control",
        "RW"
    };
    syscon0.registers[0x40002144] = {
        "EDMA1_MEM_CTRL",
        0x144,
        "eDMA1 Memory Control",
        "RW"
    };
    syscon0.registers[0x4000214C] = {
        "ETF_MEM_CTRL",
        0x14C,
        "ETF Memory Control",
        "RW"
    };
    syscon0.registers[0x40002150] = {
        "MMU1_MEM_CTRL",
        0x150,
        "MMU1 Memory Control",
        "RW"
    };
    syscon0.registers[0x40002154] = {
        "XSPI0_MEM_CTRL",
        0x154,
        "XSPI0 Memory Control",
        "RW"
    };
    syscon0.registers[0x40002158] = {
        "XSPI1_MEM_CTRL",
        0x158,
        "XSPI1 Memory Control",
        "RW"
    };
    syscon0.registers[0x4000215C] = {
        "XSPI0_DATA_MEM_CTRL",
        0x15C,
        "CACHE64_CTRL0 Data Memory Control",
        "RW"
    };
    syscon0.registers[0x40002160] = {
        "XSPI1_DATA_MEM_CTRL",
        0x160,
        "CACHE64_CTRL1 Data Memory Control",
        "RW"
    };
    syscon0.registers[0x40002164] = {
        "NPU_MEM_CTRL",
        0x164,
        "NPU Memory Control",
        "RW"
    };
    syscon0.registers[0x40002168] = {
        "PKC0_MEM_CTRL",
        0x168,
        "PKC Memory 0 Control",
        "RW"
    };
    syscon0.registers[0x4000216C] = {
        "PKC1_MEM_CTRL",
        0x16C,
        "PKC Memory 1 Control",
        "RW"
    };
    syscon0.registers[0x40002174] = {
        "CM33_MEM_DATA_CTRL",
        0x174,
        "CPU0 Memory Data Control",
        "RW"
    };
    syscon0.registers[0x40002178] = {
        "CM33_MEM_TAG_CTRL",
        0x178,
        "CPU0 Memory Tag Control",
        "RW"
    };
    syscon0.registers[0x40002208] = {
        "HIFI4_MEM_CTL",
        0x208,
        "HiFi4 Memory Control",
        "RW"
    };
    syscon0.registers[0x40002240] = {
        "SAI0_MCLK_CTRL",
        0x240,
        "SAI0-2 MCLK IO Direction Control",
        "RW"
    };
    syscon0.registers[0x4000225C] = {
        "XSPI0_TAG_MEM_CTRL",
        0x25C,
        "CACHE64_CTRL0 Tag Memory Control",
        "RW"
    };
    syscon0.registers[0x40002260] = {
        "XSPI1_TAG_MEM_CTRL",
        0x260,
        "CACHE64_CTRL1 Tag Memory Control",
        "RW"
    };
    syscon0.registers[0x40002280] = {
        "COMP_AUTOGATE_EN",
        0x280,
        "VDD2_COMP Auto Gating Enable",
        "RW"
    };
    syscon0.registers[0x40002290] = {
        "COMP_DEBUG_HALTED_SEL",
        0x290,
        "VDD2_COMP Debug Halted Select",
        "RW"
    };
    syscon0.registers[0x40002300] = {
        "DSPSTALL",
        0x300,
        "HiFi4 Stall",
        "RW"
    };
    syscon0.registers[0x40002304] = {
        "OCDHALTONRESET",
        0x304,
        "HiFi4 OCDHaltOnReset",
        "RW"
    };
    syscon0.registers[0x4000231C] = {
        "DSP_VECT_REMAP",
        0x31C,
        "HiFi4 DSP Vector Remap",
        "RW"
    };
    syscon0.registers[0x40002420] = {
        "EDMA0_EN0",
        0x420,
        "eDMA0 Request Enable 0",
        "RW"
    };
    syscon0.registers[0x40002424] = {
        "EDMA0_EN1",
        0x424,
        "eDMA0 Request Enable 1",
        "RW"
    };
    syscon0.registers[0x40002428] = {
        "EDMA0_EN2",
        0x428,
        "eDMA0 Request Enable 2",
        "RW"
    };
    syscon0.registers[0x4000242C] = {
        "EDMA0_EN3",
        0x42C,
        "eDMA0 Request Enable 3",
        "RW"
    };
    syscon0.registers[0x40002430] = {
        "EDMA1_EN0",
        0x430,
        "eDMA1 Request Enable 0",
        "RW"
    };
    syscon0.registers[0x40002434] = {
        "EDMA1_EN1",
        0x434,
        "eDMA1 Request Enable 1",
        "RW"
    };
    syscon0.registers[0x40002438] = {
        "EDMA1_EN2",
        0x438,
        "eDMA1 Request Enable 2",
        "RW"
    };
    syscon0.registers[0x4000243C] = {
        "EDMA1_EN3",
        0x43C,
        "eDMA1 Request Enable 3",
        "RW"
    };
    syscon0.registers[0x40002600] = {
        "AXBS_CTRL",
        0x600,
        "AXBS Control",
        "RW"
    };
    syscon0.registers[0x40002628] = {
        "I3C_ASYNC_WAKEUP_CTRL",
        0x628,
        "I3C Asynchronous Wake-up Control",
        "RW"
    };
    syscon0.registers[0x40002650] = {
        "GRAY_CODE_LSB",
        0x650,
        "Gray to Binary Converter - Gray Code [31:0]",
        "RW"
    };
    syscon0.registers[0x40002654] = {
        "GRAY_CODE_MSB",
        0x654,
        "Gray to Binary Converter - Gray Code [63:32]",
        "RW"
    };
    syscon0.registers[0x40002B08] = {
        "ELS_TEMPORAL_STATE",
        0xB08,
        "ELS Temporal State",
        "RW"
    };
    syscon0.registers[0x40002B0C] = {
        "ELS_KDF_MASK",
        0xB0C,
        "Key Derivation Function Mask",
        "RW"
    };
    syscon0.registers[0x40002B68] = {
        "ELS_ASSET_PROT",
        0xB68,
        "ELS Asset Protection",
        "RW"
    };
    syscon0.registers[0x40002E30] = {
        "CLK_OVERRIDE_RAMPKC",
        0xE30,
        "PKC RAM Clock Override",
        "RW"
    };
    peripherals["SYSCON0"] = syscon0;
    
    // RSTCTL0 Peripheral - Real MIMXRT798S Definition
    PeripheralInfo rstctl0;
    rstctl0.name = "RSTCTL0";
    rstctl0.baseAddress = 0x40000000;
    rstctl0.description = "MIMXRT798S RSTCTL0 Peripheral";
    
    rstctl0.registers[0x40000010] = {
        "PRSTCTL0",
        0x010,
        "Common Domain Peripheral Reset Control 0",
        "RW"
    };
    rstctl0.registers[0x40000014] = {
        "PRSTCTL1",
        0x014,
        "Compute Domain Peripheral Reset Control 1",
        "RW"
    };
    rstctl0.registers[0x40000018] = {
        "PRSTCTL2",
        0x018,
        "Compute Domain Peripheral Reset Control 2",
        "RW"
    };
    rstctl0.registers[0x4000001C] = {
        "PRSTCTL3",
        0x01C,
        "Compute Domain Peripheral Reset Control 3",
        "RW"
    };
    rstctl0.registers[0x40000020] = {
        "PRSTCTL4",
        0x020,
        "Compute Domain Peripheral Reset Control 4",
        "RW"
    };
    rstctl0.registers[0x40000024] = {
        "PRSTCTL5",
        0x024,
        "DSP Domain Peripheral Reset Control 5",
        "RW"
    };
    peripherals["RSTCTL0"] = rstctl0;
    
    if (PeriphVerbose) {
        errs() << "✅ Initialized " << peripherals.size() << " MIMXRT798S peripherals with real definitions\n";
        for (const auto &p : peripherals) {
            errs() << "   " << p.first << ": " << p.second.registers.size()
                   << " registers at base 0x" << format("%08X", p.second.baseAddress) << "\n";
        }
    }
}

bool PeripheralAnalysisPass::runOnModule(Module &M) {
    if (PeriphVerbose) {
        errs() << "🔍 Starting MIMXRT798S Peripheral Analysis\n";
        errs() << "   Module: " << M.getName() << "\n";
    }
    
    for (Function &F : M) {
        if (F.isDeclaration()) continue;
        
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                analyzeInstruction(&I, &F);
            }
        }
    }
    
    if (PeriphVerbose) {
        errs() << "✅ Analysis complete: " << register_accesses.size() << " peripheral register accesses found\n";
    }
    
    generateAnalysisReport();
    
    if (PeriphChronological) {
        generateChronologicalReport();
    }
    
    return false; // We don't modify the module
}

bool PeripheralAnalysisPass::analyzeInstruction(Instruction *I, Function *F) {
    uint64_t address = 0;
    bool is_peripheral_access = false;
    
    // Analyze different instruction types for peripheral access
    if (auto *SI = dyn_cast<StoreInst>(I)) {
        if (auto *CE = dyn_cast<ConstantExpr>(SI->getPointerOperand())) {
            if (CE->getOpcode() == Instruction::IntToPtr) {
                if (auto *CI = dyn_cast<ConstantInt>(CE->getOperand(0))) {
                    address = CI->getZExtValue();
                    is_peripheral_access = true;
                }
            }
        }
    } else if (auto *LI = dyn_cast<LoadInst>(I)) {
        if (auto *CE = dyn_cast<ConstantExpr>(LI->getPointerOperand())) {
            if (CE->getOpcode() == Instruction::IntToPtr) {
                if (auto *CI = dyn_cast<ConstantInt>(CE->getOperand(0))) {
                    address = CI->getZExtValue();
                    is_peripheral_access = true;
                }
            }
        }
    } else if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
        // Handle struct-based peripheral access
        if (GEP->hasAllConstantIndices()) {
            Value *base = GEP->getPointerOperand();
            if (auto *CE = dyn_cast<ConstantExpr>(base)) {
                if (CE->getOpcode() == Instruction::IntToPtr) {
                    if (auto *CI = dyn_cast<ConstantInt>(CE->getOperand(0))) {
                        uint64_t base_addr = CI->getZExtValue();
                        
                        // Calculate offset from GEP indices
                        uint64_t offset = 0;
                        for (auto it = GEP->idx_begin(); it != GEP->idx_end(); ++it) {
                            if (auto *CI = dyn_cast<ConstantInt>(*it)) {
                                offset += CI->getZExtValue() * 4; // Assume 32-bit registers
                            }
                        }
                        
                        address = base_addr + offset;
                        is_peripheral_access = true;
                    }
                }
            }
        }
    }
    
    if (is_peripheral_access && address >= 0x40000000 && address <= 0x5FFFFFFF) {
        // This looks like a peripheral address
        auto [peripheral_name, register_name] = identifyPeripheralRegister(address);
        
        if (!peripheral_name.empty()) {
            RegisterAccess access;
            access.peripheral_name = peripheral_name;
            access.register_name = register_name;
            access.address = address;
            access.access_type = determineAccessType(I);
            access.function_name = F->getName().str();
            access.source_line = 0;
            access.bits_modified = analyzeBitsModified(I, address);
            access.purpose = determinePurpose(peripheral_name, register_name, access.function_name);
            access.sequence_number = global_sequence_counter++;
            access.execution_phase = determineExecutionPhase(access.function_name);
            
            // Get source location
            access.source_file = getSourceLocation(I, access.source_file, access.source_line);
            
            register_accesses.push_back(access);
            
            if (PeriphVerbose) {
                errs() << "   Found: " << peripheral_name << "." << register_name
                       << " (0x" << format("%08X", address) << ") in "
                       << access.function_name << "()\n";
            }
            
            return true;
        }
    }
    
    return false;
}

std::pair<std::string, std::string> PeripheralAnalysisPass::identifyPeripheralRegister(uint64_t address) {
    for (const auto &peripheral : peripherals) {
        const auto &info = peripheral.second;
        
        // Check if address falls within this peripheral's range
        if (address >= info.baseAddress && address < (info.baseAddress + 0x1000)) {
            // Look for exact register match
            auto reg_it = info.registers.find(address);
            if (reg_it != info.registers.end()) {
                return {peripheral.first, reg_it->second.name};
            }
            
            // If no exact match, calculate offset and create generic name
            uint64_t offset = address - info.baseAddress;
            return {peripheral.first, "REG_0x" + std::to_string(offset)};
        }
    }
    
    return {"", ""};
}

std::string PeripheralAnalysisPass::getSourceLocation(Instruction *I, std::string &file, int &line) {
    if (DILocation *Loc = I->getDebugLoc()) {
        file = Loc->getFilename().str();
        line = Loc->getLine();
        return file + ":" + std::to_string(line);
    }
    file = "unknown";
    line = 0;
    return "unknown:0";
}

std::string PeripheralAnalysisPass::determineAccessType(Instruction *I) {
    if (isa<StoreInst>(I)) {
        return "volatile_write";
    } else if (isa<LoadInst>(I)) {
        return "volatile_read";
    } else if (isa<GetElementPtrInst>(I)) {
        return "struct_access";
    }
    return "unknown";
}

std::vector<std::string> PeripheralAnalysisPass::analyzeBitsModified(Instruction *I, uint64_t address) {
    std::vector<std::string> bits;
    
    // For now, assume full register access
    // This could be enhanced to analyze actual bit operations
    if (isa<StoreInst>(I)) {
        bits.push_back("bit_0-31");
    }
    
    return bits;
}

std::string PeripheralAnalysisPass::determinePurpose(const std::string &peripheral, const std::string &reg, const std::string &func) {
    std::string purpose = "unknown";
    
    // Analyze function name for purpose
    std::string func_lower = func;
    std::transform(func_lower.begin(), func_lower.end(), func_lower.begin(), ::tolower);
    
    if (func_lower.find("init") != std::string::npos) {
        purpose = "initialization";
    } else if (func_lower.find("config") != std::string::npos) {
        purpose = "configuration";
    } else if (func_lower.find("clock") != std::string::npos) {
        purpose = "clock_control";
    } else if (func_lower.find("gpio") != std::string::npos) {
        purpose = "gpio_control";
    } else if (func_lower.find("transfer") != std::string::npos) {
        purpose = "data_transfer";
    }
    
    return purpose;
}

std::string PeripheralAnalysisPass::determineExecutionPhase(const std::string &function_name) {
    std::string func_lower = function_name;
    std::transform(func_lower.begin(), func_lower.end(), func_lower.begin(), ::tolower);
    
    if (func_lower.find("board") != std::string::npos) {
        return "board_initialization";
    } else if (func_lower.find("init") != std::string::npos) {
        return "driver_initialization";
    } else if (func_lower.find("transfer") != std::string::npos || func_lower.find("main") != std::string::npos) {
        return "runtime_operation";
    }
    
    return "driver_initialization";
}

void PeripheralAnalysisPass::generateAnalysisReport() {
    std::string filename = PeriphOutputFile.getValue().empty() ? "peripheral_analysis_rebuilt.json" : PeriphOutputFile.getValue();
    std::ofstream outFile(filename);
    
    if (!outFile.is_open()) {
        errs() << "❌ Error: Could not open output file " << filename << "\n";
        return;
    }
    
    outFile << "{\n";
    outFile << "  \"analysis_metadata\": {\n";
    outFile << "    \"analysis_type\": \"rebuilt_mimxrt798s_peripheral_analysis\",\n";
    outFile << "    \"device\": \"MIMXRT798S\",\n";
    outFile << "    \"source\": \"Real NXP Device Headers\",\n";
    outFile << "    \"total_accesses\": " << register_accesses.size() << ",\n";
    outFile << "    \"peripherals_analyzed\": " << peripherals.size() << "\n";
    outFile << "  },\n";
    
    // Group accesses by peripheral
    std::map<std::string, std::vector<RegisterAccess>> peripheral_groups;
    for (const auto &access : register_accesses) {
        peripheral_groups[access.peripheral_name].push_back(access);
    }
    
    outFile << "  \"peripheral_accesses\": [\n";
    bool first_peripheral = true;
    
    for (const auto &group : peripheral_groups) {
        if (!first_peripheral) outFile << ",\n";
        first_peripheral = false;
        
        outFile << "    {\n";
        outFile << "      \"peripheral_name\": \"" << group.first << "\",\n";
        
        // Find base address
        auto peripheral_it = peripherals.find(group.first);
        if (peripheral_it != peripherals.end()) {
            std::stringstream ss;
            ss << std::hex << std::uppercase << peripheral_it->second.baseAddress;
            outFile << "      \"base_address\": \"0x" << ss.str() << "\",\n";
        }
        
        outFile << "      \"accesses\": [\n";
        
        bool first_access = true;
        for (const auto &access : group.second) {
            if (!first_access) outFile << ",\n";
            first_access = false;
            
            outFile << "        {\n";
            outFile << "          \"register_name\": \"" << access.register_name << "\",\n";
            std::stringstream addr_ss;
            addr_ss << std::hex << std::uppercase << access.address;
            outFile << "          \"address\": \"0x" << addr_ss.str() << "\",\n";
            outFile << "          \"access_type\": \"" << access.access_type << "\",\n";
            outFile << "          \"source_location\": {\n";
            outFile << "            \"function\": \"" << access.function_name << "\",\n";
            outFile << "            \"file\": \"" << access.source_file << "\",\n";
            outFile << "            \"line\": " << access.source_line << "\n";
            outFile << "          },\n";
            outFile << "          \"purpose\": \"" << access.purpose << "\",\n";
            outFile << "          \"execution_phase\": \"" << access.execution_phase << "\",\n";
            outFile << "          \"sequence_number\": " << access.sequence_number << ",\n";
            outFile << "          \"bits_modified\": [";
            for (size_t i = 0; i < access.bits_modified.size(); ++i) {
                if (i > 0) outFile << ", ";
                outFile << "\"" << access.bits_modified[i] << "\"";
            }
            outFile << "]\n";
            outFile << "        }";
        }
        
        outFile << "\n      ]\n";
        outFile << "    }";
    }
    
    outFile << "\n  ]\n";
    outFile << "}\n";
    
    outFile.close();
    
    if (PeriphVerbose) {
        errs() << "✅ Analysis report saved to: " << filename << "\n";
        errs() << "   Total peripheral accesses: " << register_accesses.size() << "\n";
        errs() << "   Peripherals analyzed: " << peripherals.size() << "\n";
    }
}

void PeripheralAnalysisPass::generateChronologicalReport() {
    std::string filename = "chronological_analysis_rebuilt.json";
    std::ofstream outFile(filename);
    
    if (!outFile.is_open()) {
        errs() << "❌ Error: Could not open chronological output file\n";
        return;
    }
    
    // Sort accesses by sequence number
    std::sort(register_accesses.begin(), register_accesses.end(),
              [](const RegisterAccess &a, const RegisterAccess &b) {
                  return a.sequence_number < b.sequence_number;
              });
    
    outFile << "{\n";
    outFile << "  \"analysis_metadata\": {\n";
    outFile << "    \"analysis_type\": \"rebuilt_chronological_mimxrt798s_analysis\",\n";
    outFile << "    \"total_accesses\": " << register_accesses.size() << ",\n";
    outFile << "    \"chronological_order\": true\n";
    outFile << "  },\n";
    outFile << "  \"chronological_sequence\": [\n";
    
    for (size_t i = 0; i < register_accesses.size(); ++i) {
        const auto &access = register_accesses[i];
        
        if (i > 0) outFile << ",\n";
        
        outFile << "    {\n";
        outFile << "      \"sequence_number\": " << access.sequence_number << ",\n";
        outFile << "      \"peripheral_name\": \"" << access.peripheral_name << "\",\n";
        outFile << "      \"register_name\": \"" << access.register_name << "\",\n";
        std::stringstream chrono_ss;
        chrono_ss << std::hex << std::uppercase << access.address;
        outFile << "      \"address\": \"0x" << chrono_ss.str() << "\",\n";
        outFile << "      \"access_type\": \"" << access.access_type << "\",\n";
        outFile << "      \"execution_phase\": \"" << access.execution_phase << "\",\n";
        outFile << "      \"function_name\": \"" << access.function_name << "\"\n";
        outFile << "    }";
    }
    
    outFile << "\n  ]\n";
    outFile << "}\n";
    
    outFile.close();
    
    if (PeriphVerbose) {
        errs() << "✅ Chronological report saved to: " << filename << "\n";
    }
}

} // end anonymous namespace

// Register the pass
static RegisterPass<PeripheralAnalysisPass> X("peripheral-analysis", 
                                              "MIMXRT798S Peripheral Analysis Pass (Rebuilt)", 
                                              false, false);

// Register for automatic loading
static void registerPeripheralAnalysisPass(const PassManagerBuilder &,
                                         legacy::PassManagerBase &PM) {
    PM.add(new PeripheralAnalysisPass());
}

// Note: RegisterStandardPasses may not be available in all LLVM versions
// This registration method works with LLVM legacy pass manager
