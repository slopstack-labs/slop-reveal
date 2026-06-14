#include "cache/cache.h"
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

Cache::Cache(const std::string& cache_dir) : cache_dir_(cache_dir) {}

std::string Cache::hash_binary(const std::vector<uint8_t>& data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), digest);

    std::ostringstream oss;
    for (auto b : digest)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

std::string Cache::cache_path(const std::string& binary_hash,
                               const std::string& provider,
                               std::size_t chunk_index) const {
    return (fs::path(cache_dir_) /
            (binary_hash + "_" + provider + "_" + std::to_string(chunk_index) + ".txt")).string();
}

std::optional<std::string> Cache::get(const std::string& binary_hash,
                                       const std::string& provider,
                                       std::size_t chunk_index) const {
    if (!enabled_) return std::nullopt;

    std::string p = cache_path(binary_hash, provider, chunk_index);
    std::ifstream f(p);
    if (!f) return std::nullopt;

    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

void Cache::put(const std::string& binary_hash,
                const std::string& provider,
                std::size_t chunk_index,
                const std::string& result) {
    if (!enabled_) return;

    fs::create_directories(cache_dir_);
    std::string p = cache_path(binary_hash, provider, chunk_index);
    std::ofstream f(p, std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot write cache file: " + p);
    f << result;
}
