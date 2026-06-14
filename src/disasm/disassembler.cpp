#include "disasm/disassembler.h"
#include <capstone/capstone.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

// Minimal ELF header detection
static bool is_elf(const std::vector<uint8_t>& data) {
    return data.size() >= 4 &&
           data[0] == 0x7f && data[1] == 'E' && data[2] == 'L' && data[3] == 'F';
}

// Minimal PE detection
static bool is_pe(const std::vector<uint8_t>& data) {
    return data.size() >= 2 && data[0] == 'M' && data[1] == 'Z';
}

struct ElfHeader32 {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ElfHeader64 {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ElfShdr64 {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

struct ElfShdr32 {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

// e_machine values
static const uint16_t EM_386    = 3;
static const uint16_t EM_ARM    = 40;
static const uint16_t EM_X86_64 = 62;
static const uint16_t EM_AARCH64 = 183;
static const uint16_t EM_MIPS  = 8;

std::vector<uint8_t> Disassembler::extract_text_section(const std::string& path,
                                                          std::string& arch_out,
                                                          uint64_t& base_addr_out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open binary: " + path);

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());

    if (data.empty()) throw std::runtime_error("Empty binary file.");

    if (is_elf(data)) {
        bool is64 = (data[4] == 2); // EI_CLASS: 1=32bit, 2=64bit
        bool le   = (data[5] == 1); // EI_DATA: 1=LE
        (void)le; // always treat as LE on x86

        if (is64) {
            ElfHeader64 hdr;
            std::memcpy(&hdr, data.data(), sizeof(hdr));

            // Detect arch
            switch (hdr.e_machine) {
                case EM_X86_64:   arch_out = "x64"; break;
                case EM_AARCH64:  arch_out = "arm64"; break;
                case EM_MIPS:     arch_out = "mips"; break;
                default:          arch_out = "x64"; break;
            }

            // Find section name table
            if (hdr.e_shoff + (hdr.e_shnum * sizeof(ElfShdr64)) > data.size())
                throw std::runtime_error("Malformed ELF: section headers out of bounds.");

            ElfShdr64 shstr;
            std::memcpy(&shstr, data.data() + hdr.e_shoff + hdr.e_shstrndx * sizeof(ElfShdr64),
                        sizeof(ElfShdr64));

            // Find .text section
            for (uint16_t i = 0; i < hdr.e_shnum; ++i) {
                ElfShdr64 sh;
                std::memcpy(&sh, data.data() + hdr.e_shoff + i * sizeof(ElfShdr64), sizeof(ElfShdr64));
                if (sh.sh_name + 5 > shstr.sh_size) continue;
                const char* name = reinterpret_cast<const char*>(data.data() + shstr.sh_offset + sh.sh_name);
                if (std::strncmp(name, ".text", 5) == 0) {
                    base_addr_out = sh.sh_addr;
                    return std::vector<uint8_t>(data.data() + sh.sh_offset,
                                                data.data() + sh.sh_offset + sh.sh_size);
                }
            }
        } else {
            ElfHeader32 hdr;
            std::memcpy(&hdr, data.data(), sizeof(hdr));

            switch (hdr.e_machine) {
                case EM_386:  arch_out = "x86"; break;
                case EM_ARM:  arch_out = "arm"; break;
                case EM_MIPS: arch_out = "mips"; break;
                default:      arch_out = "x86"; break;
            }

            ElfShdr32 shstr;
            std::memcpy(&shstr, data.data() + hdr.e_shoff + hdr.e_shstrndx * sizeof(ElfShdr32),
                        sizeof(ElfShdr32));

            for (uint16_t i = 0; i < hdr.e_shnum; ++i) {
                ElfShdr32 sh;
                std::memcpy(&sh, data.data() + hdr.e_shoff + i * sizeof(ElfShdr32), sizeof(ElfShdr32));
                const char* name = reinterpret_cast<const char*>(data.data() + shstr.sh_offset + sh.sh_name);
                if (std::strncmp(name, ".text", 5) == 0) {
                    base_addr_out = sh.sh_addr;
                    return std::vector<uint8_t>(data.data() + sh.sh_offset,
                                                data.data() + sh.sh_offset + sh.sh_size);
                }
            }
        }
        // Fallback: return all data
        arch_out = "x64";
        base_addr_out = 0;
        return data;
    }

    if (is_pe(data)) {
        // PE: find PE header
        uint32_t pe_offset = 0;
        std::memcpy(&pe_offset, data.data() + 0x3C, 4);
        if (pe_offset + 6 > data.size()) throw std::runtime_error("Malformed PE.");

        uint16_t machine = 0;
        std::memcpy(&machine, data.data() + pe_offset + 4, 2);

        if (machine == 0x8664)      arch_out = "x64";
        else if (machine == 0x014c) arch_out = "x86";
        else if (machine == 0xAA64) arch_out = "arm64";
        else if (machine == 0x01C0) arch_out = "arm";
        else                         arch_out = "x64";

        base_addr_out = 0x400000; // default PE image base
        return data; // simplified: disassemble whole blob (real PE parsing is complex)
    }

    // Unknown format: try as raw bytes
    arch_out = "x64";
    base_addr_out = 0;
    return data;
}

Disassembler::Disassembler(const std::string& arch_hint) : arch_(arch_hint) {
    init_capstone();
}

Disassembler::~Disassembler() {
    if (cs_handle_) {
        csh h = reinterpret_cast<csh>(reinterpret_cast<uintptr_t>(cs_handle_));
        cs_close(&h);
    }
}

void Disassembler::init_capstone() {
    cs_arch ca;
    cs_mode  cm;

    if (arch_ == "x86") {
        ca = CS_ARCH_X86; cm = CS_MODE_32;
    } else if (arch_ == "x64") {
        ca = CS_ARCH_X86; cm = CS_MODE_64;
    } else if (arch_ == "arm") {
        ca = CS_ARCH_ARM; cm = CS_MODE_ARM;
    } else if (arch_ == "arm64") {
        ca = CS_ARCH_ARM64; cm = CS_MODE_ARM;
    } else if (arch_ == "mips") {
        ca = CS_ARCH_MIPS; cm = static_cast<cs_mode>(CS_MODE_MIPS32 | CS_MODE_BIG_ENDIAN);
    } else {
        // default to x64
        ca = CS_ARCH_X86; cm = CS_MODE_64;
        arch_ = "x64";
    }

    csh handle;
    if (cs_open(ca, cm, &handle) != CS_ERR_OK)
        throw std::runtime_error("Failed to initialize Capstone for arch: " + arch_);

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    cs_handle_ = reinterpret_cast<void*>(static_cast<uintptr_t>(handle));
}

std::vector<DisasmChunk> Disassembler::disassemble(const std::vector<uint8_t>& code,
                                                     uint64_t base_addr,
                                                     std::size_t chunk_size) const {
    csh handle = static_cast<csh>(reinterpret_cast<uintptr_t>(cs_handle_));

    cs_insn* insn = nullptr;
    std::size_t count = cs_disasm(handle, code.data(), code.size(), base_addr, 0, &insn);

    std::vector<DisasmChunk> chunks;
    if (count == 0) return chunks;

    std::size_t i = 0;
    while (i < count) {
        DisasmChunk chunk;
        chunk.start_addr = insn[i].address;
        chunk.insn_count = 0;

        std::ostringstream oss;
        for (std::size_t j = 0; j < chunk_size && i < count; ++j, ++i) {
            oss << "0x" << std::hex << insn[i].address << ":\t"
                << insn[i].mnemonic << "\t" << insn[i].op_str << "\n";
            ++chunk.insn_count;
        }
        chunk.text = oss.str();
        chunks.push_back(std::move(chunk));
    }

    cs_free(insn, count);
    return chunks;
}
