#pragma once
#include "providers/provider.h"

class OpenAIProvider : public Provider {
public:
    explicit OpenAIProvider(const ProviderConfig& cfg);
    std::string generate(const std::string& prompt) override;
    std::string name() const override { return "openai"; }

private:
    ProviderConfig cfg_;
};
