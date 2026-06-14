#pragma once
#include "providers/provider.h"

class OllamaProvider : public Provider {
public:
    explicit OllamaProvider(const ProviderConfig& cfg);
    std::string generate(const std::string& prompt) override;
    std::string name() const override { return "ollama"; }

private:
    ProviderConfig cfg_;
};
