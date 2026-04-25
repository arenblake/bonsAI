#include "inference/InferenceSession.hpp"
#include "inference/ModelManager.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace bonsai {
namespace inference {

InferenceSession::InferenceSession() {
    std::cout << "[InferenceSession] Session object created." << std::endl;
}

InferenceSession::~InferenceSession() {
    if (m_conversation) {
        std::cout << "[InferenceSession] Destroying conversation..." << std::endl;
        litert_lm_conversation_delete(m_conversation);
        m_conversation = nullptr;
    }
    if (m_engineLock.owns_lock()) {
        std::cout << "[InferenceSession] Releasing engine lock." << std::endl;
        m_engineLock.unlock();
    }
    std::cout << "[InferenceSession] Session object destroyed." << std::endl;
}

bool InferenceSession::init(const std::string& toolsJson) {
    auto& manager = ModelManager::getInstance();
    if (!manager.isInitialized()) return false;

    // Acquire global lock for the lifetime of this InferenceSession
    std::cout << "[InferenceSession] Waiting for engine lock..." << std::endl;
    m_engineLock = std::unique_lock<std::mutex>(manager.getInferenceMutex());
    std::cout << "[InferenceSession] Engine lock acquired." << std::endl;

    auto* engine = manager.getEngine();
    const char* tools_ptr = toolsJson.empty() ? nullptr : toolsJson.c_str();

    // Note: Constrained decoding is disabled because of a segfault in the LiteRT-LM FST engine
    // when using Gemma-4 tool calling schemas.
    LiteRtLmConversationConfig* config = litert_lm_conversation_config_create(
        engine, nullptr, nullptr, tools_ptr, nullptr, false);
    
    if (!config) return false;

    m_conversation = litert_lm_conversation_create(engine, config);
    litert_lm_conversation_config_delete(config);
    
    return m_conversation != nullptr;
}

nlohmann::json InferenceSession::predict(const std::vector<Message>& messages, const SamplerSettings& settings) {
    if (!m_conversation && !init()) return json{{"error", "Conversation not initialized."}};

    json conv_msg = json::array();
    for (const auto& m : messages) {
        json entry = {{"role", m.role}};
        if (!m.content.empty()) entry["content"] = m.content;
        if (!m.tool_call_id.empty()) entry["tool_call_id"] = m.tool_call_id;
        if (!m.tool_calls.is_null() && !m.tool_calls.empty()) entry["tool_calls"] = m.tool_calls;
        conv_msg.push_back(entry);
    }

    std::string msg_str = conv_msg.dump();
    
    LiteRtLmJsonResponse* response = litert_lm_conversation_send_message(m_conversation, msg_str.c_str(), nullptr);
    if (!response) {
        return json{{"error", "Inference failed."}};
    }

    std::string result_str = litert_lm_json_response_get_string(response);
    litert_lm_json_response_delete(response);

    try {
        return json::parse(result_str);
    } catch (...) {
        return json{{"content", result_str}, {"role", "assistant"}};
    }
}

void InferenceSession::predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings) {
    if (!m_conversation && !init()) {
        callback(json{{"error", "Conversation not initialized."}}, true);
        return;
    }

    json conv_msg = json::array();
    for (const auto& m : messages) {
        json entry = {{"role", m.role}};
        if (!m.content.empty()) entry["content"] = m.content;
        if (!m.tool_call_id.empty()) entry["tool_call_id"] = m.tool_call_id;
        if (!m.tool_calls.is_null() && !m.tool_calls.empty()) entry["tool_calls"] = m.tool_calls;
        conv_msg.push_back(entry);
    }

    std::string msg_str = conv_msg.dump();

    auto* cb_ptr = new TokenCallback(callback);
    auto status = litert_lm_conversation_send_message_stream(
        m_conversation, msg_str.c_str(), nullptr,
        [](void* data, const char* chunk, bool is_final, const char* error_msg) {
            auto* cb = static_cast<TokenCallback*>(data);
            if (!cb) return;

            if (error_msg) {
                (*cb)(json{{"error", error_msg}}, true);
                std::thread([cb]() { delete cb; }).detach();
                return;
            }

            if (chunk && strlen(chunk) > 0) {
                try {
                    (*cb)(json::parse(chunk), false);
                } catch (...) {
                    // Fallback
                }
            }

            if (is_final) {
                (*cb)(json::object(), true);
                // Defer deletion to a separate thread to avoid deadlock with engine thread
                std::thread([cb]() { 
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    delete cb; 
                }).detach();
            }
        },
        cb_ptr
    );

    if (status != 0) {
        callback(json{{"error", "Error starting stream."}}, true);
        delete cb_ptr;
    }
}

} // namespace inference
} // namespace bonsai
