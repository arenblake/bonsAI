#include "inference/InferenceSession.hpp"
#include "inference/ModelManager.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

using json = nlohmann::ordered_json;

namespace bonsai {
namespace inference {

// Struct to hold callback data and keep the session alive
struct AsyncCallbackData {
    TokenCallback callback;
    std::shared_ptr<InferenceSession> session;
};

InferenceSession::InferenceSession() {
}

InferenceSession::~InferenceSession() {
    if (m_conversation) {
        litert_lm_conversation_delete(m_conversation);
        m_conversation = nullptr;
    }
    if (m_hasLock) {
        ModelManager::getInstance().releaseInferenceLock();
        m_hasLock = false;
    }
}

bool InferenceSession::init(const std::string& toolsJson, const std::string& historyJson, const SamplerSettings& settings) {
    auto& manager = ModelManager::getInstance();
    if (!manager.isInitialized()) return false;

    if (!m_hasLock) {
        manager.acquireInferenceLock();
        m_hasLock = true;
    }

    auto* engine = manager.getEngine();
    
    LiteRtLmSessionConfig* session_config = litert_lm_session_config_create();
    if (session_config) {
        LiteRtLmSamplerParams params;
        params.type = kLiteRtLmSamplerTypeTopP;
        params.temperature = settings.temperature;
        params.top_p = settings.top_p;
        params.top_k = 40;
        params.seed = 0;
        litert_lm_session_config_set_sampler_params(session_config, &params);
    }

    LiteRtLmConversationConfig* config = litert_lm_conversation_config_create();
    if (!config) {
        if (session_config) litert_lm_session_config_delete(session_config);
        if (m_hasLock) {
            manager.releaseInferenceLock();
            m_hasLock = false;
        }
        return false;
    }

    if (session_config) {
        litert_lm_conversation_config_set_session_config(config, session_config);
        litert_lm_session_config_delete(session_config);
    }

    m_toolsJson = toolsJson;
    if (!m_toolsJson.empty()) {
        litert_lm_conversation_config_set_tools(config, m_toolsJson.c_str());
        litert_lm_conversation_config_set_enable_constrained_decoding(config, false);
    }
    
    if (!historyJson.empty()) {
        litert_lm_conversation_config_set_messages(config, historyJson.c_str());
    }

    m_conversation = litert_lm_conversation_create(engine, config);
    litert_lm_conversation_config_delete(config);
    
    if (!m_conversation) {
        if (m_hasLock) {
            manager.releaseInferenceLock();
            m_hasLock = false;
        }
    }

    return m_conversation != nullptr;
}

nlohmann::json InferenceSession::predict(const std::vector<Message>& messages, const SamplerSettings& settings, const std::string& toolsJson) {
    nlohmann::ordered_json history = nlohmann::ordered_json::array();
    for (const auto& msg : messages) {
        nlohmann::ordered_json m = nlohmann::ordered_json::object();
        m["role"] = msg.role;
        m["content"] = msg.content;
        if (!msg.tool_call_id.empty()) m["tool_call_id"] = msg.tool_call_id;
        if (!msg.tool_calls.is_null()) m["tool_calls"] = msg.tool_calls;
        history.push_back(m);
    }

    if (!m_conversation) {
        nlohmann::ordered_json history_to_init = nlohmann::ordered_json::array();
        if (messages.size() > 0) {
            for (size_t i = 0; i < messages.size() - 1; ++i) {
                history_to_init.push_back(history[i]);
            }
            if (!init(toolsJson, history_to_init.dump(), settings)) return nlohmann::json{{"error", "Failed to init"}};
            m_lastMessageIndex = messages.size() - 1;
        } else {
             if (!init(toolsJson, "[]", settings)) return nlohmann::json{{"error", "Failed to init"}};
             m_lastMessageIndex = 0;
        }
    }

    LiteRtLmJsonResponse* response = nullptr;
    for (size_t i = m_lastMessageIndex; i < messages.size(); ++i) {
        std::string json_message = history[i].dump();
        std::string extra_context = "";
        if (i < messages.size() - 1) {
            extra_context = "{\"has_pending_message\": true}";
        }
        if (response) litert_lm_json_response_delete(response);
        response = litert_lm_conversation_send_message(m_conversation, json_message.c_str(), 
                                                       extra_context.empty() ? nullptr : extra_context.c_str(), nullptr);
    }
    
    m_lastMessageIndex = messages.size();

    if (!response) return nlohmann::json{{"error", "Engine error"}};
    
    std::string response_str;
    const char* str = litert_lm_json_response_get_string(response);
    if (str) response_str = str;

    nlohmann::json response_json = nlohmann::json::parse(response_str, nullptr, false);
    if (response_json.is_discarded()) {
        response_json = nlohmann::json{{"content", response_str}};
    }

    litert_lm_json_response_delete(response);
    return response_json;
}

void InferenceSession::predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings, const std::string& toolsJson) {
    nlohmann::ordered_json history = nlohmann::ordered_json::array();
    for (const auto& msg : messages) {
        nlohmann::ordered_json m = nlohmann::ordered_json::object();
        m["role"] = msg.role;
        m["content"] = msg.content;
        if (!msg.tool_call_id.empty()) m["tool_call_id"] = msg.tool_call_id;
        if (!msg.tool_calls.is_null()) m["tool_calls"] = msg.tool_calls;
        history.push_back(m);
    }

    if (!m_conversation) {
        nlohmann::ordered_json history_to_init = nlohmann::ordered_json::array();
        if (messages.size() > 0) {
            for (size_t i = 0; i < messages.size() - 1; ++i) {
                history_to_init.push_back(history[i]);
            }
            if (!init(toolsJson, history_to_init.dump(), settings)) {
                callback(nlohmann::json{{"error", "Failed to init"}}, true);
                return;
            }
            m_lastMessageIndex = messages.size() - 1;
        } else {
            if (!init(toolsJson, "[]", settings)) {
                callback(nlohmann::json{{"error", "Failed to init"}}, true);
                return;
            }
            m_lastMessageIndex = 0;
        }
    }

    for (size_t i = m_lastMessageIndex; i < messages.size() - 1; ++i) {
        std::string json_message = history[i].dump();
        const char* extra = "{\"has_pending_message\": true}";
        auto* res = litert_lm_conversation_send_message(m_conversation, json_message.c_str(), extra, nullptr);
        if (res) litert_lm_json_response_delete(res);
    }

    std::string last_json = history[messages.size() - 1].dump();
    m_lastMessageIndex = messages.size();

    auto data = std::make_unique<AsyncCallbackData>();
    data->callback = callback;
    try {
        data->session = shared_from_this();
    } catch (...) {
    }
    auto* data_ptr = data.release();
    
    int status = litert_lm_conversation_send_message_stream(
        m_conversation, last_json.c_str(), nullptr, nullptr,
        [](void* user_data, const char* text, bool is_final, const char* error_message) {
            auto* data = static_cast<AsyncCallbackData*>(user_data);
            
            if (error_message) {
                data->callback(nlohmann::json{{"error", error_message}}, true);
            } else if (text) {
                nlohmann::json parsed = nlohmann::json::parse(text, nullptr, false);
                if (!parsed.is_discarded()) {
                    data->callback(parsed, false);
                } else {
                    data->callback(nlohmann::json{{"content", text}}, false);
                }
            }

            if (is_final) {
                data->callback(nlohmann::json::object(), true);
                std::thread([data]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    delete data;
                }).detach();
            }
        },
        data_ptr
    );

    if (status != 0) {
        std::cerr << "[InferenceSession] Failed to start stream: " << status << std::endl;
        callback(nlohmann::json{{"error", "Error starting stream."}}, true);
        delete data_ptr;
    }
}

} // namespace inference
} // namespace bonsai
