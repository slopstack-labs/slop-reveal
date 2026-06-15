#include "config/config.h"
#include "disasm/disassembler.h"
#include "providers/provider.h"
#include "cache/cache.h"
#include "output/output.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

// Pessimistic mode: structured assembly prompt.
static std::string build_asm_prompt(const DisasmChunk& chunk,
                                     const std::string& arch,
                                     const std::string& language,
                                     std::size_t chunk_idx,
                                     std::size_t total_chunks) {
    std::ostringstream oss;
    oss << "You are a reverse engineering expert. The following is " << arch
        << " assembly code (chunk " << (chunk_idx + 1) << " of " << total_chunks
        << ", starting at address 0x" << std::hex << chunk.start_addr << std::dec
        << ", " << chunk.insn_count << " instructions).\n\n"
        << "Decompile this assembly into idiomatic " << language << " source code.\n"
        << "Return ONLY the source code in a fenced code block. "
        << "Add brief comments where logic is non-obvious. "
        << "Use sensible variable and function names.\n\n"
        << "Assembly:\n```\n" << chunk.text << "```\n";
    return oss.str();
}

// Normal mode: raw binary hex-dump prompt.
static std::string build_raw_prompt(const std::vector<uint8_t>& bytes,
                                     uint64_t offset,
                                     const std::string& language,
                                     std::size_t chunk_idx,
                                     std::size_t total_chunks) {
    std::ostringstream hex;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i % 16 == 0) {
            if (i > 0) hex << "\n";
            hex << std::hex << std::setw(8) << std::setfill('0') << (offset + i) << "  ";
        }
        hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
    }

    std::ostringstream oss;
    oss << "You are a reverse engineering expert. The following is raw binary data "
        << "(chunk " << (chunk_idx + 1) << " of " << total_chunks
        << ", file offset 0x" << std::hex << offset << std::dec
        << ", " << bytes.size() << " bytes).\n\n"
        << "Decompile this binary data into idiomatic " << language << " source code.\n"
        << "Return ONLY the source code in a fenced code block. "
        << "Add brief comments where logic is non-obvious. "
        << "Use sensible variable and function names.\n\n"
        << "Binary data (hex):\n```\n" << hex.str() << "\n```\n";
    return oss.str();
}

static void print_progress(std::size_t done, std::size_t total, bool verbose) {
    int pct = static_cast<int>(done * 100 / total);
    int bar_width = 40;
    int filled = bar_width * pct / 100;

    std::cerr << "\r[";
    for (int i = 0; i < bar_width; ++i)
        std::cerr << (i < filled ? '#' : '-');
    std::cerr << "] " << std::setw(3) << pct << "% (" << done << "/" << total << ")";
    if (done == total) std::cerr << "\n";
    std::cerr.flush();
    (void)verbose;
}

int main(int argc, char** argv) {
    try {
        AppConfig cfg = Config::load(argc, argv);

        if (cfg.output.verbose) {
            std::cerr << "Binary:   " << cfg.binary_path << "\n"
                      << "Provider: " << cfg.provider.name << " / " << cfg.provider.model << "\n"
                      << "Mode:     " << (cfg.pessimistic ? "pessimistic" : "normal") << "\n"
                      << "Language: " << cfg.output.language << "\n"
                      << "Output:   " << cfg.output.out_dir << "\n";
            if (cfg.pessimistic)
                std::cerr << "Arch:     " << cfg.disasm.arch << "\n";
        }

        // Read binary
        std::ifstream bin_file(cfg.binary_path, std::ios::binary);
        if (!bin_file)
            throw std::runtime_error("Cannot open binary: " + cfg.binary_path);
        std::vector<uint8_t> bin_data((std::istreambuf_iterator<char>(bin_file)),
                                       std::istreambuf_iterator<char>());

        // Cache + provider (shared by both modes)
        Cache cache(cfg.cache.cache_dir);
        cache.set_enabled(cfg.cache.enabled);

        std::string bin_hash = Cache::hash_binary(bin_data);
        if (cfg.output.verbose)
            std::cerr << "Binary hash: " << bin_hash << "\n";

        auto provider = Provider::create(cfg.provider);
        std::cerr << "Using provider: " << provider->name()
                  << " (" << cfg.provider.model << ")\n";
        std::cerr << "Mode: " << (cfg.pessimistic
            ? "pessimistic (binary → Capstone → AI)"
            : "normal (binary → AI)") << "\n\n";

        std::vector<std::string> results;

        if (!cfg.pessimistic) {
            // ── Normal mode ──────────────────────────────────────────────────
            // Chunk raw binary bytes and send as hex dumps directly to the AI.
            std::size_t total      = bin_data.size();
            std::size_t chunk_size = cfg.disasm.chunk_size;
            std::size_t num_chunks = (total + chunk_size - 1) / chunk_size;

            std::cerr << "Binary size: " << total << " bytes → "
                      << num_chunks << " chunk(s) of up to "
                      << chunk_size << " bytes.\n";

            results.reserve(num_chunks);
            std::string cache_key = provider->name();

            for (std::size_t i = 0; i < num_chunks; ++i) {
                print_progress(i, num_chunks, cfg.output.verbose);

                auto cached = cache.get(bin_hash, cache_key, i);
                if (cached) {
                    if (cfg.output.verbose)
                        std::cerr << "\n  [cache hit] chunk " << i << "\n";
                    results.push_back(*cached);
                    continue;
                }

                std::size_t offset = i * chunk_size;
                std::size_t end    = std::min(offset + chunk_size, total);
                std::vector<uint8_t> chunk(bin_data.begin() + offset,
                                           bin_data.begin() + end);

                std::string prompt = build_raw_prompt(chunk, static_cast<uint64_t>(offset),
                                                      cfg.output.language, i, num_chunks);
                std::string response = provider->generate(prompt);
                cache.put(bin_hash, cache_key, i, response);
                results.push_back(std::move(response));
            }
            print_progress(num_chunks, num_chunks, cfg.output.verbose);

        } else {
            // ── Pessimistic mode ─────────────────────────────────────────────
            // Disassemble with Capstone first, then decompile chunk by chunk.
            std::string detected_arch;
            uint64_t base_addr = 0;
            std::vector<uint8_t> text_bytes;

            try {
                text_bytes = Disassembler::extract_text_section(cfg.binary_path, detected_arch, base_addr);
            } catch (const std::exception& e) {
                std::cerr << "Warning: section extraction failed (" << e.what()
                          << "), falling back to raw bytes.\n";
                text_bytes = bin_data;
                detected_arch = "x64";
                base_addr = 0;
            }

            if (cfg.disasm.arch == "auto" || cfg.disasm.arch.empty())
                cfg.disasm.arch = detected_arch;

            std::cerr << "Detected arch: " << cfg.disasm.arch
                      << ", code bytes: " << text_bytes.size() << "\n";

            Disassembler dis(cfg.disasm.arch);
            auto chunks = dis.disassemble(text_bytes, base_addr, cfg.disasm.chunk_size);

            if (chunks.empty())
                throw std::runtime_error("No instructions disassembled. Is this a valid binary?");

            std::cerr << "Disassembled into " << chunks.size() << " chunk(s).\n";
            results.reserve(chunks.size());
            std::string cache_key = provider->name() + "-pessimistic";

            for (std::size_t i = 0; i < chunks.size(); ++i) {
                print_progress(i, chunks.size(), cfg.output.verbose);

                auto cached = cache.get(bin_hash, cache_key, i);
                if (cached) {
                    if (cfg.output.verbose)
                        std::cerr << "\n  [cache hit] chunk " << i << "\n";
                    results.push_back(*cached);
                    continue;
                }

                std::string prompt = build_asm_prompt(chunks[i], cfg.disasm.arch,
                                                      cfg.output.language, i, chunks.size());
                std::string response = provider->generate(prompt);
                cache.put(bin_hash, cache_key, i, response);
                results.push_back(std::move(response));
            }
            print_progress(chunks.size(), chunks.size(), cfg.output.verbose);
        }

        // Write output
        Output out(cfg.output);
        out.write(cfg.binary_path, results, cfg.output.language);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }
}
