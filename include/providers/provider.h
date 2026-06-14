#pragma once
#include <string>
#include <memory>
#include "config/config.h"

class Provider {
public:
    virtual ~Provider() = default;

    // Send a prompt, return the AI response text.
    virtual std::string generate(const std::string& prompt) = 0;

    virtual std::string name() const = 0;

    static std::unique_ptr<Provider> create(const ProviderConfig& cfg);
};
