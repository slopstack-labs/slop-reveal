#pragma once
#include <string>
#include <optional>
#include <vector>
#include <cstdint>

class Cache {
public:
    explicit Cache(const std::string& cache_dir);

    // Hash the binary bytes with SHA-256, return hex string.
    static std::string hash_binary(const std::vector<uint8_t>& data);

    // Key is derived from binary_hash + provider_name + chunk_index.
    std::optional<std::string> get(const std::string& binary_hash,
                                   const std::string& provider,
                                   std::size_t chunk_index) const;

    void put(const std::string& binary_hash,
             const std::string& provider,
             std::size_t chunk_index,
             const std::string& result);

    bool is_enabled() const { return enabled_; }
    void set_enabled(bool v) { enabled_ = v; }

private:
    std::string cache_path(const std::string& binary_hash,
                           const std::string& provider,
                           std::size_t chunk_index) const;

    std::string cache_dir_;
    bool enabled_ = true;
};
