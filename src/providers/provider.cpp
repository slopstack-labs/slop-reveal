#include "providers/provider.h"
#include "providers/claude_provider.h"
#include "providers/ollama_provider.h"
#include "providers/openai_provider.h"
#include "providers/gemini_provider.h"
#include <stdexcept>
#include <algorithm>

std::unique_ptr<Provider> Provider::create(const ProviderConfig& cfg) {
    std::string name = cfg.name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "claude")  return std::make_unique<ClaudeProvider>(cfg);
    if (name == "ollama")  return std::make_unique<OllamaProvider>(cfg);
    if (name == "openai")  return std::make_unique<OpenAIProvider>(cfg);
    if (name == "gemini")  return std::make_unique<GeminiProvider>(cfg);

    throw std::runtime_error("Unknown provider: " + cfg.name +
                             ". Supported: claude, ollama, openai, gemini");
}
