#include "inference/InferenceSession.hpp"
#include "inference/ModelManager.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

namespace bonsai {
namespace inference {

InferenceSession::InferenceSession() {
    auto& manager = ModelManager::getInstance();
    if (manager.isInitialized()) {
        auto* engine = manager.getEngine();
        LiteRtLmConversationConfig* config = litert_lm_conversation_config_create(
            engine, nullptr, nullptr, nullptr, nullptr, false);
        
        m_conversation = litert_lm_conversation_create(engine, config);
        litert_lm_conversation_config_delete(config);
    }
}

InferenceSession::~InferenceSession() {
    if (m_conversation) {
        litert_lm_conversation_delete(m_conversation);
    }
}

std::string InferenceSession::predict(const std::vector<Message>& messages, const SamplerSettings& settings) {
    if (!m_conversation) return "Error: Conversation not initialized.";

    json conv_msg = json::array();
    for (const auto& m : messages) {
        conv_msg.push_back({{"role", m.role}, {"content", m.content}});
    }

    std::string msg_str = conv_msg.dump();
    
    // Note: Sampler settings are not directly supported in the simple C SendMessage call yet,
    // they usually go through optional_args_json if supported.
    
    LiteRtLmJsonResponse* response = litert_lm_conversation_send_message(m_conversation, msg_str.c_str(), nullptr);
    if (!response) {
        return "Error: Inference failed.";
    }

    std::string result = litert_lm_json_response_get_string(response);
    litert_lm_json_response_delete(response);

    try {
        json j = json::parse(result);
        if (j.contains("content") && j["content"].is_array()) {
            std::string text;
            for (const auto& item : j["content"]) {
                if (item.contains("text")) {
                    text += item["text"].get<std::string>();
                }
            }
            return text;
        }
    } catch (...) {}

    return result;
}

void InferenceSession::predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings) {
    if (!m_conversation) {
        callback("Error: Conversation not initialized.", true);
        return;
    }

    json conv_msg = json::array();
    for (const auto& m : messages) {
        conv_msg.push_back({{"role", m.role}, {"content", m.content}});
    }

    std::string msg_str = conv_msg.dump();

    auto status = litert_lm_conversation_send_message_stream(
        m_conversation, msg_str.c_str(), nullptr,
        [](void* data, const char* chunk, bool is_final, const char* error_msg) {
            auto* cb = static_cast<TokenCallback*>(data);
            if (error_msg) {
                (*cb)(std::string("Error: ") + error_msg, true);
                delete cb;
                return;
            }
            if (chunk) {
                try {
                    json j = json::parse(chunk);
                    if (j.contains("content") && j["content"].is_array()) {
                        std::string text;
                        for (const auto& item : j["content"]) {
                            if (item.contains("text")) {
                                text += item["text"].get<std::string>();
                            }
                        }
                        if (!text.empty()) {
                            (*cb)(text, false);
                        }
                    }
                } catch (...) {
                    // Raw chunk fallback
                }
            }
            if (is_final) {
                (*cb)("", true);
                delete cb;
            }
        },
        new TokenCallback(callback)
    );

    if (status != 0) {
        callback("Error starting stream.", true);
    }
}

} // namespace inference
} // namespace bonsai
