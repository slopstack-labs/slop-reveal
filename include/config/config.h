#pragma once
#include <string>
#include <cstddef>

struct ProviderConfig {
    std::string name;
    std::string api_key;
    std::string model;
    std::string base_url;
    int timeout_seconds = 120;
};

struct CacheConfig {
    bool enabled = true;
    std::string cache_dir = ".slopreveal_cache";
};

struct OutputConfig {
    std::string out_dir = "out";
    std::string language = "c";
    bool stdout_only = false;
    bool verbose = false;
};

struct DisasmConfig {
    std::string arch = "auto";
    std::size_t chunk_size = 200;
};

struct AppConfig {
    std::string binary_path;
    ProviderConfig provider;
    CacheConfig cache;
    OutputConfig output;
    DisasmConfig disasm;
    bool interactive = false;
    bool pessimistic = false;
};

class Config {
public:
    static AppConfig load(int argc, char** argv);

    static void print_help(const char* prog);

private:
    static void apply_toml(AppConfig& cfg, const std::string& path);
    static void apply_env(AppConfig& cfg);
    static void apply_cli(AppConfig& cfg, int argc, char** argv);
    static void validate(const AppConfig& cfg);
};
