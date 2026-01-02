#!/bin/bash
# Setup QLever development environment for Claude Code on the web
# This script installs dependencies and configures the environment for agent swarms
# Optimized for Linux/Ubuntu cloud environments (Claude Code web)

# Merge stderr into stdout to ensure all output is captured properly.
# When running as a hook via socket (not TTY), stderr becomes fully-buffered
# and can be lost. This ensures all output flows together.
exec 2>&1

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Version requirements
CMAKE_MIN_VERSION="3.27"
CLANG_MIN_VERSION="16.0"
GCC_MIN_VERSION="11.0"

echo "Setting up QLever development environment..."
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
    echo "Detected Claude Code remote environment (cloud)"
fi
echo ""

# Helper function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Helper function to check version comparison
# Usage: version_ge "3.28.0" "3.27.0" returns 0 (true) if first >= second
version_ge() {
    printf '%s\n%s\n' "$2" "$1" | sort -V -C
}

# Check command version meets requirement
# Usage: check_command_version "cmake" "3.27" "--version"
check_command_version() {
    local cmd=$1
    local min_version=$2
    local version_flag=${3:-"--version"}
    
    if ! command_exists "$cmd"; then
        return 1
    fi
    
    local version_output
    version_output=$($cmd $version_flag 2>&1 | head -n1)
    
    # Extract version number (handle different formats)
    local version
    version=$(echo "$version_output" | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
    
    if [ -z "$version" ]; then
        echo "${YELLOW}Warning: Could not parse version for $cmd${NC}"
        return 1
    fi
    
    if version_ge "$version" "$min_version"; then
        return 0
    else
        return 1
    fi
}

# Check if running in remote environment
is_remote_env() {
    [ "$CLAUDE_CODE_REMOTE" = "true" ]
}

# Install package via apt if not already installed
install_if_missing() {
    local package=$1
    local description=${2:-$package}
    
    if dpkg -l | grep -q "^ii.*$package "; then
        echo "  ✓ $description already installed"
        return 0
    fi
    
    echo "  Installing $description..."
    if ! sudo apt-get install -y "$package" >/dev/null 2>&1; then
        echo "${RED}ERROR: Failed to install $package${NC}"
        return 1
    fi
    echo "  ✓ $description installed"
    return 0
}

# Verify CMake version and install/upgrade if needed
verify_cmake_version() {
    echo "Checking CMake version..."
    
    if check_command_version "cmake" "$CMAKE_MIN_VERSION"; then
        local version
        version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
        echo "  ✓ CMake $version (>= $CMAKE_MIN_VERSION) found"
        return 0
    fi
    
    if ! command_exists "cmake"; then
        echo "  CMake not found, installing..."
        install_if_missing "cmake" "CMake"
        if ! check_command_version "cmake" "$CMAKE_MIN_VERSION"; then
            echo "${YELLOW}Warning: Installed CMake version may be < $CMAKE_MIN_VERSION${NC}"
            echo "  You may need to install CMake from source or PPA"
            echo "  See: https://cmake.org/download/"
            return 1
        fi
    else
        local version
        version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
        echo "${RED}ERROR: CMake $version found, but >= $CMAKE_MIN_VERSION required${NC}"
        echo "  Please upgrade CMake manually or install from source"
        return 1
    fi
    
    return 0
}

# Verify compiler version
verify_compiler() {
    echo "Checking compiler..."
    
    local compiler_ok=false
    local compiler_name=""
    local compiler_version=""
    
    # Check Clang
    if command_exists "clang++"; then
        if check_command_version "clang++" "$CLANG_MIN_VERSION"; then
            compiler_version=$(clang++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
            echo "  ✓ Clang++ $compiler_version (>= $CLANG_MIN_VERSION) found"
            compiler_ok=true
            compiler_name="Clang"
        fi
    fi
    
    # Check GCC if Clang not sufficient
    if [ "$compiler_ok" = false ] && command_exists "g++"; then
        if check_command_version "g++" "$GCC_MIN_VERSION"; then
            compiler_version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
            echo "  ✓ G++ $compiler_version (>= $GCC_MIN_VERSION) found"
            compiler_ok=true
            compiler_name="GCC"
        fi
    fi
    
    if [ "$compiler_ok" = false ]; then
        echo "${RED}ERROR: No suitable compiler found${NC}"
        echo "  Required: Clang >= $CLANG_MIN_VERSION or GCC >= $GCC_MIN_VERSION"
        echo "  Install with: sudo apt-get install build-essential"
        return 1
    fi
    
    return 0
}

# Setup Conan (optimized with profile caching)
setup_conan() {
    echo "Setting up Conan..."

    # Check if Conan is installed
    if command_exists "conan"; then
        local version
        version=$(conan --version 2>&1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        echo "  ✓ Conan $version found"
    else
        echo "  Installing Conan 2.x..."
        if ! pip install conan >/dev/null 2>&1; then
            echo "${RED}ERROR: Failed to install Conan${NC}"
            return 1
        fi
        echo "  ✓ Conan installed"
    fi

    # Detect/create profile (cache if already exists)
    echo "  Detecting Conan profile..."
    local conan_profile_path="${HOME}/.conan2/profiles/default"

    if [ -f "$conan_profile_path" ]; then
        echo "  ✓ Conan profile already exists (cached)"
    else
        if ! conan profile detect --force >/dev/null 2>&1; then
            echo "${YELLOW}Warning: Could not detect Conan profile automatically${NC}"
            echo "  You may need to configure it manually: conan profile detect"
        else
            echo "  ✓ Conan profile detected/created"
        fi
    fi

    # Verify Conan can work (basic check)
    if conan --version >/dev/null 2>&1; then
        echo "  ✓ Conan is functional"
        return 0
    else
        echo "${RED}ERROR: Conan installation appears broken${NC}"
        return 1
    fi
}

# Install system dependencies (optimized for cloud environment)
install_system_deps() {
    echo "Installing system dependencies..."

    # Skip apt-get update in cloud environments (packages pre-cached)
    if ! is_remote_env; then
        # Update package list (non-fatal if it fails)
        echo "  Updating package list..."
        sudo apt-get update >/dev/null 2>&1 || echo "${YELLOW}Warning: apt-get update failed (may be in restricted environment)${NC}"
    fi

    # Critical build tools - batch install for speed
    local critical_packages="build-essential cmake ninja-build pkg-config git wget libicu-dev libssl-dev libboost1.83-dev libboost-program-options1.83-dev libboost-iostreams1.83-dev libboost-url1.83-dev libboost-container1.83-dev"

    if is_remote_env; then
        # Cloud environment: batch install only missing packages
        echo "  Batch installing critical packages (cloud-optimized)..."
        local missing_packages=""
        for pkg in $critical_packages; do
            if ! dpkg -l | grep -q "^ii.*$pkg "; then
                missing_packages="$missing_packages $pkg"
            fi
        done

        if [ -n "$missing_packages" ]; then
            if ! sudo apt-get install -y $missing_packages >/dev/null 2>&1; then
                echo "${YELLOW}Warning: Some packages failed to install${NC}"
            fi
        else
            echo "  ✓ Critical packages already installed"
        fi
    else
        # Local environment: install individually with feedback
        for pkg in $critical_packages; do
            install_if_missing "$pkg"
        done
    fi

    # Optional performance packages
    echo "  Installing optional packages..."
    install_if_missing "libjemalloc-dev" "Jemalloc memory allocator (optional)"
    install_if_missing "libzstd-dev" "Zstandard compression library (optional)"
    install_if_missing "tzdata" "Timezone data (optional)"
    install_if_missing "uuid-dev" "UUID development libraries (optional)"

    # Python development
    if ! command_exists "pip" && ! command_exists "pip3"; then
        echo "  Installing python3-pip..."
        install_if_missing "python3-pip" "Python pip"
    else
        echo "  ✓ pip already available"
    fi
}

# Install Python dependencies (cloud-optimized with parallel installation)
install_python_deps() {
    echo "Installing Python dependencies..."

    # Determine pip command
    local pip_cmd="pip"
    if ! command_exists "pip" && command_exists "pip3"; then
        pip_cmd="pip3"
    fi

    if ! command_exists "$pip_cmd"; then
        echo "${RED}ERROR: pip not found${NC}"
        return 1
    fi

    # Check which packages are already installed (batch check)
    local precommit_needed=false
    local pyaml_needed=false
    local pyicu_needed=false

    $pip_cmd show pre-commit >/dev/null 2>&1 || precommit_needed=true
    $pip_cmd show pyaml >/dev/null 2>&1 || pyaml_needed=true
    $pip_cmd show pyicu >/dev/null 2>&1 || pyicu_needed=true

    # Batch install available packages in parallel
    if [ "$precommit_needed" = true ] || [ "$pyaml_needed" = true ]; then
        echo "  Installing pre-commit and pyaml (batch)..."
        local packages_to_install=""
        [ "$precommit_needed" = true ] && packages_to_install="$packages_to_install pre-commit"
        [ "$pyaml_needed" = true ] && packages_to_install="$packages_to_install pyaml"

        if $pip_cmd install $packages_to_install >/dev/null 2>&1; then
            echo "  ✓ Python packages installed"
        else
            echo "${YELLOW}Warning: Some Python packages failed to install${NC}"
        fi
    else
        echo "  ✓ pre-commit and pyaml already installed"
    fi

    # Install pyicu separately (requires compilation from source)
    if [ "$pyicu_needed" = true ]; then
        echo "  Installing pyicu..."
        # Check if libicu-dev is installed
        if ! dpkg -l | grep -q "^ii.*libicu-dev "; then
            echo "${YELLOW}Warning: libicu-dev not found, pyicu installation may fail${NC}"
        fi
        if $pip_cmd install --no-binary=:pyicu: pyicu >/dev/null 2>&1; then
            echo "  ✓ pyicu installed"
        else
            echo "${YELLOW}Warning: Failed to install pyicu (E2E tests may fail)${NC}"
        fi
    else
        echo "  ✓ pyicu already installed"
    fi

    # Setup pre-commit hooks (optional in cloud, required for commits)
    if is_remote_env && [ "$BACKGROUND_MODE" = true ]; then
        echo "  Deferring pre-commit hook setup (background installation)..."
    else
        echo "  Setting up git hooks..."
        if command_exists "pre-commit" && pre-commit install >/dev/null 2>&1; then
            echo "  ✓ Pre-commit hooks installed"
        else
            echo "${YELLOW}Warning: Failed to install pre-commit hooks${NC}"
        fi
    fi
}

# Comprehensive environment verification
verify_environment() {
    echo ""
    echo "Verifying environment..."
    
    local errors=0
    
    # Check CMake
    if ! check_command_version "cmake" "$CMAKE_MIN_VERSION"; then
        echo "${RED}✗ CMake >= $CMAKE_MIN_VERSION not found${NC}"
        errors=$((errors + 1))
    else
        echo "${GREEN}✓ CMake version OK${NC}"
    fi
    
    # Check Ninja
    if ! command_exists "ninja"; then
        echo "${RED}✗ Ninja not found${NC}"
        errors=$((errors + 1))
    else
        echo "${GREEN}✓ Ninja found${NC}"
    fi
    
    # Check compiler
    local compiler_ok=false
    if command_exists "clang++" && check_command_version "clang++" "$CLANG_MIN_VERSION"; then
        compiler_ok=true
    elif command_exists "g++" && check_command_version "g++" "$GCC_MIN_VERSION"; then
        compiler_ok=true
    fi
    
    if [ "$compiler_ok" = false ]; then
        echo "${RED}✗ Suitable compiler not found${NC}"
        errors=$((errors + 1))
    else
        echo "${GREEN}✓ Compiler version OK${NC}"
    fi
    
    # Check Conan
    if ! command_exists "conan"; then
        echo "${RED}✗ Conan not found${NC}"
        errors=$((errors + 1))
    else
        echo "${GREEN}✓ Conan found${NC}"
    fi
    
    # Check Python dependencies
    local pip_cmd="pip"
    if ! command_exists "pip" && command_exists "pip3"; then
        pip_cmd="pip3"
    fi
    
    if ! $pip_cmd show pre-commit >/dev/null 2>&1; then
        echo "${RED}✗ pre-commit not installed${NC}"
        errors=$((errors + 1))
    else
        echo "${GREEN}✓ pre-commit installed${NC}"
    fi
    
    if [ $errors -eq 0 ]; then
        echo ""
        echo "${GREEN}✓ Environment verification passed!${NC}"
        return 0
    else
        echo ""
        echo "${RED}✗ Environment verification failed with $errors error(s)${NC}"
        return 1
    fi
}

# Main execution
main() {
    # Parse command-line arguments
    BACKGROUND_MODE=false
    NO_BACKGROUND_MODE=false

    for arg in "$@"; do
        case "$arg" in
            --background)
                BACKGROUND_MODE=true
                ;;
            --no-background)
                NO_BACKGROUND_MODE=true
                ;;
        esac
    done

    # Check if we can use sudo (may not be available in all environments)
    if ! sudo -n true 2>/dev/null; then
        echo "${YELLOW}Note: Some installations may require sudo privileges${NC}"
        echo "  If the script fails, you may need to run with sudo or configure passwordless sudo"
        echo ""
    fi

    # Install system dependencies (tracks failures but continues)
    if ! install_system_deps; then
        echo "${YELLOW}Warning: Some system dependencies failed to install${NC}"
    fi

    # Verify build tools
    if ! verify_cmake_version; then
        echo "${RED}ERROR: CMake verification failed${NC}"
        return 1
    fi

    if ! verify_compiler; then
        echo "${RED}ERROR: Compiler verification failed${NC}"
        return 1
    fi

    # Setup Conan
    if ! setup_conan; then
        echo "${RED}ERROR: Conan setup failed${NC}"
        return 1
    fi

    # Install Python dependencies
    if ! install_python_deps; then
        echo "${RED}ERROR: Python dependencies installation failed${NC}"
        return 1
    fi

    # Final verification
    if verify_environment; then
        echo ""
        echo "${GREEN}✓ Development environment setup complete!${NC}"
        echo ""
        echo "Next steps:"
        echo "  1. mkdir build && cd build"
        echo "  2. cmake -DCMAKE_BUILD_TYPE=Release -GNinja .."
        echo "  3. cmake --build ."
        echo "  4. ctest --output-on-failure"
        echo ""
        echo "For more options and scenarios, see: ./scripts/build-release.sh"
        return 0
    else
        echo ""
        echo "${RED}Setup completed with errors. Please review the messages above.${NC}"
        return 1
    fi
}

# Run main function
main "$@"
