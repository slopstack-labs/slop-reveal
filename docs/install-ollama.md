# Installing Ollama

Ollama lets you run AI models locally — no API key, no cloud, no cost. This is the easiest way to get started with SlopReveal.

---

## Linux

### One-line install (recommended)

```bash
curl -fsSL https://ollama.com/install.sh | sh
```

This installs the `ollama` binary to `/usr/local/bin` and sets up a systemd service that starts automatically on boot.

Verify the install:

```bash
ollama --version
```

### Manual install (no curl)

1. Download the latest release from [github.com/ollama/ollama/releases](https://github.com/ollama/ollama/releases) — pick the `ollama-linux-amd64` binary (or `arm64` for ARM).
2. Make it executable and move it into your PATH:

```bash
chmod +x ollama-linux-amd64
sudo mv ollama-linux-amd64 /usr/local/bin/ollama
```

3. Start the server manually (or set up systemd yourself):

```bash
ollama serve &
```

### GPU support (optional but faster)

Ollama auto-detects NVIDIA and AMD GPUs. For NVIDIA, the CUDA driver must already be installed (`nvidia-smi` should work). For AMD, ROCm drivers are needed. No extra Ollama config is required — it picks up the GPU automatically.

---

## Windows

### Installer (recommended)

1. Go to [ollama.com/download](https://ollama.com/download) and click **Download for Windows**.
2. Run the downloaded `OllamaSetup.exe`.
3. Follow the installer — it adds `ollama` to your PATH and registers a background service.
4. Open a new **Command Prompt** or **PowerShell** and verify:

```powershell
ollama --version
```

The Ollama server starts automatically in the background (check the system tray).

### GPU support on Windows

- **NVIDIA**: Install the latest Game Ready or Studio driver from [nvidia.com](https://www.nvidia.com/Download/index.aspx). CUDA itself is bundled with Ollama — no separate CUDA install needed.
- **AMD**: Requires ROCm-compatible hardware and drivers. Check [ollama.com](https://ollama.com) for the current AMD support matrix on Windows.

---

## Pulling a code model

Once Ollama is installed and the server is running, pull a model suited for code:

```bash
# Lightweight, fast — good starting point
ollama pull qwen2.5-coder

# Larger, more accurate (requires more RAM/VRAM)
ollama pull qwen2.5-coder:14b

# Classic option
ollama pull codellama

# DeepSeek — strong on decompilation tasks
ollama pull deepseek-coder
```

Model sizes range from ~2 GB to 20+ GB. If you have less than 8 GB of RAM, start with `qwen2.5-coder` (the default 7b variant).

Verify the model is ready:

```bash
ollama run qwen2.5-coder "Say hello"
```

---

## Troubleshooting

### "connection refused" when SlopReveal tries to reach Ollama

The Ollama server isn't running. Start it:

```bash
# Linux / macOS
ollama serve

# Windows — re-launch the Ollama app from the Start menu or system tray
```

### Port conflict

By default Ollama listens on `http://localhost:11434`. If that port is taken, set a different one:

```bash
OLLAMA_HOST=0.0.0.0:11435 ollama serve
```

Then tell SlopReveal where to find it:

```bash
./slopreveal -p ollama --base-url http://localhost:11435 examples/hello_calc
```

### Model runs slowly (CPU only)

If the GPU isn't being used, check:

- **Linux/NVIDIA**: `nvidia-smi` shows a running process while the model is loaded.
- **Windows**: Task Manager → Performance → GPU shows activity.

If neither shows GPU usage, your driver may need updating, or the model is too large to fit in VRAM and is partially offloaded to CPU — this is expected for large models on small GPUs and will be slow.

### "out of memory" error

Use a smaller model variant:

```bash
ollama pull qwen2.5-coder:3b
```

---

## Further reading

- [Ollama model library](https://ollama.com/library) — full list of available models
- [Ollama GitHub](https://github.com/ollama/ollama) — source, issues, release notes
- [SlopReveal example binary](../examples/README.md) — test SlopReveal with a prebuilt example
