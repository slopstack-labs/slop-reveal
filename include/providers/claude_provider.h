#pragma once
#include "providers/provider.h"

class ClaudeProvider : public Provider {
public:
    explicit ClaudeProvider(const ProviderConfig& cfg);
    std::string generate(const std::string& prompt) override;
    std::string name() const override { return "claude"; }

private:
    ProviderConfig cfg_;
};
