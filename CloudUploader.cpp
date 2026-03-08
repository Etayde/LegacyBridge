#include "CloudUploader.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "httplib.h"


void CloudUploader::start() {
    std::cout << "[Uploader] Started. Waiting for data..." << std::endl;
    OrderInfo record;

    while (true) {
        if (q.pop(record)) {
            json payload = {
                {"timestamp", record.timestamp},
                {"table_id", record.table_id},
                {"item_name", record.item_name},
                {"category", record.category},
                {"price", record.price}
            };

            std::string json_string = payload.dump();
            sendToCloudWithRetry(json_string, record.item_name);
        }
    }
}

void CloudUploader::sendToCloudWithRetry(const std::string& payload, const std::string& item_name) {
    httplib::SSLClient cli(supabase_url);
    
    httplib::Headers headers = {
        {"apikey", api_key},
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"}
    };

    bool success = false;
    int max_retries = 5;
    int current_try = 0;

    while (!success && current_try < max_retries) {
        current_try++;
        
        auto res = cli.Post("/rest/v1/orders", headers, payload, "application/json");

        if (res && (res->status == 201 || res->status == 200)) {
            std::cout << "[Uploader] SUCCESS: Sent " << item_name << " to cloud." << std::endl;
            success = true;
        } else {
            int status = res ? res->status : 0;
            std::cerr << "[Uploader] ERROR " << status << ". Attempt " 
                        << current_try << "/" << max_retries << " failed. Retrying in 3s..." << std::endl;
            
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    if (!success) {
        std::cerr << "[Uploader] CRITICAL: Failed to send data after " << max_retries << " attempts. Data dropped." << std::endl;
    }
}