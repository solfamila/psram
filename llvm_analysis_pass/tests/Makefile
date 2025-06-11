# Makefile for MIMXRT700 Peripheral Analysis Pass Tests

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0

# Simple function coverage test (no LLVM dependencies)
test_function_coverage: test_function_coverage.cpp
	$(CXX) $(CXXFLAGS) -o test_function_coverage test_function_coverage.cpp

# Comprehensive register access test (no LLVM dependencies)
comprehensive_register_test: comprehensive_register_access_test.cpp
	$(CXX) $(CXXFLAGS) -o comprehensive_register_test comprehensive_register_access_test.cpp

# Run the simple test
run_simple_test: test_function_coverage
	@echo "🧪 Running Simple Function Coverage Test"
	@echo "=" $$(printf '=%.0s' {1..50})
	./test_function_coverage

# Run comprehensive register access test
run_comprehensive_test: comprehensive_register_test
	@echo "🧪 Running Comprehensive Register Access Test"
	@echo "=" $$(printf '=%.0s' {1..60})
	./comprehensive_register_test

# Clean build artifacts
clean:
	rm -f test_function_coverage comprehensive_register_test
	rm -rf build/

# Full test with LLVM (requires LLVM and GTest)
full_test:
	@echo "🧪 Running Full LLVM Test Suite"
	@echo "=" $$(printf '=%.0s' {1..50})
	@if [ -f "run_tests.sh" ]; then \
		./run_tests.sh; \
	else \
		echo "❌ run_tests.sh not found"; \
		exit 1; \
	fi

# Default target
all: test_function_coverage comprehensive_register_test

# Run all tests
run_all_tests: run_simple_test run_comprehensive_test

# Help target
help:
	@echo "MIMXRT700 Peripheral Analysis Pass Test Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  test_function_coverage  - Build simple function coverage test"
	@echo "  run_simple_test        - Build and run simple test (no LLVM deps)"
	@echo "  full_test              - Run full LLVM test suite (requires LLVM)"
	@echo "  clean                  - Clean build artifacts"
	@echo "  help                   - Show this help"
	@echo ""
	@echo "Quick Start:"
	@echo "  make run_simple_test   - Run basic function coverage validation"

.PHONY: run_simple_test full_test clean help all
