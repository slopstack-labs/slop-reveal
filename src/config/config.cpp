#include "config/config.h"
#define TOML_EXCEPTIONS 0
#include <toml.hpp>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <getopt.h>

static const char* HELP_TEXT = R"(
Usage: slopreveal [OPTIONS] <binary>

Options:
  -b, --binary <path>       Input binary file (can also be positional arg)
  -p, --provider <name>     AI provider: claude, ollama, openai, gemini  [default: claude]
  -m, --model <name>        Model name for the provider
  -k, --api-key <key>       API key (overrides env/config)
  -u, --base-url <url>      Base URL for provider (e.g. http://localhost:11434 for Ollama)
  -a, --arch <arch>         Architecture: auto, x86, x64, arm, arm64, mips  [default: auto]
  -l, --language <lang>     Output language: c, cpp, rust, python  [default: c]
  -o, --output <dir>        Output directory  [default: out]
      --stdout              Print generated code to stdout only
  -C, --config <path>       TOML config file  [default: slopreveal.toml]
      --chunk-size <n>      Instructions per AI request chunk  [default: 200]
      --no-cache            Disable result cache
      --cache-dir <dir>     Cache directory  [default: .slopreveal_cache]
  -t, --tui                 Launch interactive TUI mode
  -v, --verbose             Verbose output
  -h, --help                Show this help

Environment variables:
  SLOPREVEAL_API_KEY        API key (any provider)
  SLOPREVEAL_PROVIDER       Provider name
  SLOPREVEAL_MODEL          Model name
  ANTHROPIC_API_KEY         Claude API key (auto-detected)
  OPENAI_API_KEY            OpenAI API key (auto-detected)
  GEMINI_API_KEY            Gemini API key (auto-detected)

Config file (TOML):
  Loaded from --config path or ./slopreveal.toml if present.
  Priority: CLI flags > env vars > config file > defaults.
)";

static const struct option LONG_OPTS[] = {
    {"binary",     required_argument, nullptr, 'b'},
    {"provider",   required_argument, nullptr, 'p'},
    {"model",      required_argument, nullptr, 'm'},
    {"api-key",    required_argument, nullptr, 'k'},
    {"base-url",   required_argument, nullptr, 'u'},
    {"arch",       required_argument, nullptr, 'a'},
    {"language",   required_argument, nullptr, 'l'},
    {"output",     required_argument, nullptr, 'o'},
    {"config",     required_argument, nullptr, 'C'},
    {"chunk-size", required_argument, nullptr, 'S'},
    {"cache-dir",  required_argument, nullptr, 'D'},
    {"stdout",     no_argument,       nullptr, 's'},
    {"no-cache",   no_argument,       nullptr, 'n'},
    {"tui",        no_argument,       nullptr, 't'},
    {"verbose",    no_argument,       nullptr, 'v'},
    {"help",       no_argument,       nullptr, 'h'},
    {nullptr, 0, nullptr, 0}
};
static const char* SHORT_OPTS = "b:p:m:k:u:a:l:o:C:S:D:sntvh";

void Config::print_help(const char* prog) {
    std::cout << "slopreveal - decompile binaries with AI\n" << HELP_TEXT;
    (void)prog;
}

void Config::apply_toml(AppConfig& cfg, const std::string& path) {
    auto result = toml::parse_file(path);
    if (!result) return; // file not found or parse error — silently skip

    auto& tbl = result.table();

    if (auto v = tbl["provider"]["name"].value<std::string>())        cfg.provider.name = *v;
    if (auto v = tbl["provider"]["api_key"].value<std::string>())     cfg.provider.api_key = *v;
    if (auto v = tbl["provider"]["model"].value<std::string>())       cfg.provider.model = *v;
    if (auto v = tbl["provider"]["base_url"].value<std::string>())    cfg.provider.base_url = *v;
    if (auto v = tbl["provider"]["timeout"].value<int64_t>())         cfg.provider.timeout_seconds = static_cast<int>(*v);

    if (auto v = tbl["cache"]["enabled"].value<bool>())               cfg.cache.enabled = *v;
    if (auto v = tbl["cache"]["dir"].value<std::string>())            cfg.cache.cache_dir = *v;

    if (auto v = tbl["output"]["dir"].value<std::string>())           cfg.output.out_dir = *v;
    if (auto v = tbl["output"]["language"].value<std::string>())      cfg.output.language = *v;

    if (auto v = tbl["disasm"]["arch"].value<std::string>())          cfg.disasm.arch = *v;
    if (auto v = tbl["disasm"]["chunk_size"].value<int64_t>())        cfg.disasm.chunk_size = static_cast<std::size_t>(*v);
}

void Config::apply_env(AppConfig& cfg) {
    auto env = [](const char* name) -> std::string {
        const char* v = std::getenv(name);
        return v ? std::string(v) : "";
    };

    if (auto v = env("SLOPREVEAL_PROVIDER"); !v.empty()) cfg.provider.name = v;
    if (auto v = env("SLOPREVEAL_MODEL");    !v.empty()) cfg.provider.model = v;

    // Unified key takes priority; provider-specific keys are fallbacks.
    if (auto v = env("SLOPREVEAL_API_KEY"); !v.empty()) {
        cfg.provider.api_key = v;
    } else if (cfg.provider.name == "claude" && !env("ANTHROPIC_API_KEY").empty()) {
        cfg.provider.api_key = env("ANTHROPIC_API_KEY");
    } else if (cfg.provider.name == "openai" && !env("OPENAI_API_KEY").empty()) {
        cfg.provider.api_key = env("OPENAI_API_KEY");
    } else if (cfg.provider.name == "gemini" && !env("GEMINI_API_KEY").empty()) {
        cfg.provider.api_key = env("GEMINI_API_KEY");
    } else if (cfg.provider.api_key.empty()) {
        // Best-effort: pick whichever provider key is set
        for (auto [prov, key] : std::initializer_list<std::pair<const char*, const char*>>{
                {"claude", "ANTHROPIC_API_KEY"},
                {"openai", "OPENAI_API_KEY"},
                {"gemini", "GEMINI_API_KEY"}}) {
            auto v2 = env(key);
            if (!v2.empty()) { cfg.provider.api_key = v2; break; }
        }
    }
}

// One-pass CLI parse; writes into cfg. Does NOT call print_help/exit — caller does.
static void parse_cli_into(AppConfig& cfg, int argc, char** argv) {
    int opt, idx = 0;
    optind = 1;
    while ((opt = getopt_long(argc, argv, SHORT_OPTS, LONG_OPTS, &idx)) != -1) {
        switch (opt) {
            case 'b': cfg.binary_path = optarg; break;
            case 'p': cfg.provider.name = optarg; break;
            case 'm': cfg.provider.model = optarg; break;
            case 'k': cfg.provider.api_key = optarg; break;
            case 'u': cfg.provider.base_url = optarg; break;
            case 'a': cfg.disasm.arch = optarg; break;
            case 'l': cfg.output.language = optarg; break;
            case 'o': cfg.output.out_dir = optarg; break;
            case 'S': cfg.disasm.chunk_size = static_cast<std::size_t>(std::stoul(optarg)); break;
            case 'D': cfg.cache.cache_dir = optarg; break;
            case 's': cfg.output.stdout_only = true; break;
            case 'n': cfg.cache.enabled = false; break;
            case 't': cfg.interactive = true; break;
            case 'v': cfg.output.verbose = true; break;
            case 'h': Config::print_help(argv[0]); std::exit(0);
            case 'C': break; // handled separately
            default:  break;
        }
    }
    if (cfg.binary_path.empty() && optind < argc)
        cfg.binary_path = argv[optind];
}

// Extract --config / -C value from argv before any other parsing.
static std::string find_config_path(int argc, char** argv) {
    int opt, idx = 0;
    optind = 1;
    while ((opt = getopt_long(argc, argv, SHORT_OPTS, LONG_OPTS, &idx)) != -1)
        if (opt == 'C') return optarg;
    return "slopreveal.toml";
}

void Config::apply_cli(AppConfig& cfg, int argc, char** argv) {
    parse_cli_into(cfg, argc, argv);
}

void Config::validate(const AppConfig& cfg) {
    if (cfg.binary_path.empty())
        throw std::runtime_error(
            "No binary file specified. Use -b <path> or pass it as a positional argument.");

    if (cfg.provider.name.empty())
        throw std::runtime_error(
            "No provider specified. Use -p claude|ollama|openai|gemini.");

    if (cfg.provider.name != "ollama" && cfg.provider.api_key.empty())
        throw std::runtime_error(
            "No API key for provider '" + cfg.provider.name + "'. "
            "Set via -k, SLOPREVEAL_API_KEY, or the provider-specific env var.");
}

AppConfig Config::load(int argc, char** argv) {
    // Defaults
    AppConfig cfg;
    cfg.provider.name = "claude";

    // Priority: defaults → TOML → ENV → CLI
    std::string config_path = find_config_path(argc, argv);
    apply_toml(cfg, config_path);
    apply_env(cfg);
    apply_cli(cfg, argc, argv);

    validate(cfg);
    return cfg;
}
