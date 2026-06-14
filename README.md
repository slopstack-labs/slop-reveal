# SlopReveal

**Decompile binaries to source code using AI.**

[![Build (Linux)](https://github.com/slopstack-labs/SlopReveal/actions/workflows/build-linux.yml/badge.svg)](https://github.com/slopstack-labs/SlopReveal/actions/workflows/build-linux.yml)
[![Build (macOS)](https://github.com/slopstack-labs/SlopReveal/actions/workflows/build-macos.yml/badge.svg)](https://github.com/slopstack-labs/SlopReveal/actions/workflows/build-macos.yml)
[![Build (Windows)](https://github.com/slopstack-labs/SlopReveal/actions/workflows/build-windows.yml/badge.svg)](https://github.com/slopstack-labs/SlopReveal/actions/workflows/build-windows.yml)

SlopReveal takes a compiled binary, disassembles it with [Capstone](https://www.capstone-engine.org/), and uses a configurable AI provider to predict the original source code — outputting a ready-to-build project in your language of choice.

---

## Features

- **Auto-detects architecture** from ELF/PE headers (x86, x64, ARM, ARM64, MIPS)
- **Four AI providers**: Claude, Ollama (local), OpenAI, Gemini
- **Result caching** by SHA-256 binary hash — skip re-sending unchanged chunks
- **Multi-language output**: C, C++, Rust, Python — with generated CMakeLists / Cargo.toml
- **Config layering**: TOML file → environment variables → CLI flags (CLI wins)
- **Progress bar** showing chunk completion

---

## Quick Start

```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Decompile with Claude (set ANTHROPIC_API_KEY)
export ANTHROPIC_API_KEY=sk-ant-...
./build/slopreveal /path/to/binary

# Decompile with Ollama (no key needed, runs locally)
./build/slopreveal -p ollama -m codellama /path/to/binary

# Output as C++ instead of C
./build/slopreveal -l cpp /path/to/binary

# Pipe raw output to stdout
./build/slopreveal --stdout /path/to/binary
```

Generated files land in `out/` with a `CMakeLists.txt` (or `Cargo.toml` for Rust).

---

## CLI Flags

| Flag | Short | Default | Description |
|------|-------|---------|-------------|
| `--binary <path>` | `-b` | _(positional)_ | Input binary |
| `--provider <name>` | `-p` | `claude` | `claude` \| `ollama` \| `openai` \| `gemini` |
| `--model <name>` | `-m` | provider default | Model override |
| `--api-key <key>` | `-k` | env var | API key |
| `--base-url <url>` | `-u` | provider default | Custom endpoint (useful for Ollama) |
| `--arch <arch>` | `-a` | `auto` | `auto` \| `x86` \| `x64` \| `arm` \| `arm64` \| `mips` |
| `--language <lang>` | `-l` | `c` | `c` \| `cpp` \| `rust` \| `python` |
| `--output <dir>` | `-o` | `out` | Output directory |
| `--stdout` | | | Print to stdout only |
| `--config <path>` | `-C` | `slopreveal.toml` | TOML config file |
| `--chunk-size <n>` | | `200` | Assembly instructions per AI request |
| `--no-cache` | | | Disable result cache |
| `--cache-dir <dir>` | | `.slopreveal_cache` | Cache directory |
| `--verbose` | `-v` | | Verbose logging |
| `--help` | `-h` | | Show help |

---

## Environment Variables

| Variable | Description |
|----------|-------------|
| `SLOPREVEAL_API_KEY` | API key (any provider) |
| `SLOPREVEAL_PROVIDER` | Provider name |
| `SLOPREVEAL_MODEL` | Model name |
| `ANTHROPIC_API_KEY` | Claude key (auto-detected) |
| `OPENAI_API_KEY` | OpenAI key (auto-detected) |
| `GEMINI_API_KEY` | Gemini key (auto-detected) |

---

## Config File (TOML)

Place `slopreveal.toml` in the working directory or pass `--config <path>`:

```toml
[provider]
name    = "claude"
model   = "claude-sonnet-4-6"
timeout = 120

[disasm]
arch       = "auto"
chunk_size = 200

[cache]
enabled = true
dir     = ".slopreveal_cache"

[output]
dir      = "out"
language = "c"
```

---

## Providers

| Provider | Key env var | Default model | Notes |
|----------|-------------|---------------|-------|
| `claude` | `ANTHROPIC_API_KEY` | `claude-sonnet-4-6` | Anthropic API |
| `ollama` | _(none)_ | `codellama` | Local — set `--base-url http://localhost:11434` |
| `openai` | `OPENAI_API_KEY` | `gpt-4o` | OpenAI API |
| `gemini` | `GEMINI_API_KEY` | `gemini-2.0-flash` | Google Generative AI |

---

## Building from Source

### Linux

```bash
sudo apt install libcapstone-dev libcurl4-openssl-dev libssl-dev cmake build-essential
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### macOS

```bash
brew install capstone curl openssl cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build build
```

### Windows

```powershell
# Install vcpkg then:
vcpkg install capstone:x64-windows openssl:x64-windows curl:x64-windows

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

---

## Pre-built Binaries

Pre-built executables for each release are attached to [GitHub Releases](https://github.com/slopstack-labs/SlopReveal/releases) for Linux (x64), macOS (arm64 + x64), and Windows (x64).

---

## How It Works

```
Binary file
    │
    ▼
ELF/PE parser → extract .text section + detect arch
    │
    ▼
Capstone disassembler → N × assembly chunks (200 insns each)
    │
    ▼  (SHA-256 cache check per chunk)
AI provider → source code prediction per chunk
    │
    ▼
Output writer → out/ directory
    ├── decompiled.c   (or .cpp / .rs / .py)
    └── CMakeLists.txt (or Cargo.toml)
```

---

## License

MIT
