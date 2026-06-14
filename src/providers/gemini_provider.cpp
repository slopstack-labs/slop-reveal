#include "providers/gemini_provider.h"
#include "http/http_client.h"
#include <json.hpp>
#include <stdexcept>

using json = nlohmann::json;

GeminiProvider::GeminiProvider(const ProviderConfig& cfg) : cfg_(cfg) {
    if (cfg_.model.empty())
        cfg_.model = "gemini-2.0-flash";
    if (cfg_.base_url.empty())
        cfg_.base_url = "https://generativelanguage.googleapis.com";
}

std::string GeminiProvider::generate(const std::string& prompt) {
    HttpClient http(cfg_.timeout_seconds);

    json body = {
        {"contents", json::array({
            {{"parts", json::array({
                {{"text", prompt}}
            })}}
        })}
    };

    std::string url = cfg_.base_url + "/v1beta/models/" + cfg_.model +
                      ":generateContent?key=" + cfg_.api_key;

    auto resp = http.post(url, body.dump(), {{"Content-Type", "application/json"}});

    if (resp.status_code != 200) {
        throw std::runtime_error("Gemini API error " + std::to_string(resp.status_code) +
                                 ": " + resp.body);
    }

    auto j = json::parse(resp.body);
    return j["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
}
