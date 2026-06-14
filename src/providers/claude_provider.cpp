#include "providers/claude_provider.h"
#include "http/http_client.h"
#include <json.hpp>
#include <stdexcept>

using json = nlohmann::json;

ClaudeProvider::ClaudeProvider(const ProviderConfig& cfg) : cfg_(cfg) {
    if (cfg_.model.empty())
        cfg_.model = "claude-sonnet-4-6";
    if (cfg_.base_url.empty())
        cfg_.base_url = "https://api.anthropic.com";
}

std::string ClaudeProvider::generate(const std::string& prompt) {
    HttpClient http(cfg_.timeout_seconds);

    json body = {
        {"model", cfg_.model},
        {"max_tokens", 8192},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })}
    };

    auto resp = http.post(
        cfg_.base_url + "/v1/messages",
        body.dump(),
        {
            {"Content-Type",      "application/json"},
            {"x-api-key",         cfg_.api_key},
            {"anthropic-version", "2023-06-01"}
        }
    );

    if (resp.status_code != 200) {
        throw std::runtime_error("Claude API error " + std::to_string(resp.status_code) +
                                 ": " + resp.body);
    }

    auto j = json::parse(resp.body);
    return j["content"][0]["text"].get<std::string>();
}
