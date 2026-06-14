#pragma once
#include "providers/provider.h"

class GeminiProvider : public Provider {
public:
    explicit GeminiProvider(const ProviderConfig& cfg);
    std::string generate(const std::string& prompt) override;
    std::string name() const override { return "gemini"; }

private:
    ProviderConfig cfg_;
};
