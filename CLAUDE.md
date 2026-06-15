# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Linux dependencies:**
```bash
sudo apt install libcapstone-dev libcurl4-openssl-dev libssl-dev cmake build-essential
```

**Build:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Binary lands at build/slopreveal
```

**Debug build:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**macOS:**
```bash
brew install capstone curl openssl cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build build
```

There is no test suite — validate changes by running the binary against a real binary file.

## Architecture

SlopReveal is a single-binary C++17 CLI tool. The pipeline in `src/main.cpp` is:

1. **Config** (`src/config/config.cpp`) — loads `AppConfig` via layered resolution: TOML file → env vars → CLI flags. CLI wins. The config file path is extracted from argv in a separate pre-pass before the main parse so `--config` itself can override the path.

2. **Disassembler** (`src/disasm/disassembler.cpp`) — parses ELF/PE/Mach-O headers to extract the `.text` section and detect architecture, then runs Capstone to produce `DisasmChunk` slices of `chunk_size` instructions each.

3. **Cache** (`src/cache/cache.cpp`) — SHA-256 hashes the binary bytes (via OpenSSL) and stores per-chunk AI responses as files in `.slopreveal_cache/`. Cache key is `{binary_hash}/{provider}/{chunk_index}`. Skips network calls on cache hits.

4. **Provider** (`src/providers/`) — abstract `Provider` base with `generate(prompt) -> string`. `Provider::create()` in `provider.cpp` is the factory. Each concrete provider (`ClaudeProvider`, `OllamaProvider`, `OpenAIProvider`, `GeminiProvider`) constructs a JSON request and sends it via `HttpClient`.

5. **HttpClient** (`src/http/http_client.cpp`) — thin libcurl wrapper. One `post()` method, synchronous.

6. **Output** (`src/output/output.cpp`) — strips fenced code blocks from AI responses, concatenates all chunks into one file, writes `out/<stem>.<ext>` plus a `CMakeLists.txt` (C/C++) or `Cargo.toml` (Rust).

## Adding a new provider

1. Add `include/providers/<name>_provider.h` and `src/providers/<name>_provider.cpp` following the pattern of any existing provider.
2. Register it in `Provider::create()` in `src/providers/provider.cpp`.
3. Add the source file to `SOURCES` in `CMakeLists.txt`.
4. Add a sample config to `config_examples/`.

## Third-party headers (header-only)

- `third_party/json.hpp` — nlohmann/json, used by all providers for request/response parsing.
- `third_party/toml.hpp` — toml++ (TOML_EXCEPTIONS 0 mode), used by config.
- `third_party/getopt_compat.h` — Windows shim for `getopt_long`.

## Key defaults

| Setting | Default |
|---|---|
| Provider | `claude` |
| Model (Claude) | `claude-sonnet-4-6` |
| Chunk size | 200 instructions |
| Output dir | `out/` |
| Cache dir | `.slopreveal_cache/` |
| Language | `c` |
