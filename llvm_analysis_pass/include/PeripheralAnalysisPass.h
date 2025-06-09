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

/// Structure to represent a peripheral register access
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
};

/// Structure to represent a peripheral definition
struct PeripheralInfo {
    std::string name;
    uint64_t baseAddress;
    std::map<uint64_t, std::string> registers;
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
    
    /// Export results to JSON format
    void exportToJSON(const std::string& filename) const;

private:
    std::vector<RegisterAccess> registerAccesses;
    std::map<std::string, PeripheralInfo> peripherals;
    
    /// Initialize peripheral definitions for MIMXRT700
    void initializePeripheralDefinitions();
    
    /// Analyze a function for register accesses
    void analyzeFunction(Function &F);
    
    /// Analyze a load instruction for register reads
    void analyzeLoadInstruction(LoadInst *LI);
    
    /// Analyze a store instruction for register writes
    void analyzeStoreInstruction(StoreInst *SI);
    
    /// Analyze atomic read-modify-write operations
    void analyzeAtomicRMWInstruction(AtomicRMWInst *RMWI);
    
    /// Analyze compare-and-swap operations
    void analyzeCmpXchgInstruction(AtomicCmpXchgInst *CXI);
    
    /// Get the effective address from a pointer value
    uint64_t getEffectiveAddress(Value *Ptr);
    
    /// Determine which peripheral and register an address belongs to
    std::pair<std::string, std::string> identifyPeripheralRegister(uint64_t address);
    
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
