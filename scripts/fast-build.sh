#!/bin/bash
# QLever Fast Iteration Build System
# Enables per-second rebuilds using ccache + RAM disk + Ninja incremental compilation
# Usage: ./scripts/fast-build.sh [target] [options]

set -e

QLEVER_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${QLEVER_ROOT}/build}"
RAMDISK_SIZE="${RAMDISK_SIZE:-4G}"  # Size of RAM disk for build artifacts
CCACHE_SIZE="${CCACHE_SIZE:-10G}"   # Size of ccache
JOBS="${JOBS:-$(nproc)}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== QLever Fast Iteration Build System ===${NC}"

# Setup ccache
setup_ccache() {
    echo -e "${YELLOW}Setting up ccache...${NC}"
    export CCACHE_DIR="${CCACHE_DIR:-$HOME/.cache/ccache}"
    export CCACHE_MAXSIZE="${CCACHE_SIZE}"
    export CCACHE_BASEDIR="${QLEVER_ROOT}"
    export CCACHE_SLOPPINESS="file_macro,time_macros"

    # Create ccache dir
    mkdir -p "${CCACHE_DIR}"

    # Get current stats
    CCACHE_STATS=$(ccache -s 2>/dev/null | grep "Cache hit rate" || echo "Cache not initialized")
    echo -e "${GREEN}✓ ccache configured: ${CCACHE_DIR}${NC}"
    echo "  $CCACHE_STATS"
}

# Setup RAM disk for build directory
setup_ramdisk() {
    if [[ "$USE_RAMDISK" != "yes" ]]; then
        return
    fi

    echo -e "${YELLOW}Setting up RAM disk...${NC}"

    RAMDISK_MOUNT="/mnt/qlever-ramdisk"

    # Check if already mounted
    if mountpoint -q "${RAMDISK_MOUNT}" 2>/dev/null; then
        echo -e "${GREEN}✓ RAM disk already mounted at ${RAMDISK_MOUNT}${NC}"
        BUILD_DIR="${RAMDISK_MOUNT}/build"
        return
    fi

    # Create mount point
    sudo mkdir -p "${RAMDISK_MOUNT}" 2>/dev/null || RAMDISK_MOUNT="/tmp/qlever-ramdisk"

    # Try to create RAM disk
    if [[ "$RAMDISK_MOUNT" == "/mnt/"* ]]; then
        sudo mount -t tmpfs -o size="${RAMDISK_SIZE}" tmpfs "${RAMDISK_MOUNT}" 2>/dev/null || {
            echo -e "${YELLOW}Warning: Could not create /mnt RAM disk, using /tmp${NC}"
            RAMDISK_MOUNT="/tmp/qlever-ramdisk"
            mkdir -p "${RAMDISK_MOUNT}"
        }
    else
        mkdir -p "${RAMDISK_MOUNT}"
    fi

    BUILD_DIR="${RAMDISK_MOUNT}/build"
    mkdir -p "${BUILD_DIR}"
    echo -e "${GREEN}✓ RAM disk ready at ${RAMDISK_MOUNT}${NC}"
}

# Initialize build directory
init_build() {
    echo -e "${YELLOW}Initializing build directory...${NC}"

    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    # Check if cmake needs to be run
    if [[ ! -f "build.ninja" ]]; then
        echo "Running CMake configuration..."
        conan install .. --output-folder=. --build=missing -s build_type="${BUILD_TYPE}" >/dev/null 2>&1 || true

        cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
              -GNinja \
              -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
              -DCMAKE_C_COMPILER_LAUNCHER=ccache \
              -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
              -DBUILD_TESTING=OFF \
              .. >/dev/null 2>&1 || {
            echo -e "${RED}CMake failed, retrying with full output...${NC}"
            cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
                  -GNinja \
                  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
                  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
                  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
                  -DBUILD_TESTING=OFF \
                  ..
        }
    fi

    echo -e "${GREEN}✓ Build directory ready${NC}"
}

# Build target
build_target() {
    local target="${1:-ServerMain}"
    local start_time=$(date +%s%3N)

    echo -e "${YELLOW}Building ${target}...${NC}"

    cd "${BUILD_DIR}"
    ninja "${target}" -j "${JOBS}" 2>&1 | tail -20

    local end_time=$(date +%s%3N)
    local duration=$((end_time - start_time))

    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✓ Build completed in ${duration}ms${NC}"
        ccache -s 2>/dev/null | grep "Cache hit rate" || true
        return 0
    else
        echo -e "${RED}✗ Build failed${NC}"
        return 1
    fi
}

# Watch mode: rebuild on file changes
watch_mode() {
    local target="${1:-ServerMain}"

    if ! command -v inotifywait &> /dev/null; then
        echo -e "${RED}inotify-tools not installed. Install with: sudo apt-get install inotify-tools${NC}"
        return 1
    fi

    echo -e "${YELLOW}Watching for changes (Ctrl+C to stop)...${NC}"
    echo "Press any key to rebuild manually"

    # Background process to read keypresses
    (while IFS= read -r -t 0.1 -n 1 key; do
        if [[ -n "$key" ]]; then
            echo "Manual rebuild triggered"
            build_target "${target}"
        fi
    done) &
    KEYPOLL_PID=$!

    # Watch for file changes
    inotifywait -m -r \
        -e modify,create,delete \
        --exclude '(build/|\.git/|\.conan2/|__pycache__|\.o$)' \
        "${QLEVER_ROOT}/src" "${QLEVER_ROOT}/test" | while read path action file; do
        if [[ "$file" =~ \.(cpp|h|hpp|c|cc|cxx)$ ]]; then
            echo -e "${YELLOW}[$(date +%H:%M:%S)] Changed: $file${NC}"
            build_target "${target}"
        fi
    done
}

# Show build statistics
show_stats() {
    echo -e "\n${GREEN}=== Build Statistics ===${NC}"
    echo "Build directory: ${BUILD_DIR}"
    echo "ccache directory: ${CCACHE_DIR}"
    echo "ccache size: ${CCACHE_SIZE}"
    echo "Parallel jobs: ${JOBS}"
    echo ""
    ccache -s 2>/dev/null | head -20 || echo "ccache not initialized"
}

# Main logic
main() {
    local target="${1:-ServerMain}"
    local mode="${2:-build}"

    case "$target" in
        --help|-h)
            echo "Usage: $0 [target] [mode] [options]"
            echo ""
            echo "Targets:"
            echo "  ServerMain              (default) Build SPARQL query server"
            echo "  IndexBuilderMain        Build RDF index builder"
            echo "  all                     Build both"
            echo ""
            echo "Modes:"
            echo "  build                   (default) Build once"
            echo "  watch                   Watch for changes and rebuild"
            echo "  stats                   Show build statistics"
            echo ""
            echo "Options:"
            echo "  USE_RAMDISK=yes         Use RAM disk for build artifacts (4GB)"
            echo "  BUILD_TYPE=Release      Debug or Release (default: Release)"
            echo "  JOBS=N                  Parallel jobs (default: nproc)"
            echo ""
            echo "Examples:"
            echo "  ./scripts/fast-build.sh ServerMain build"
            echo "  ./scripts/fast-build.sh ServerMain watch"
            echo "  USE_RAMDISK=yes ./scripts/fast-build.sh ServerMain watch"
            echo "  ./scripts/fast-build.sh all stats"
            exit 0
            ;;
    esac

    # Handle mode as second positional argument
    if [[ "$mode" != "build" && "$mode" != "watch" && "$mode" != "stats" && -n "$mode" ]]; then
        target="$mode"
        mode="build"
    fi

    # Setup
    setup_ccache
    setup_ramdisk
    init_build

    # Execute mode
    case "$mode" in
        build)
            if [[ "$target" == "all" ]]; then
                build_target "ServerMain" && build_target "IndexBuilderMain"
            else
                build_target "${target}"
            fi
            ;;
        watch)
            watch_mode "${target}"
            ;;
        stats)
            show_stats
            ;;
        *)
            build_target "${target}"
            ;;
    esac

    # Show final stats
    show_stats
}

main "$@"
