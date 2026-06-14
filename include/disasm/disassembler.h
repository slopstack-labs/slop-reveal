#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

struct DisasmChunk {
    uint64_t start_addr;
    std::string text;        // formatted assembly text
    std::size_t insn_count;
};

class Disassembler {
public:
    explicit Disassembler(const std::string& arch_hint = "auto");
    ~Disassembler();

    // Disassemble raw bytes and return chunks of `chunk_size` instructions.
    std::vector<DisasmChunk> disassemble(const std::vector<uint8_t>& code,
                                          uint64_t base_addr,
                                          std::size_t chunk_size) const;

    // Detect ELF/PE/Mach-O and extract .text section bytes + arch.
    static std::vector<uint8_t> extract_text_section(const std::string& path,
                                                      std::string& arch_out,
                                                      uint64_t& base_addr_out);

    const std::string& arch() const { return arch_; }

private:
    void init_capstone();

    std::string arch_;
    void* cs_handle_ = nullptr; // csh stored as void* to avoid including capstone in header
};
