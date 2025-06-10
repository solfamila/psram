//===-- peripheral-analyzer.cpp - Standalone Peripheral Analysis Tool --===//
//
// Standalone tool to run peripheral register access analysis on LLVM IR files
//
//===----------------------------------------------------------------------===//

#include "PeripheralAnalysisPass.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include <memory>

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional,
                                        cl::desc("<input LLVM IR file>"),
                                        cl::Required);

static cl::opt<std::string> OutputFilename("o",
                                         cl::desc("Output JSON file"),
                                         cl::value_desc("filename"),
                                         cl::init("peripheral_analysis.json"));

static cl::opt<bool> Verbose("v",
                           cl::desc("Enable verbose output"),
                           cl::init(false));

static cl::opt<bool> Chronological("chronological",
                                  cl::desc("Export results in chronological execution order"),
                                  cl::init(false));

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);

    cl::ParseCommandLineOptions(argc, argv, "MIMXRT700 Peripheral Register Access Analyzer\n");
    
    LLVMContext Context;
    SMDiagnostic Err;
    
    // Load the LLVM IR module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }
    
    if (Verbose) {
        outs() << "Loaded module: " << M->getName() << "\n";
        outs() << "Functions in module: " << M->size() << "\n";
    }
    
    // Create and run the analysis pass
    PeripheralAnalysisPass Pass;
    ModuleAnalysisManager MAM;
    
    if (Verbose) {
        outs() << "Running peripheral analysis pass...\n";
    }
    
    Pass.run(*M, MAM);
    
    const auto& accesses = Pass.getRegisterAccesses();
    
    if (Verbose) {
        outs() << "Found " << accesses.size() << " peripheral register accesses\n";

        // Print summary
        std::map<std::string, int> accessCounts;
        std::map<std::string, int> phaseCounts;

        for (const auto& access : accesses) {
            accessCounts[access.peripheralName]++;
            phaseCounts[access.executionPhase]++;
        }

        outs() << "\nAccess summary by peripheral:\n";
        for (const auto& [peripheral, count] : accessCounts) {
            outs() << "  " << peripheral << ": " << count << " accesses\n";
        }

        if (Chronological) {
            outs() << "\nAccess summary by execution phase:\n";
            for (const auto& [phase, count] : phaseCounts) {
                outs() << "  " << phase << ": " << count << " accesses\n";
            }
        }
    }

    // Export results to JSON
    if (Verbose) {
        outs() << "\nExporting results to: " << OutputFilename;
        if (Chronological) {
            outs() << " (chronological order)";
        }
        outs() << "\n";
    }

    if (Chronological) {
        Pass.exportChronologicalJSON(OutputFilename);
    } else {
        Pass.exportToJSON(OutputFilename);
    }
    
    if (Verbose) {
        outs() << "Analysis complete!\n";
    }
    
    return 0;
}
