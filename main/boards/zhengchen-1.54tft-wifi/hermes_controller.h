#ifndef __HERMES_CONTROLLER_H__
#define __HERMES_CONTROLLER_H__

#include "mcp_server.h"
#include "settings.h"
#include <esp_http_client.h>
#include <esp_log.h>
#include <cJSON.h>

#define HERMES_TAG "Hermes_Controller"

class HermesController {
private:
    std::string bridge_url_;

    std::string MakeBridgeRequest(const std::string& endpoint, const std::string& body) {
        std::string url = bridge_url_ + endpoint;
        
        esp_http_client_config_t config = {};
        config.url = url.c_str();
        config.method = HTTP_METHOD_POST;
        config.timeout_ms = 15000;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, body.c_str(), body.length());

        esp_err_t err = esp_http_client_perform(client);
        std::string result;
        
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            if (status_code == 200) {
                char buffer[2048];
                int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
                if (len > 0) {
                    buffer[len] = '\0';
                    result = std::string(buffer, len);
                }
            }
            ESP_LOGI(HERMES_TAG, "Bridge %s -> %d", endpoint.c_str(), status_code);
        } else {
            ESP_LOGE(HERMES_TAG, "Bridge request failed: %s", esp_err_to_name(err));
            result = "{\"error\": \"bridge_unreachable\"}";
        }
        
        esp_http_client_cleanup(client);
        return result;
    }

public:
    HermesController() {
        // Load bridge URL from NVS
        Settings settings("hermes", true);
        bridge_url_ = settings.GetString("url", "http://192.168.0.195:4567");
        
        auto& mcp_server = McpServer::GetInstance();
        
        // Hermes query - send text query to Hermes AI via bridge
        mcp_server.AddTool("hermes.query", "Send a query to Hermes AI agent and get a response",
            PropertyList({Property("query", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string query = properties["query"].value<std::string>();
                std::string body = "{\"query\": \"" + query + "\"}";
                std::string result = MakeBridgeRequest("/hermes/query", body);
                return result;
            });

        // Hermes status - check Hermes bridge status
        mcp_server.AddTool("hermes.get_status", "Get Hermes AI agent bridge status",
            PropertyList(),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string result = MakeBridgeRequest("/hermes/status", "{}");
                return result;
            });

        ESP_LOGI(HERMES_TAG, "Hermes Controller initialized. Bridge: %s", bridge_url_.c_str());
    }
};

#endif // __HERMES_CONTROLLER_H__
