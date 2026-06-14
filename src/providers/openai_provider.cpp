#include "providers/openai_provider.h"
#include "http/http_client.h"
#include <json.hpp>
#include <stdexcept>

using json = nlohmann::json;

OpenAIProvider::OpenAIProvider(const ProviderConfig& cfg) : cfg_(cfg) {
    if (cfg_.model.empty())
        cfg_.model = "gpt-4o";
    if (cfg_.base_url.empty())
        cfg_.base_url = "https://api.openai.com";
}

std::string OpenAIProvider::generate(const std::string& prompt) {
    HttpClient http(cfg_.timeout_seconds);

    json body = {
        {"model", cfg_.model},
        {"max_tokens", 8192},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })}
    };

    auto resp = http.post(
        cfg_.base_url + "/v1/chat/completions",
        body.dump(),
        {
            {"Content-Type",  "application/json"},
            {"Authorization", "Bearer " + cfg_.api_key}
        }
    );

    if (resp.status_code != 200) {
        throw std::runtime_error("OpenAI API error " + std::to_string(resp.status_code) +
                                 ": " + resp.body);
    }

    auto j = json::parse(resp.body);
    return j["choices"][0]["message"]["content"].get<std::string>();
}
