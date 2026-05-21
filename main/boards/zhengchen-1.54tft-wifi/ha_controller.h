#ifndef __HA_CONTROLLER_H__
#define __HA_CONTROLLER_H__

#include "mcp_server.h"
#include "settings.h"
#include <esp_http_client.h>
#include <esp_log.h>
#include <cJSON.h>

#define HA_TAG "HA_Controller"

class HaController {
private:
    std::string ha_url_;
    std::string ha_token_;

    static HaController* instance_;

    std::string GetApiUrl(const std::string& path) {
        return ha_url_ + "/api/" + path;
    }

    std::string MakeHaRequest(const std::string& method, const std::string& path, const std::string& body = "") {
        std::string url = GetApiUrl(path);
        
        esp_http_client_config_t config = {};
        config.url = url.c_str();
        config.method = (method == "POST") ? HTTP_METHOD_POST : HTTP_METHOD_GET;
        config.timeout_ms = 5000;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        
        // Set headers
        esp_http_client_set_header(client, "Authorization", ("Bearer " + ha_token_).c_str());
        esp_http_client_set_header(client, "Content-Type", "application/json");
        
        if (!body.empty()) {
            esp_http_client_set_post_field(client, body.c_str(), body.length());
        }

        esp_err_t err = esp_http_client_perform(client);
        std::string result;
        
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            if (status_code == 200 || status_code == 201) {
                char buffer[1024];
                int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
                if (len > 0) {
                    buffer[len] = '\0';
                    result = std::string(buffer, len);
                }
            }
            ESP_LOGI(HA_TAG, "HA API %s %s -> %d", method.c_str(), path.c_str(), status_code);
        } else {
            ESP_LOGE(HA_TAG, "HA API request failed: %s", esp_err_to_name(err));
            result = "{\"error\": \"request_failed\"}";
        }
        
        esp_http_client_cleanup(client);
        return result;
    }

public:
    HaController() {
        // Load HA config from NVS
        Settings settings("ha", true);
        ha_url_ = settings.GetString("url", "http://192.168.0.196:8123");
        ha_token_ = settings.GetString("token", "");
        
        auto& mcp_server = McpServer::GetInstance();
        
        // HA Light tools
        mcp_server.AddTool("ha.light.turn_on", "Turn on a Home Assistant light/switch by entity_id", 
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/light/turn_on", body);
                return result;
            });

        mcp_server.AddTool("ha.light.turn_off", "Turn off a Home Assistant light/switch by entity_id",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/light/turn_off", body);
                return result;
            });

        mcp_server.AddTool("ha.light.toggle", "Toggle a Home Assistant light/switch by entity_id",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/light/toggle", body);
                return result;
            });

        mcp_server.AddTool("ha.light.get_state", "Get the current state of a Home Assistant entity",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string result = MakeHaRequest("GET", "states/" + entity_id);
                return result;
            });

        // HA Switch tools
        mcp_server.AddTool("ha.switch.turn_on", "Turn on a Home Assistant switch by entity_id",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/switch/turn_on", body);
                return result;
            });

        mcp_server.AddTool("ha.switch.turn_off", "Turn off a Home Assistant switch by entity_id",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/switch/turn_off", body);
                return result;
            });

        // HA Cover tools  
        mcp_server.AddTool("ha.cover.open", "Open a Home Assistant cover/curtain by entity_id",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/cover/open_cover", body);
                return result;
            });

        mcp_server.AddTool("ha.cover.close", "Close a Home Assistant cover/curtain by entity_id",
            PropertyList({Property("entity_id", kPropertyTypeString)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\"}";
                std::string result = MakeHaRequest("POST", "services/cover/close_cover", body);
                return result;
            });

        // HA Climate tools
        mcp_server.AddTool("ha.climate.set_temperature", "Set the target temperature of a Home Assistant climate entity",
            PropertyList({Property("entity_id", kPropertyTypeString), Property("temperature", kPropertyTypeInteger, 22, 16, 30)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string entity_id = properties["entity_id"].value<std::string>();
                int temp = properties["temperature"].value<int>();
                std::string body = "{\"entity_id\": \"" + entity_id + "\", \"temperature\": " + std::to_string(temp) + "}";
                std::string result = MakeHaRequest("POST", "services/climate/set_temperature", body);
                return result;
            });

        ESP_LOGI(HA_TAG, "HA Controller initialized. HA URL: %s", ha_url_.c_str());
    }
};

#endif // __HA_CONTROLLER_H__
