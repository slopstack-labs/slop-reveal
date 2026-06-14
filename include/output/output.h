#pragma once
#include <string>
#include <vector>
#include "config/config.h"

struct GeneratedFile {
    std::string filename;
    std::string content;
};

class Output {
public:
    explicit Output(const OutputConfig& cfg);

    // Parse AI response chunks into source files and write them to out_dir.
    void write(const std::string& binary_name,
               const std::vector<std::string>& ai_chunks,
               const std::string& language);

private:
    std::vector<GeneratedFile> parse_chunks(const std::vector<std::string>& chunks,
                                            const std::string& stem,
                                            const std::string& language) const;
    void write_files(const std::vector<GeneratedFile>& files) const;
    void write_build_system(const std::string& stem,
                            const std::vector<GeneratedFile>& files,
                            const std::string& language) const;

    OutputConfig cfg_;
};
