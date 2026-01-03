# Docker Development Environment (Claude Code Matching)

This Docker setup provides a development environment that matches Claude Code on the web's universal image for C++, Rust, and Python development.

## Quick Start

### Build the image

```bash
docker build -f Dockerfile.claude-code -t qlever:claude-code-dev .
```

### Run interactively

```bash
docker run -it --rm \
  -v $(pwd):/workspace/qlever \
  -w /workspace/qlever \
  qlever:claude-code-dev
```

### Using Docker Compose

```bash
# Start the container
docker-compose -f docker-compose.dev.yml up -d

# Enter the container
docker-compose -f docker-compose.dev.yml exec claude-code-dev bash

# Stop the container
docker-compose -f docker-compose.dev.yml down
```

## What's Included

### C++ Development
- **GCC** and **Clang** compilers (latest stable)
- **CMake** 3.27+
- **Ninja** build system
- **Boost** (program-options, iostreams, url, container)
- **ICU** (Unicode support)
- **OpenSSL** (TLS/SSL)
- **Zstd** (compression)
- **Jemalloc** (memory allocator)

### Rust Development
- **Rust** stable toolchain
- **Cargo** package manager
- **rustfmt** (code formatter)
- **clippy** (linter)

### Python Development
- **Python** 3.x
- **pip** (package manager)
- **poetry** (dependency management)
- **pre-commit** (git hooks)
- **pytest** (testing)
- **black** (code formatter)
- **flake8** (linter)
- **mypy** (type checker)
- **Conan** 2.x (C++ package manager)

## Development Workflow

Once inside the container:

```bash
# Setup development environment
make setup

# Configure CMake
make configure

# Build
make build

# Test
make test

# Benchmark
make benchmark
```

## Matching Claude Code on the Web

This Docker image is designed to match the Claude Code on the web universal image:

- ✅ C++: GCC and Clang compilers
- ✅ Rust: Rust toolchain with cargo
- ✅ Python: Python 3.x with pip, poetry, and common tools
- ✅ Build tools: CMake, Ninja, Make
- ✅ QLever dependencies: Boost, ICU, OpenSSL, Zstd

## Persisting Caches

The Docker Compose setup includes volumes for:
- **Cargo cache**: `/root/.cargo/registry` (Rust dependencies)
- **Pip cache**: `/root/.cache/pip` (Python packages)

This speeds up subsequent builds by reusing downloaded packages.

## Customization

To customize the environment:

1. Edit `Dockerfile.claude-code` to add/remove packages
2. Rebuild: `docker build -f Dockerfile.claude-code -t qlever:claude-code-dev .`
3. Or modify `docker-compose.dev.yml` for runtime configuration

## Troubleshooting

### Permission Issues

If you encounter permission issues with mounted volumes:

```bash
# Fix ownership (run from host)
sudo chown -R $USER:$USER .
```

### Out of Space

Clear Docker caches:

```bash
docker system prune -a
```

### Rebuild from Scratch

```bash
docker build --no-cache -f Dockerfile.claude-code -t qlever:claude-code-dev .
```

