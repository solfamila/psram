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
#include <fstream>
#include <regex>
#include <cstdio>

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional,
                                        cl::desc("<input LLVM IR file>"),
                                        cl::Required);

static cl::list<std::string> AdditionalInputs("input",
                                             cl::desc("Additional LLVM IR files for multi-module analysis"),
                                             cl::value_desc("filename"));

static cl::opt<std::string> InputDirectory("input-dir",
                                         cl::desc("Directory containing LLVM IR files for multi-module analysis"),
                                         cl::value_desc("directory"));

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

    // Collect all input files
    std::vector<std::string> inputFiles;
    inputFiles.push_back(InputFilename);

    // Add additional input files
    for (const auto& file : AdditionalInputs) {
        inputFiles.push_back(file);
    }

    // Add files from input directory
    if (!InputDirectory.empty()) {
        std::string findCommand = "find " + InputDirectory + " -name '*.ll' -type f";
        FILE* pipe = popen(findCommand.c_str(), "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string file(buffer);
                file.erase(file.find_last_not_of(" \n\r\t") + 1); // trim
                inputFiles.push_back(file);
            }
            pclose(pipe);
        }
    }

    if (Verbose) {
        outs() << "Multi-module analysis with " << inputFiles.size() << " files:\n";
        for (const auto& file : inputFiles) {
            outs() << "  " << file << "\n";
        }
        outs() << "\n";
    }

    // Load all modules
    std::vector<std::unique_ptr<Module>> modules;
    for (const auto& inputFile : inputFiles) {
        std::unique_ptr<Module> M = parseIRFile(inputFile, Err, Context);

        // If parsing fails due to debug info, try to strip it and retry
        if (!M) {
            if (Verbose) {
                errs() << "Warning: Failed to parse " << inputFile << ", attempting to strip debug info...\n";
            }

            // Try to read the file and strip debug declarations
            std::ifstream file(inputFile);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                file.close();

                // Remove debug declarations that cause parsing issues
                std::regex debug_regex(R"(\s*#dbg_[^,\n]*(?:,[^,\n]*)*(?:,[^)]*\))?\s*\n?)");
                content = std::regex_replace(content, debug_regex, "\n");

                // Write to temporary file and try parsing again
                std::string tempFile = inputFile + ".temp";
                std::ofstream temp(tempFile);
                if (temp.is_open()) {
                    temp << content;
                    temp.close();

                    M = parseIRFile(tempFile, Err, Context);

                    // Clean up temp file
                    std::remove(tempFile.c_str());
                }
            }
        }

        if (M) {
            if (Verbose) {
                outs() << "Loaded: " << inputFile << " (" << M->size() << " functions)\n";
            }
            modules.push_back(std::move(M));
        } else {
            errs() << "Warning: Skipping " << inputFile << " (failed to parse)\n";
        }
    }

    if (modules.empty()) {
        errs() << "Error: No modules could be loaded\n";
        return 1;
    }
    
    // Create and run the multi-module analysis pass
    PeripheralAnalysisPass Pass;
    ModuleAnalysisManager MAM;

    if (Verbose) {
        outs() << "Running multi-module peripheral analysis pass...\n";
    }

    // Analyze all modules
    for (auto& module : modules) {
        Pass.run(*module, MAM);
    }
    
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
