#!/bin/bash
# Unified Build Script for MIMXRT700 XSPI PSRAM Example
# Supports both local development and remote agent execution

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc"
TEST_DIR="$PROJECT_ROOT/tests"

# Default values
BUILD_TYPE="debug"
RUN_TESTS="false"
CLEAN_BUILD="false"
VERBOSE="false"
CONTAINER_BUILD="false"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
MIMXRT700 XSPI PSRAM Example Build Script

Usage: $0 [OPTIONS]

OPTIONS:
    -t, --type TYPE         Build type: debug, release, flash_debug, flash_release (default: debug)
    -c, --clean             Clean build artifacts before building
    -T, --test              Run unit tests after building
    -v, --verbose           Enable verbose output
    -C, --container         Build in container (simulates remote agent)
    -h, --help              Show this help message

EXAMPLES:
    $0                      # Build debug version
    $0 -t release           # Build release version
    $0 -c -t debug          # Clean and build debug version
    $0 -T                   # Build and run tests
    $0 -C                   # Build in container

ENVIRONMENT VARIABLES:
    ARMGCC_DIR              Path to ARM GCC toolchain (default: /opt/gcc-arm-none-eabi)
    SDK_ROOT                Path to SDK root directory (auto-detected)
    UNITY_ROOT              Path to Unity testing framework (default: /opt/unity)

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -T|--test)
            RUN_TESTS="true"
            shift
            ;;
        -v|--verbose)
            VERBOSE="true"
            shift
            ;;
        -C|--container)
            CONTAINER_BUILD="true"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Validate build type
case $BUILD_TYPE in
    debug|release|flash_debug|flash_release)
        ;;
    *)
        log_error "Invalid build type: $BUILD_TYPE"
        log_info "Valid types: debug, release, flash_debug, flash_release"
        exit 1
        ;;
esac

# SDK validation function
validate_sdk() {
    log_info "Validating NXP MIMXRT700 SDK..."

    # Check if SDK_ROOT is set and exists
    if [[ -z "$SDK_ROOT" ]]; then
        log_error "SDK_ROOT environment variable not set"
        return 1
    fi

    if [[ ! -d "$SDK_ROOT" ]]; then
        log_error "SDK directory not found: $SDK_ROOT"
        return 1
    fi

    # Check for essential SDK components
    local required_dirs=(
        "CMSIS"
        "devices"
        "boards"
        "components"
    )

    for dir in "${required_dirs[@]}"; do
        if [[ ! -d "$SDK_ROOT/$dir" ]]; then
            log_error "Required SDK directory missing: $SDK_ROOT/$dir"
            return 1
        fi
    done

    # Check for SDK manifest and version
    local manifest_file="$SDK_ROOT/MIMXRT700-EVK_manifest_v3_15.xml"
    if [[ -f "$manifest_file" ]]; then
        # Look for the ksdk version attribute which contains the actual SDK version
        local sdk_version=$(grep -o '<ksdk[^>]*version="[^"]*"' "$manifest_file" | grep -o 'version="[^"]*"' | cut -d'"' -f2)
        if [[ -n "$sdk_version" ]]; then
            log_info "Found NXP SDK version: $sdk_version"

            # Validate minimum required version (25.03.00)
            if [[ "$sdk_version" < "25.03.00" ]]; then
                log_warning "SDK version $sdk_version may be incompatible. Recommended: 25.03.00 or later"
            fi
        else
            log_warning "Could not parse SDK version from manifest file."
        fi
    else
        log_warning "SDK manifest file not found. Cannot verify SDK version."
    fi

    # Check for license file
    if [[ -f "$SDK_ROOT/COPYING-BSD-3" ]]; then
        log_info "SDK license: BSD-3-Clause (redistribution allowed)"
    fi

    log_success "SDK validation completed successfully"
    return 0
}

# Environment setup
setup_environment() {
    log_info "Setting up build environment..."

    # Set default ARM GCC path if not specified
    if [[ -z "$ARMGCC_DIR" ]]; then
        if [[ -d "/opt/gcc-arm-none-eabi-10.3-2021.10" ]]; then
            export ARMGCC_DIR="/opt/gcc-arm-none-eabi-10.3-2021.10"
        elif [[ -d "/opt/gcc-arm-none-eabi" ]]; then
            export ARMGCC_DIR="/opt/gcc-arm-none-eabi"
        elif [[ -d "/usr" ]] && command -v arm-none-eabi-gcc &> /dev/null; then
            export ARMGCC_DIR="/usr"
        else
            log_error "ARM GCC toolchain not found. Please set ARMGCC_DIR environment variable."
            log_info "Install ARM GCC toolchain from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm"
            exit 1
        fi
    fi

    # Set SDK root with auto-detection
    if [[ -z "$SDK_ROOT" ]]; then
        # Try to auto-detect SDK location
        local possible_sdk_paths=(
            "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
            "$PROJECT_ROOT/sdk"
            "$PROJECT_ROOT/../sdk"
            "/opt/nxp/sdk"
        )

        for sdk_path in "${possible_sdk_paths[@]}"; do
            if [[ -d "$sdk_path" && -d "$sdk_path/CMSIS" ]]; then
                export SDK_ROOT="$sdk_path"
                log_info "Auto-detected SDK at: $SDK_ROOT"
                break
            fi
        done

        if [[ -z "$SDK_ROOT" ]]; then
            log_error "NXP SDK not found. Please set SDK_ROOT environment variable."
            log_info "Expected SDK structure with CMSIS, devices, boards, and components directories."
            exit 1
        fi
    fi

    # Validate SDK
    if ! validate_sdk; then
        log_error "SDK validation failed. Please check your SDK installation."
        exit 1
    fi

    # Add ARM GCC to PATH
    export PATH="$ARMGCC_DIR/bin:$PATH"

    # Verify toolchain
    if ! command -v arm-none-eabi-gcc &> /dev/null; then
        log_error "arm-none-eabi-gcc not found in PATH"
        log_info "ARMGCC_DIR: $ARMGCC_DIR"
        log_info "PATH: $PATH"
        exit 1
    fi

    # Verify toolchain version compatibility
    local gcc_version=$(arm-none-eabi-gcc --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -n1)
    log_info "ARM GCC Version: $gcc_version"

    # Check for minimum required version (10.3.0)
    if [[ "$gcc_version" < "10.3.0" ]]; then
        log_warning "ARM GCC version $gcc_version may be incompatible. Recommended: 10.3.0 or later"
    fi

    log_success "Environment setup complete"
    if [[ "$VERBOSE" == "true" ]]; then
        log_info "ARMGCC_DIR: $ARMGCC_DIR"
        log_info "SDK_ROOT: $SDK_ROOT"
        log_info "ARM GCC Version: $gcc_version"
        log_info "SDK Components:"
        ls -la "$SDK_ROOT" | head -10
    fi
}

# Container build function
build_in_container() {
    log_info "Building in container (simulating remote agent)..."
    
    if ! command -v docker &> /dev/null; then
        log_error "Docker not found. Please install Docker to use container builds."
        exit 1
    fi
    
    # Build container image
    log_info "Building container image..."
    docker build -f .augment/Dockerfile -t mimxrt700-dev . || {
        log_error "Failed to build container image"
        exit 1
    }
    
    # Run build in container
    log_info "Running build in container..."
    docker run --rm -v "$(pwd):/workspace" mimxrt700-dev \
        /workspace/build.sh -t "$BUILD_TYPE" $([ "$CLEAN_BUILD" == "true" ] && echo "-c") \
        $([ "$RUN_TESTS" == "true" ] && echo "-T") $([ "$VERBOSE" == "true" ] && echo "-v") || {
        log_error "Container build failed"
        exit 1
    }
    
    log_success "Container build completed"
    exit 0
}

# Clean function
clean_build() {
    log_info "Cleaning build artifacts..."
    
    cd "$BUILD_DIR"
    if [[ -f "clean.sh" ]]; then
        ./clean.sh
    else
        # Manual cleanup
        rm -rf debug release flash_debug flash_release
        rm -f CMakeCache.txt
        rm -rf CMakeFiles
    fi
    
    # Clean test artifacts
    if [[ -d "$TEST_DIR" ]]; then
        cd "$TEST_DIR"
        rm -rf build
        rm -f test_results.txt
    fi
    
    log_success "Clean completed"
}

# Build function
build_project() {
    log_info "Building project (type: $BUILD_TYPE)..."
    
    cd "$BUILD_DIR"
    
    # Make build scripts executable
    chmod +x *.sh 2>/dev/null || true
    
    # Select build script based on type
    case $BUILD_TYPE in
        debug)
            BUILD_SCRIPT="./build_debug.sh"
            ;;
        release)
            BUILD_SCRIPT="./build_release.sh"
            ;;
        flash_debug)
            BUILD_SCRIPT="./build_flash_debug.sh"
            ;;
        flash_release)
            BUILD_SCRIPT="./build_flash_release.sh"
            ;;
    esac
    
    # Execute build
    if [[ "$VERBOSE" == "true" ]]; then
        $BUILD_SCRIPT
    else
        $BUILD_SCRIPT > /dev/null 2>&1
    fi
    
    # Verify build artifacts
    ARTIFACT_DIR="$BUILD_TYPE"
    if [[ ! -d "$ARTIFACT_DIR" ]]; then
        log_error "Build failed: artifact directory not found"
        exit 1
    fi
    
    ELF_FILE="$ARTIFACT_DIR/xspi_psram_polling_transfer_cm33_core0.elf"
    BIN_FILE="$ARTIFACT_DIR/xspi_psram_polling_transfer.bin"
    
    if [[ ! -f "$ELF_FILE" ]]; then
        log_error "Build failed: ELF file not found"
        exit 1
    fi
    
    if [[ ! -f "$BIN_FILE" ]]; then
        log_error "Build failed: BIN file not found"
        exit 1
    fi
    
    log_success "Build completed successfully"
    
    if [[ "$VERBOSE" == "true" ]]; then
        log_info "Build artifacts:"
        ls -la "$ARTIFACT_DIR"

        # Display ELF file info if 'file' command is available
        if command -v file &> /dev/null; then
            log_info "ELF file info:"
            file "$ELF_FILE"
        else
            log_info "ELF file info: (file command not available)"
            log_info "ELF file size: $(stat -c%s "$ELF_FILE" 2>/dev/null || echo "unknown") bytes"
        fi
    fi
}

# Test function
run_tests() {
    log_info "Running unit tests..."
    
    # Check if Unity is available
    if [[ -z "$UNITY_ROOT" ]]; then
        export UNITY_ROOT="/opt/unity"
    fi
    
    if [[ ! -d "$UNITY_ROOT" ]]; then
        log_warning "Unity testing framework not found at $UNITY_ROOT"
        log_info "To install Unity: git clone https://github.com/ThrowTheSwitch/Unity.git $UNITY_ROOT"
        return 0
    fi
    
    cd "$TEST_DIR"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure and build tests
    cmake .. || {
        log_error "Test configuration failed"
        return 1
    }
    
    make || {
        log_error "Test build failed"
        return 1
    }
    
    # Run tests
    if [[ -f "run_tests" ]]; then
        ./run_tests || {
            log_error "Tests failed"
            return 1
        }
        log_success "All tests passed"
    else
        log_warning "Test executable not found"
        return 1
    fi
}

# Main execution
main() {
    log_info "Starting MIMXRT700 build process..."
    
    # Handle container build
    if [[ "$CONTAINER_BUILD" == "true" ]]; then
        build_in_container
    fi
    
    # Setup environment
    setup_environment
    
    # Clean if requested
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        clean_build
    fi
    
    # Build project
    build_project
    
    # Run tests if requested
    if [[ "$RUN_TESTS" == "true" ]]; then
        run_tests
    fi
    
    log_success "Build process completed successfully!"
}

# Execute main function
main "$@"
