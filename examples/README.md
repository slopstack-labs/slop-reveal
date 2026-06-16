# SlopReveal Example Binary

`hello_calc.c` is a simple C program that prints factorials, Fibonacci numbers, powers of 2, and basic arithmetic. It's a good first target for testing SlopReveal because it has several distinct functions and predictable output, making it easy to verify that the decompiled result is correct.

## Build the example binary

### Linux / macOS

```bash
cd examples
gcc -O2 -o hello_calc hello_calc.c
# or with clang:
clang -O2 -o hello_calc hello_calc.c
```

Verify it runs:

```bash
./hello_calc
```

### Windows (MSVC — Developer Command Prompt)

```bat
cd examples
cl /O2 hello_calc.c /Fe:hello_calc.exe
hello_calc.exe
```

### Windows (MinGW / MSYS2)

```bash
cd examples
gcc -O2 -o hello_calc.exe hello_calc.c
./hello_calc.exe
```

---

## Run SlopReveal on the binary

Make sure SlopReveal is built or downloaded first. See the [main README](../README.md) for download links and build instructions.

### With Ollama (free, runs locally — recommended for first try)

Install Ollama first: see [docs/install-ollama.md](../docs/install-ollama.md).

```bash
# Pull a code model once
ollama pull qwen2.5-coder

# Decompile
./slopreveal -p ollama -m qwen2.5-coder examples/hello_calc
```

On Windows:

```bat
slopreveal.exe -p ollama -m qwen2.5-coder examples\hello_calc.exe
```

### With Claude (Anthropic API key required)

```bash
export ANTHROPIC_API_KEY=sk-ant-...
./slopreveal examples/hello_calc
```

### With pessimistic mode (disassemble first, then decompile)

Pessimistic mode feeds structured assembly to the AI instead of raw bytes. It tends to produce more accurate results at the cost of being slower.

```bash
./slopreveal --pessimistic -p ollama -m qwen2.5-coder examples/hello_calc
```

---

## What to expect

SlopReveal writes its output to `out/` by default:

```
out/
├── decompiled.c       ← predicted source code
└── CMakeLists.txt     ← generated build file
```

Because `hello_calc.c` has clear, well-structured functions (factorial, fibonacci, power), a decent code model should recover something close to the original — making it straightforward to judge quality.

To verify the decompiled output compiles and produces the same output as the original:

```bash
cd out
cmake -S . -B build && cmake --build build
./build/decompiled   # or build\Debug\decompiled.exe on Windows
```

Compare against the original:

```bash
./examples/hello_calc
```
