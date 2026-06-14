#include "output/output.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;

Output::Output(const OutputConfig& cfg) : cfg_(cfg) {}

static std::string ext_for_lang(const std::string& lang) {
    if (lang == "cpp" || lang == "c++") return ".cpp";
    if (lang == "rust")                  return ".rs";
    if (lang == "python" || lang == "py") return ".py";
    return ".c";
}

static std::string lang_display(const std::string& lang) {
    if (lang == "cpp" || lang == "c++") return "C++";
    if (lang == "rust")                  return "Rust";
    if (lang == "python" || lang == "py") return "Python";
    return "C";
}

// Extract fenced code blocks from AI response.  If none found, treat entire
// response as one block.
static std::vector<std::string> extract_code_blocks(const std::string& text) {
    std::vector<std::string> blocks;
    std::regex fence_re(R"(```(?:\w+)?\s*\n([\s\S]*?)```)");
    auto begin = std::sregex_iterator(text.begin(), text.end(), fence_re);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it)
        blocks.push_back((*it)[1].str());

    if (blocks.empty())
        blocks.push_back(text);
    return blocks;
}

std::vector<GeneratedFile> Output::parse_chunks(const std::vector<std::string>& chunks,
                                                  const std::string& stem,
                                                  const std::string& language) const {
    std::vector<GeneratedFile> files;
    std::string ext = ext_for_lang(language);
    std::ostringstream combined;

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        auto blocks = extract_code_blocks(chunks[i]);
        for (const auto& blk : blocks)
            combined << blk << "\n\n";
    }

    files.push_back({stem + ext, combined.str()});
    return files;
}

void Output::write_files(const std::vector<GeneratedFile>& files) const {
    fs::create_directories(cfg_.out_dir);
    for (const auto& f : files) {
        fs::path p = fs::path(cfg_.out_dir) / f.filename;
        std::ofstream ofs(p);
        if (!ofs) throw std::runtime_error("Cannot write: " + p.string());
        ofs << f.content;
        std::cout << "  wrote " << p.string() << "\n";
    }
}

void Output::write_build_system(const std::string& stem,
                                 const std::vector<GeneratedFile>& files,
                                 const std::string& language) const {
    if (language == "rust") {
        // Generate Cargo.toml
        std::string cargo = "[package]\nname = \"" + stem + "\"\nversion = \"0.1.0\"\nedition = \"2021\"\n\n[dependencies]\n";
        fs::path p = fs::path(cfg_.out_dir) / "Cargo.toml";
        std::ofstream f(p);
        f << cargo;
        std::cout << "  wrote " << p.string() << "\n";

        fs::create_directories(fs::path(cfg_.out_dir) / "src");
        // Move main file to src/
        for (const auto& gf : files) {
            fs::path src = fs::path(cfg_.out_dir) / gf.filename;
            fs::path dst = fs::path(cfg_.out_dir) / "src" / "main.rs";
            if (fs::exists(src)) fs::rename(src, dst);
        }
        return;
    }

    if (language == "python" || language == "py") return; // no build needed

    // C/C++ → CMakeLists.txt
    std::string lang_name = (language == "cpp" || language == "c++") ? "CXX" : "C";
    std::string std_ver   = (language == "cpp" || language == "c++") ? "17" : "11";

    std::ostringstream cmake;
    cmake << "cmake_minimum_required(VERSION 3.16)\n"
          << "project(" << stem << " " << lang_name << ")\n\n"
          << "set(CMAKE_" << lang_name << "_STANDARD " << std_ver << ")\n\n"
          << "add_executable(" << stem << "\n";
    for (const auto& gf : files)
        cmake << "    " << gf.filename << "\n";
    cmake << ")\n";

    fs::path p = fs::path(cfg_.out_dir) / "CMakeLists.txt";
    std::ofstream f(p);
    f << cmake.str();
    std::cout << "  wrote " << p.string() << "\n";
}

void Output::write(const std::string& binary_name,
                    const std::vector<std::string>& ai_chunks,
                    const std::string& language) {
    // Strip path and extension from binary name for use as file stem
    fs::path bp(binary_name);
    std::string stem = bp.stem().string();
    if (stem.empty()) stem = "decompiled";

    if (cfg_.stdout_only) {
        for (const auto& chunk : ai_chunks)
            std::cout << chunk << "\n";
        return;
    }

    auto files = parse_chunks(ai_chunks, stem, language);
    write_files(files);
    write_build_system(stem, files, language);

    std::cout << "\nOutput written to " << cfg_.out_dir << "/ ("
              << lang_display(language) << ")\n";
}
