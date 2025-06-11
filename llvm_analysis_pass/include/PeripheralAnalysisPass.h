//===-- PeripheralAnalysisPass.h - MIMXRT700 Peripheral Analysis Pass ---===//
//
// MIMXRT700 XSPI PSRAM Peripheral Register Access Analysis Pass
//
// This pass analyzes LLVM IR to identify and document all peripheral register
// accesses in the MIMXRT700 XSPI PSRAM project, including:
// - Memory-mapped I/O (MMIO) register accesses
// - Volatile memory accesses in peripheral address space
// - Register access patterns through SDK macros and functions
// - Bitfield operations on peripheral registers
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_PERIPHERAL_ANALYSIS_PASS_H
#define LLVM_ANALYSIS_PERIPHERAL_ANALYSIS_PASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/JSON.h"
#include <map>
#include <vector>
#include <string>
#include <set>

namespace llvm {

/// Structure to represent a peripheral register access with execution order
struct RegisterAccess {
    std::string peripheralName;
    std::string registerName;
    uint64_t address;
    std::string accessType;  // "read", "write", "read-modify-write"
    unsigned dataSize;       // 8, 16, 32 bits
    std::vector<std::string> bitsModified;
    std::string fileName;
    std::string functionName;
    unsigned lineNumber;
    std::string purpose;

    // Value tracking fields
    uint64_t valueWritten;       // The actual value written to the register (0 if not applicable)
    uint64_t valueRead;          // The value read from the register (0 if not applicable)
    bool hasValueWritten;        // True if valueWritten is valid
    bool hasValueRead;           // True if valueRead is valid
    std::string valueWrittenStr; // String representation of written value
    std::string valueReadStr;    // String representation of read value

    // Enhanced fields for chronological execution order
    uint64_t sequenceNumber;     // Global sequence number for execution order
    std::string executionPhase;  // "board_init", "driver_init", "runtime"
    std::string callStack;       // Function call stack context
    std::string basicBlockId;    // Basic block identifier for ordering within function
    uint64_t instructionIndex;   // Index within basic block for precise ordering
    std::string executionContext; // Additional context (e.g., "startup", "configuration", "operation")

    // Constructor to initialize value tracking fields
    RegisterAccess() : valueWritten(0), valueRead(0), hasValueWritten(false), hasValueRead(false) {}
};

/// Structure to represent a peripheral definition
struct PeripheralInfo {
    std::string name;
    uint64_t baseAddress;
    std::map<uint64_t, std::string> registers;
    std::map<uint32_t, std::string> structMemberToRegister; // Map struct member index to register name
    std::set<uint64_t> accessedAddresses;
};

/// MIMXRT700 Peripheral Analysis Pass
class PeripheralAnalysisPass : public PassInfoMixin<PeripheralAnalysisPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    
    /// Get the analysis results
    const std::vector<RegisterAccess>& getRegisterAccesses() const {
        return registerAccesses;
    }

    /// Clear all analysis results (for fresh analysis)
    void clearResults() {
        registerAccesses.clear();
        peripherals.clear();
        visitedFunctions.clear();
        globalSequenceCounter = 0;
    }

    /// Export results to JSON format
    void exportToJSON(const std::string& filename) const;

    /// Export results in chronological execution order
    void exportChronologicalJSON(const std::string& filename) const;

    /// Get register accesses sorted by execution order
    std::vector<RegisterAccess> getChronologicalAccesses() const;

private:
    std::vector<RegisterAccess> registerAccesses;
    std::map<std::string, PeripheralInfo> peripherals;

    // Enhanced fields for execution order tracking
    uint64_t globalSequenceCounter;
    std::map<std::string, std::string> functionToPhaseMap; // Map function names to execution phases
    std::set<Function*> visitedFunctions; // Track visited functions for execution order analysis
    std::map<std::string, uint64_t> functionExecutionOrder; // Map function names to execution order
    
    /// Initialize peripheral definitions for MIMXRT700
    void initializePeripheralDefinitions();
    
    /// Analyze a function for register accesses
    void analyzeFunction(Function &F);

    /// Analyze functions in execution order starting from a root function
    void analyzeFunctionInExecutionOrder(Function *F);
    
    /// Analyze a load instruction for register reads
    void analyzeLoadInstruction(LoadInst *LI);
    
    /// Analyze a store instruction for register writes
    void analyzeStoreInstruction(StoreInst *SI);
    
    /// Analyze atomic read-modify-write operations
    void analyzeAtomicRMWInstruction(AtomicRMWInst *RMWI);
    
    /// Analyze compare-and-swap operations
    void analyzeCmpXchgInstruction(AtomicCmpXchgInst *CXI);

    /// Analyze function calls for peripheral operations
    void analyzeFunctionCall(CallInst *CI);

    /// Analyze IOPCTL_PinMuxSet function calls
    void analyzeIOPCTLPinMuxSet(CallInst *CI);

    /// Analyze RESET_ClearPeripheralReset function calls
    void analyzeRESETClearPeripheralReset(CallInst *CI);

    /// Analyze CLOCK_AttachClk function calls
    void analyzeCLOCKAttachClk(CallInst *CI);

    /// Analyze CLOCK_SetClkDiv function calls
    void analyzeCLOCKSetClkDiv(CallInst *CI);

    /// Analyze ARM_MPU_SetRegion function calls
    void analyzeARMMPUSetRegion(CallInst *CI);

    /// Analyze ARM_MPU_Enable function calls
    void analyzeARMMPUEnable(CallInst *CI);

    /// Analyze ARM_MPU_Disable function calls
    void analyzeARMMPUDisable(CallInst *CI);

    /// Analyze XCACHE_EnableCache function calls
    void analyzeXCACHEEnableCache(CallInst *CI);

    /// Analyze XCACHE_DisableCache function calls
    void analyzeXCACHEDisableCache(CallInst *CI);

    /// Analyze ARM_MPU_SetMemAttr function calls
    void analyzeARMMPUSetMemAttr(CallInst *CI);

    /// Analyze GPIO_PinWrite function calls
    void analyzeGPIOPinWrite(CallInst *CI);

    /// Analyze GPIO_PinRead function calls
    void analyzeGPIOPinRead(CallInst *CI);

    /// Analyze GPIO_PinInit function calls
    void analyzeGPIOPinInit(CallInst *CI);

    /// Calculate IOPCTL register address from port/pin
    uint64_t calculateIOPCTLRegisterAddress(uint32_t port, uint32_t pin);

    /// Get IOPCTL peripheral name from port number
    std::string getIOPCTLPeripheralName(uint32_t port);

    /// Get the effective address from a pointer value
    uint64_t getEffectiveAddress(Value *Ptr);

    /// Get effective address from GEP instruction or constant expression
    uint64_t getEffectiveAddressFromGEP(Value *GEP);

    /// Calculate offset from GEP instruction
    uint64_t calculateGEPOffset(uint64_t baseAddr, GetElementPtrInst *GEP);

    /// Calculate offset from GEP constant expression
    uint64_t calculateGEPOffsetFromConstantExpr(uint64_t baseAddr, ConstantExpr *GEPCE);

    /// Trace peripheral base address from function argument
    uint64_t tracePeripheralBaseFromArgument(Argument *Arg);

    /// Infer peripheral address from instruction context (function name, etc.)
    uint64_t inferPeripheralAddressFromContext(Instruction *I);

    /// Determine which peripheral and register an address belongs to
    std::pair<std::string, std::string> identifyPeripheralRegister(uint64_t address);

    /// Identify peripheral register from struct member index
    std::pair<std::string, std::string> identifyPeripheralRegisterFromStructMember(uint64_t baseAddress, uint32_t memberIndex);

    /// Extract struct member index from GEP instruction
    uint32_t extractStructMemberIndex(GetElementPtrInst *GEP);

    /// Extract struct member index from GEP constant expression
    uint32_t extractStructMemberIndexFromConstantExpr(ConstantExpr *GEPCE);
    
    /// Analyze bitfield operations in store instructions
    std::vector<std::string> analyzeBitfieldOperations(StoreInst *SI);
    
    /// Get debug information for an instruction
    std::tuple<std::string, std::string, unsigned> getDebugInfo(Instruction *I);
    
    /// Determine the purpose of a register access based on context
    std::string determinePurpose(Instruction *I, const std::string& peripheralName, 
                                const std::string& registerName);
    
    /// Check if an address is in peripheral space
    bool isPeripheralAddress(uint64_t address);
    
    /// Get data size from type
    unsigned getDataSizeFromType(Type *Ty);

    /// Enhanced methods for execution order tracking

    /// Initialize execution phase mapping based on function names
    void initializeExecutionPhaseMapping();

    /// Determine execution phase from function name and context
    std::string determineExecutionPhase(const std::string& functionName, const std::string& fileName);

    /// Build call stack context for an instruction
    std::string buildCallStackContext(Instruction *I);

    /// Generate basic block identifier for ordering
    std::string generateBasicBlockId(BasicBlock *BB);

    /// Get instruction index within basic block
    uint64_t getInstructionIndex(Instruction *I);

    /// Assign sequence number and execution context to register access
    void assignExecutionOrder(RegisterAccess &access, Instruction *I);

    /// Get function-based execution order for proper chronological sequencing
    uint64_t getFunctionExecutionOrder(const std::string& functionName);

    /// Initialize function execution order mapping
    void initializeFunctionExecutionOrder();
};

/// Legacy pass wrapper for compatibility
class PeripheralAnalysisLegacyPass : public ModulePass {
public:
    static char ID;
    
    PeripheralAnalysisLegacyPass() : ModulePass(ID) {}
    
    bool runOnModule(Module &M) override;
    
    void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.setPreservesAll();
    }
    
    /// Get the analysis results
    const std::vector<RegisterAccess>& getRegisterAccesses() const {
        return Pass.getRegisterAccesses();
    }
    
    /// Export results to JSON format
    void exportToJSON(const std::string& filename) const {
        Pass.exportToJSON(filename);
    }

private:
    PeripheralAnalysisPass Pass;
};

} // namespace llvm

#endif // LLVM_ANALYSIS_PERIPHERAL_ANALYSIS_PASS_H
