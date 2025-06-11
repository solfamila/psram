/**
 * Comprehensive Test Suite for MIMXRT700 Peripheral Analysis Pass
 * 
 * This test suite validates:
 * 1. Function call detection and analysis
 * 2. Value extraction accuracy
 * 3. Execution order tracking
 * 4. Register address mapping
 * 5. Missing function coverage
 * 
 * Critical Tests:
 * - XCACHE_DisableCache detection (currently missing!)
 * - ARM_MPU_Enable value extraction (should be 0x00000007)
 * - Chronological ordering accuracy
 */

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCAnalysisManager.h"
#include "llvm/IR/PassManager.h"

#include "../include/PeripheralAnalysisPass.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>

class PeripheralAnalysisPassTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("test_module", *context);
        builder = std::make_unique<llvm::IRBuilder<>>(*context);
        
        // Create basic types
        int32Type = llvm::Type::getInt32Ty(*context);
        voidType = llvm::Type::getVoidTy(*context);
        int8PtrType = llvm::Type::getInt8PtrTy(*context);
        
        // Initialize the analysis pass
        analysisPass = std::make_unique<PeripheralAnalysisPass>();
    }

    void TearDown() override {
        analysisPass.reset();
        builder.reset();
        module.reset();
        context.reset();
    }

    // Helper function to create a test function
    llvm::Function* createTestFunction(const std::string& name) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(voidType, false);
        llvm::Function* func = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, name, module.get());
        
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", func);
        builder->SetInsertPoint(entry);
        
        return func;
    }

    // Helper function to create a function call instruction
    llvm::CallInst* createFunctionCall(const std::string& funcName, 
                                      const std::vector<llvm::Value*>& args = {}) {
        llvm::FunctionType* funcType;
        
        if (args.empty()) {
            funcType = llvm::FunctionType::get(voidType, false);
        } else {
            std::vector<llvm::Type*> argTypes;
            for (auto* arg : args) {
                argTypes.push_back(arg->getType());
            }
            funcType = llvm::FunctionType::get(voidType, argTypes, false);
        }
        
        llvm::Function* callee = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, funcName, module.get());
        
        return builder->CreateCall(callee, args);
    }

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<PeripheralAnalysisPass> analysisPass;
    
    llvm::Type* int32Type;
    llvm::Type* voidType;
    llvm::Type* int8PtrType;
};

// Test 1: Function Detection Coverage
TEST_F(PeripheralAnalysisPassTest, TestFunctionDetectionCoverage) {
    // Create test function
    llvm::Function* testFunc = createTestFunction("BOARD_ConfigMPU");
    
    // Test critical functions that should be detected
    std::vector<std::string> criticalFunctions = {
        "XCACHE_DisableCache",  // Currently MISSING!
        "XCACHE_EnableCache",
        "ARM_MPU_Enable",
        "ARM_MPU_Disable",
        "ARM_MPU_SetRegion",
        "CLOCK_AttachClk",
        "CLOCK_SetClkDiv",
        "IOPCTL_PinMuxSet",
        "RESET_ClearPeripheralReset"
    };
    
    // Create call instructions for each function
    std::vector<llvm::CallInst*> calls;
    for (const auto& funcName : criticalFunctions) {
        if (funcName == "ARM_MPU_Enable") {
            // ARM_MPU_Enable takes a parameter
            llvm::Value* param = llvm::ConstantInt::get(int32Type, 0x6); // PRIVDEFENA | HFNMIENA
            calls.push_back(createFunctionCall(funcName, {param}));
        } else if (funcName == "XCACHE_DisableCache" || funcName == "XCACHE_EnableCache") {
            // XCACHE functions take a cache instance parameter
            llvm::Value* param = llvm::ConstantInt::get(int32Type, 0); // XCACHE0
            calls.push_back(createFunctionCall(funcName, {param}));
        } else {
            calls.push_back(createFunctionCall(funcName));
        }
    }
    
    builder->CreateRetVoid();
    
    // Run the analysis pass
    llvm::ModuleAnalysisManager MAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::LoopAnalysisManager LAM;
    llvm::CGSCCAnalysisManager CGAM;
    
    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    auto result = analysisPass->run(*module, MAM);
    
    // Verify that all critical functions were detected
    const auto& registerAccesses = analysisPass->getRegisterAccesses();
    
    std::set<std::string> detectedFunctions;
    for (const auto& access : registerAccesses) {
        // Extract function name from purpose or other fields
        if (access.purpose.find("Cache") != std::string::npos) {
            if (access.purpose.find("enable") != std::string::npos) {
                detectedFunctions.insert("XCACHE_EnableCache");
            } else if (access.purpose.find("disable") != std::string::npos) {
                detectedFunctions.insert("XCACHE_DisableCache");
            }
        } else if (access.purpose.find("MPU enable") != std::string::npos) {
            detectedFunctions.insert("ARM_MPU_Enable");
        }
        // Add more detection logic for other functions
    }
    
    // Critical test: XCACHE_DisableCache must be detected
    EXPECT_TRUE(detectedFunctions.count("XCACHE_DisableCache") > 0) 
        << "CRITICAL FAILURE: XCACHE_DisableCache not detected by analysis pass!";
    
    EXPECT_TRUE(detectedFunctions.count("XCACHE_EnableCache") > 0)
        << "XCACHE_EnableCache not detected";
    
    EXPECT_TRUE(detectedFunctions.count("ARM_MPU_Enable") > 0)
        << "ARM_MPU_Enable not detected";
    
    // Print detection summary
    std::cout << "Function Detection Summary:" << std::endl;
    std::cout << "Total register accesses found: " << registerAccesses.size() << std::endl;
    std::cout << "Detected functions: ";
    for (const auto& func : detectedFunctions) {
        std::cout << func << " ";
    }
    std::cout << std::endl;
}

// Test 2: ARM_MPU_Enable Value Extraction
TEST_F(PeripheralAnalysisPassTest, TestARMMPUEnableValueExtraction) {
    llvm::Function* testFunc = createTestFunction("test_mpu_enable");
    
    // Create ARM_MPU_Enable call with specific parameter value
    // MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk = 0x4 | 0x2 = 0x6
    llvm::Value* inputMask = llvm::ConstantInt::get(int32Type, 0x6);
    llvm::CallInst* mpuEnableCall = createFunctionCall("ARM_MPU_Enable", {inputMask});
    
    builder->CreateRetVoid();
    
    // Run analysis
    llvm::ModuleAnalysisManager MAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::LoopAnalysisManager LAM;
    llvm::CGSCCAnalysisManager CGAM;
    
    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    auto result = analysisPass->run(*module, MAM);
    
    // Find the MPU_CTRL register access
    const auto& registerAccesses = analysisPass->getRegisterAccesses();
    
    bool mpuCtrlFound = false;
    uint64_t capturedValue = 0;
    
    for (const auto& access : registerAccesses) {
        if (access.peripheralName == "MPU" && access.registerName == "CTRL") {
            mpuCtrlFound = true;
            if (access.hasValueWritten) {
                capturedValue = access.valueWritten;
            }
            break;
        }
    }
    
    ASSERT_TRUE(mpuCtrlFound) << "MPU_CTRL register access not found";
    
    // Critical test: The final value should be 0x6 | 0x1 = 0x7
    EXPECT_EQ(capturedValue, 0x7) 
        << "CRITICAL FAILURE: MPU_CTRL value incorrect. Expected 0x7, got 0x" 
        << std::hex << capturedValue;
    
    std::cout << "ARM_MPU_Enable Value Test:" << std::endl;
    std::cout << "Input mask: 0x6" << std::endl;
    std::cout << "Expected final value: 0x7 (input | ENABLE_bit)" << std::endl;
    std::cout << "Captured value: 0x" << std::hex << capturedValue << std::endl;
}

// Test 3: Execution Order Accuracy
TEST_F(PeripheralAnalysisPassTest, TestExecutionOrderAccuracy) {
    llvm::Function* testFunc = createTestFunction("BOARD_ConfigMPU");
    
    // Create calls in the exact order they appear in board.c
    std::vector<std::pair<std::string, uint64_t>> expectedOrder = {
        {"XCACHE_DisableCache", 1},  // Line 224 - FIRST!
        {"XCACHE_DisableCache", 2},  // Line 225 - SECOND!
        {"ARM_MPU_Disable", 3},      // Line 228
        {"ARM_MPU_SetRegion", 4},    // Multiple calls...
        {"ARM_MPU_Enable", 5},       // Line 262
        {"XCACHE_EnableCache", 6},   // Line 265
        {"XCACHE_EnableCache", 7}    // Line 266
    };
    
    // Create the function calls
    for (const auto& [funcName, expectedSeq] : expectedOrder) {
        if (funcName == "ARM_MPU_Enable") {
            llvm::Value* param = llvm::ConstantInt::get(int32Type, 0x6);
            createFunctionCall(funcName, {param});
        } else if (funcName.find("XCACHE") != std::string::npos) {
            llvm::Value* param = llvm::ConstantInt::get(int32Type, 0);
            createFunctionCall(funcName, {param});
        } else {
            createFunctionCall(funcName);
        }
    }
    
    builder->CreateRetVoid();
    
    // Run analysis
    llvm::ModuleAnalysisManager MAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::LoopAnalysisManager LAM;
    llvm::CGSCCAnalysisManager CGAM;
    
    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    auto result = analysisPass->run(*module, MAM);
    
    // Check execution order
    const auto& registerAccesses = analysisPass->getRegisterAccesses();
    
    // Sort by sequence number
    std::vector<RegisterAccess> sortedAccesses = registerAccesses;
    std::sort(sortedAccesses.begin(), sortedAccesses.end(), 
              [](const RegisterAccess& a, const RegisterAccess& b) {
                  return a.sequenceNumber < b.sequenceNumber;
              });
    
    // Verify that XCACHE_DisableCache comes first
    ASSERT_FALSE(sortedAccesses.empty()) << "No register accesses found";
    
    bool xcacheDisableFirst = false;
    if (!sortedAccesses.empty()) {
        const auto& firstAccess = sortedAccesses[0];
        if (firstAccess.purpose.find("Cache") != std::string::npos && 
            firstAccess.purpose.find("disable") != std::string::npos) {
            xcacheDisableFirst = true;
        }
    }
    
    EXPECT_TRUE(xcacheDisableFirst) 
        << "CRITICAL FAILURE: XCACHE_DisableCache should be the FIRST register access, not MPU_CTRL!";
    
    std::cout << "Execution Order Test:" << std::endl;
    std::cout << "First 5 register accesses:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), sortedAccesses.size()); ++i) {
        const auto& access = sortedAccesses[i];
        std::cout << "  " << (i+1) << ". " << access.peripheralName 
                  << "_" << access.registerName << " (" << access.purpose << ")" << std::endl;
    }
}

// Test 4: Register Address Mapping
TEST_F(PeripheralAnalysisPassTest, TestRegisterAddressMapping) {
    llvm::Function* testFunc = createTestFunction("test_addresses");
    
    // Test known register addresses
    struct ExpectedMapping {
        std::string funcName;
        std::string peripheral;
        std::string registerName;
        uint64_t expectedAddress;
    };
    
    std::vector<ExpectedMapping> mappings = {
        {"ARM_MPU_Enable", "MPU", "CTRL", 0xE000ED94},
        {"XCACHE_EnableCache", "XCACHE0", "CCR", 0x40180000},
        {"XCACHE_DisableCache", "XCACHE0", "CCR", 0x40180000}
    };
    
    // Create function calls
    for (const auto& mapping : mappings) {
        if (mapping.funcName == "ARM_MPU_Enable") {
            llvm::Value* param = llvm::ConstantInt::get(int32Type, 0x6);
            createFunctionCall(mapping.funcName, {param});
        } else {
            llvm::Value* param = llvm::ConstantInt::get(int32Type, 0);
            createFunctionCall(mapping.funcName, {param});
        }
    }
    
    builder->CreateRetVoid();
    
    // Run analysis
    llvm::ModuleAnalysisManager MAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::LoopAnalysisManager LAM;
    llvm::CGSCCAnalysisManager CGAM;
    
    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    auto result = analysisPass->run(*module, MAM);
    
    // Verify address mappings
    const auto& registerAccesses = analysisPass->getRegisterAccesses();
    
    for (const auto& expectedMapping : mappings) {
        bool found = false;
        for (const auto& access : registerAccesses) {
            if (access.peripheralName == expectedMapping.peripheral &&
                access.registerName == expectedMapping.registerName) {
                found = true;
                EXPECT_EQ(access.address, expectedMapping.expectedAddress)
                    << "Address mismatch for " << expectedMapping.peripheral 
                    << "_" << expectedMapping.registerName
                    << ": expected 0x" << std::hex << expectedMapping.expectedAddress
                    << ", got 0x" << std::hex << access.address;
                break;
            }
        }
        
        EXPECT_TRUE(found) 
            << "Register access not found: " << expectedMapping.peripheral 
            << "_" << expectedMapping.registerName;
    }
}

// Main test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "🧪 MIMXRT700 Peripheral Analysis Pass Test Suite" << std::endl;
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << "Testing critical issues:" << std::endl;
    std::cout << "1. XCACHE_DisableCache detection (currently missing!)" << std::endl;
    std::cout << "2. ARM_MPU_Enable value extraction (should be 0x00000007)" << std::endl;
    std::cout << "3. Execution order accuracy (XCACHE should be first, not MPU)" << std::endl;
    std::cout << "4. Register address mapping correctness" << std::endl;
    std::cout << "=" << std::string(60, '=') << std::endl;
    
    return RUN_ALL_TESTS();
}
