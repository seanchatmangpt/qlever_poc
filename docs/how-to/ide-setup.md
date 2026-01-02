# IDE Setup Guide

This guide provides step-by-step instructions for setting up QLever development in popular IDEs.

## Visual Studio Code (VS Code)

### Prerequisites

- VS Code installed (latest version recommended)
- C/C++ extension installed: `ms-vscode.cpptools-themes`
- CMake Tools extension installed: `ms-vscode.cmake-tools`
- clang-format extension installed: `xaver.clang-format`

### Setup

1. **Install Extensions**

   Open VS Code and install required extensions:
   ```
   Extensions > Search "C++" > Install "C/C++" (Microsoft)
   Extensions > Search "CMake Tools" > Install (Microsoft)
   Extensions > Search "clang-format" > Install (Xaver)
   ```

2. **Configure VS Code Settings**

   Create or update `.vscode/settings.json`:

   ```json
   {
     "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
     "C_Cpp.default.cppStandard": "c++20",
     "C_Cpp.default.cStandard": "c11",
     "C_Cpp.intelliSenseEngine": "tag-parser",
     "editor.formatOnSave": true,
     "[cpp]": {
       "editor.defaultFormatter": "xaver.clang-format",
       "editor.formatOnSave": true
     },
     "clangFormat.style": "file",
     "clangFormat.fallbackStyle": "Google",
     "cmake.generator": "Ninja",
     "cmake.configureOnEdit": false,
     "cmake.configureOnOpen": true,
     "cmake.sourceDirectory": "${workspaceFolder}",
     "cmake.buildDirectory": "${workspaceFolder}/build",
     "files.exclude": {
       "**/.*": true,
       "build": false
     }
   }
   ```

3. **Generate Compile Commands**

   CMake Tools will auto-generate `compile_commands.json` on configure. If needed manually:

   ```bash
   cd build
   cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
   ```

4. **Configure IntelliSense**

   - Open Command Palette: `Ctrl+Shift+P` (Linux/Windows) or `Cmd+Shift+P` (macOS)
   - Run: `C/C++: Edit Configurations (JSON)`
   - Set `"intelliSenseEngine": "tag-parser"`

### Usage

- **Build**: `Ctrl+Shift+B` → Select "CMake: Build"
- **Run Tests**: Use Test Explorer (left sidebar)
- **Format Code**: `Shift+Alt+F` on open file
- **Code Navigation**: `Ctrl+Click` to go to definition
- **Find References**: `Ctrl+Shift+F` or right-click

### Debugging

1. **Install CodeLLDB** extension (for Clang/LLVM debugging)
2. **Create `.vscode/launch.json`**:

   ```json
   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "Debug ServerMain",
         "type": "lldb",
         "request": "launch",
         "program": "${workspaceFolder}/build/src/server/ServerMain",
         "args": ["--help"],
         "cwd": "${workspaceFolder}",
         "stopOnEntry": false,
         "console": "integratedTerminal"
       }
     ]
   }
   ```

3. Set breakpoints and press `F5` to debug.

---

## JetBrains CLion

### Prerequisites

- CLion installed (2023.2 or later)
- Toolchain configured (GCC 11+ or Clang 16+)
- CMake 3.27+ available in PATH

### Setup

1. **Open Project**

   - File → Open → Select QLever directory
   - CLion auto-detects CMakeLists.txt

2. **Configure Toolchain**

   - Settings → Build, Execution, Deployment → Toolchains
   - Set:
     - **C Compiler**: `clang` or `gcc`
     - **C++ Compiler**: `clang++` or `g++`
     - **CMake**: `/usr/bin/cmake` or auto-detected
     - **Ninja**: Auto-detected

3. **Configure CMake Profile**

   - Settings → Build, Execution, Deployment → CMake
   - **Build Type**: Release (for performance testing)
   - **CMake options**: `-DUSE_PARALLEL=true`
   - **Generator**: Ninja

4. **Code Style**

   - Settings → Editor → Code Style → C/C++
   - Import scheme: `.clion.xml` (if provided)
   - Or set manually:
     - **Indent**: 2 spaces
     - **Line length**: 100
     - **Spacing**: Google style

5. **Enable ClangFormat**

   - Settings → Tools → ClangFormat
   - **Enable ClangFormat**: ON
   - **ClangFormat path**: `/usr/bin/clang-format` or auto-detected
   - **Reformat code on Save**: ON

### Usage

- **Build**: `Ctrl+F9` (Windows/Linux) or `Cmd+F9` (macOS)
- **Run**: `Shift+F10` (Windows/Linux) or `Ctrl+R` (macOS)
- **Test**: `Ctrl+Shift+F10` on test class/function
- **Debug**: `Shift+F9` (Windows/Linux) or `Ctrl+D` (macOS)
- **Reformat**: `Ctrl+Alt+L` (Windows/Linux) or `Cmd+Alt+L` (macOS)

### Debugging

CLion integrates GDB/LLDB debugging automatically:

1. Set breakpoint (click line number)
2. Run → Debug 'target_name'
3. Step through code with keyboard shortcuts

### Run Configurations

Create custom run configurations:

1. Run → Edit Configurations
2. Add new CMake Target:
   - **Target**: `ServerMain`
   - **Arguments**: `--help` (or other args)
   - **Working directory**: `${PROJECT_DIR}`

---

## Neovim / Vim

### Prerequisites

- Neovim 0.7+ (or Vim 9.0+)
- Language Server Protocol (LSP) configured
- clang-tools installed (`clang-tools-16` package)

### Setup with LSP

1. **Install clangd LSP**

   ```bash
   sudo apt-get install clang-tools-16
   ln -s /usr/bin/clangd-16 ~/.local/bin/clangd
   ```

2. **Install Neovim Plugin Manager**

   Using `vim-plug`:
   ```bash
   curl -fLo ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
     https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
   ```

3. **Configure `~/.config/nvim/init.vim`**

   ```vim
   " Install plugins
   call plug#begin()
   Plug 'neovim/nvim-lspconfig'
   Plug 'hrsh7th/cmp-nvim-lsp'
   Plug 'hrsh7th/nvim-cmp'
   Plug 'vim-airline/vim-airline'
   Plug 'nvim-treesitter/nvim-treesitter', {'do': ':TSUpdate'}
   call plug#end()

   " LSP Configuration
   lua << EOF
   local lspconfig = require('lspconfig')
   lspconfig.clangd.setup({
     cmd = {"clangd", "--background-index"},
     root_dir = lspconfig.util.root_pattern("CMakeLists.txt", ".git"),
   })
   EOF

   " Format on save
   autocmd BufWritePre *.cpp,*.h silent! !clang-format -i %
   ```

4. **Configure clangd LSP**

   Create `.clang-format` in project root (already exists in QLever).

### Usage

- **Code completion**: Ctrl+N (insert mode) or use nvim-cmp
- **Go to definition**: `gd` (normal mode)
- **Find references**: `gr` (normal mode, requires plugin)
- **Format**: `:! clang-format -i %` or use LSP formatting
- **Diagnostics**: `:LspDiagnostics`

### Build Integration

Add to `~/.config/nvim/init.vim`:

```vim
" Build with make
nnoremap <leader>b :! cd build && cmake --build . -j$(nproc)<CR>
nnoremap <leader>t :! cd build && ctest --output-on-failure<CR>
```

---

## Other Editors

### VSCodium (Privacy-Focused VS Code)

Use same setup as VS Code. VSCodium is fully compatible with VS Code extensions from OpenVSX registry.

### Sublime Text 4

1. Install `C++` package via Package Control
2. Install `CMake` package for syntax highlighting
3. Create build system:
   - Tools → Build System → New Build System
   - ```json
     {
       "cmd": ["sh", "-c", "cd build && cmake --build . -j$(nproc)"],
       "working_dir": "$project_path",
       "selector": "source.c++",
       "variants": [
         {
           "name": "Run Tests",
           "cmd": ["sh", "-c", "cd build && ctest --output-on-failure"]
         }
       ]
     }
     ```

### Emacs

1. Install `lsp-mode` and `ccls` (LSP server)
2. Configure in `~/.emacs.d/init.el`:
   ```elisp
   (use-package lsp-mode :ensure t)
   (use-package ccls :ensure t)
   (add-hook 'c++-mode-hook #'lsp)
   ```

---

## Tips & Tricks

### Fast Local Development

Use the convenience script:

```bash
# Fast build + test cycle
./scripts/quick-build.sh

# Or make target
make dev  # Builds and runs quick tests
```

### Running Single Tests

```bash
# Run one test
ctest -R "QueryPlannerTest" -j$(nproc) --output-on-failure

# Run with regex pattern
ctest -R "Join.*Test" -j$(nproc)
```

### Code Formatting

```bash
# Auto-format all changed files
./scripts/format.sh

# Format specific file
clang-format -i src/engine/QueryPlanner.cpp
```

### Static Analysis

```bash
# Run clang-tidy on hot-path
make lint

# Run on specific file
clang-tidy -p build src/engine/QueryPlanner.cpp
```

### Debugging Tips

1. **Add debug symbols** to build:
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   cmake --build .
   ```

2. **Use valgrind** for memory issues:
   ```bash
   valgrind --leak-check=full ./build/src/server/ServerMain
   ```

3. **Gdb quick start**:
   ```bash
   gdb ./build/src/server/ServerMain
   (gdb) b main
   (gdb) r
   (gdb) n  # Next line
   (gdb) p variable_name  # Print variable
   ```

---

## Troubleshooting

### IntelliSense not working

- Regenerate `compile_commands.json`: `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..`
- Restart IDE
- Clear IntelliSense cache: In VS Code, `Ctrl+Shift+P` → "C/C++: Rescan Solutions"

### Slow compilation

- Use `ccache` for compilation caching
- Build incrementally (only modified files)
- Use Release build for testing (Debug is slower)

### Debugger not working

- Ensure debug symbols: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- Verify debugger availability: `which gdb` or `which lldb`
- Try different debugger in IDE settings

---

## Resources

- [CMake Tools Documentation](https://cmake.org/cmake/help/latest/)
- [Clang Tools Documentation](https://clang.llvm.org/tools/index.html)
- [VS Code C++ Guide](https://code.visualstudio.com/docs/languages/cpp)
- [CLion Help](https://www.jetbrains.com/help/clion/)
- [Neovim Documentation](https://neovim.io/doc/user/)

For additional help, see [CONTRIBUTING.md](../CONTRIBUTING.md) or the [Quick Start Guide](quick-start.md).
