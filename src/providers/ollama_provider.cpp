#include "providers/ollama_provider.h"
#include "http/http_client.h"
#include <json.hpp>
#include <stdexcept>

using json = nlohmann::json;

OllamaProvider::OllamaProvider(const ProviderConfig& cfg) : cfg_(cfg) {
    if (cfg_.model.empty())
        cfg_.model = "codellama";
    if (cfg_.base_url.empty())
        cfg_.base_url = "http://localhost:11434";
}

std::string OllamaProvider::generate(const std::string& prompt) {
    HttpClient http(cfg_.timeout_seconds);

    json body = {
        {"model",  cfg_.model},
        {"prompt", prompt},
        {"stream", false}
    };

    auto resp = http.post(
        cfg_.base_url + "/api/generate",
        body.dump(),
        {{"Content-Type", "application/json"}}
    );

    if (resp.status_code != 200) {
        throw std::runtime_error("Ollama API error " + std::to_string(resp.status_code) +
                                 ": " + resp.body);
    }

    auto j = json::parse(resp.body);
    return j["response"].get<std::string>();
}
