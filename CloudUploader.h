#pragma once
#include "utils.h"
#include <nlohmann/json.hpp>


using json = nlohmann::json;

class CloudUploader {
private:
    Buffer& q;
    std::string supabase_url;
    std::string api_key;

    void sendToCloudWithRetry(const std::string& payload, const std::string& item_name);

public:
    CloudUploader(Buffer& q, const std::string& url, const std::string& key)
        : q(q), supabase_url(url), api_key(key) {}

    
    void start();
};