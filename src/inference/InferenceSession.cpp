#include "inference/InferenceSession.hpp"
#include "inference/ModelManager.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace bonsai {
namespace inference {

namespace {
json messageToJson(const Message& m) {
    json entry = {{"role", m.role}};
    if (!m.content.is_null() && !m.content.empty()) {
        entry["content"] = m.content;
    }
    if (!m.tool_call_id.empty()) {
        entry["tool_call_id"] = m.tool_call_id;
    }
    if (!m.tool_calls.is_null() && !m.tool_calls.empty()) {
        entry["tool_calls"] = m.tool_calls;
    }
    return entry;
}
}

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

bool InferenceSession::init(const std::string& toolsJson, const std::string& historyJson) {
    auto& manager = ModelManager::getInstance();
    if (!manager.isInitialized()) return false;

    if (!m_engineLock.owns_lock()) {
        std::cout << "[InferenceSession] Waiting for engine lock..." << std::endl;
        m_engineLock = std::unique_lock<std::mutex>(manager.getInferenceMutex());
        std::cout << "[InferenceSession] Engine lock acquired." << std::endl;
    }

    auto* engine = manager.getEngine();
    const char* tools_ptr = toolsJson.empty() ? nullptr : toolsJson.c_str();
    const char* history_ptr = historyJson.empty() ? nullptr : historyJson.c_str();

    if (!toolsJson.empty()) {
        std::cout << "[InferenceSession] Initializing with tools." << std::endl;
    }
    if (!historyJson.empty()) {
        std::cout << "[InferenceSession] Initializing with history." << std::endl;
    }

    LiteRtLmConversationConfig* config = litert_lm_conversation_config_create(
        engine, nullptr, nullptr, tools_ptr, history_ptr, false);
    
    if (!config) return false;

    m_conversation = litert_lm_conversation_create(engine, config);
    litert_lm_conversation_config_delete(config);
    
    return m_conversation != nullptr;
}

nlohmann::json InferenceSession::predict(const std::vector<Message>& messages, const SamplerSettings& settings) {
    if (messages.empty()) return json{{"error", "No messages provided"}};

    // Split history and last message
    json history = json::array();
    for (size_t i = 0; i < messages.size() - 1; ++i) {
        history.push_back(messageToJson(messages[i]));
    }

    // Tools should be passed during init. 
    // For now we assume init was called with tools if needed, 
    // or we might need to change the API to pass tools here.
    // Actually, ChatController calls init(toolsJson, historyJson).
    
    if (!m_conversation) {
        // This shouldn't normally happen as ChatController calls init
        if (!init("", history.dump())) return json{{"error", "Failed to init"}};
    }

    json last = messageToJson(messages.back());
    std::string msg_str = last.dump();
    std::cout << "[InferenceSession] Sending message: " << msg_str << std::endl;
    
    LiteRtLmJsonResponse* response = litert_lm_conversation_send_message(m_conversation, msg_str.c_str(), nullptr);
    if (!response) {
        std::cerr << "[InferenceSession] Inference returned null response." << std::endl;
        return json{{"error", "Inference failed."}};
    }

    std::string result_str = litert_lm_json_response_get_string(response);
    std::cout << "[InferenceSession] Received response: " << result_str << std::endl;
    litert_lm_json_response_delete(response);

    try {
        return json::parse(result_str);
    } catch (...) {
        return json{{"content", result_str}, {"role", "assistant"}};
    }
}

void InferenceSession::predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings) {
    if (messages.empty()) {
        callback(json{{"error", "No messages"}}, true);
        return;
    }

    json history = json::array();
    for (size_t i = 0; i < messages.size() - 1; ++i) {
        history.push_back(messageToJson(messages[i]));
    }

    if (!m_conversation) {
        if (!init("", history.dump())) {
            callback(json{{"error", "Failed to init"}}, true);
            return;
        }
    }

    json last = messageToJson(messages.back());
    std::string msg_str = last.dump();
    std::cout << "[InferenceSession] Sending async message: " << msg_str << std::endl;

    auto* cb_ptr = new TokenCallback(callback);
    auto status = litert_lm_conversation_send_message_stream(
        m_conversation, msg_str.c_str(), nullptr,
        [](void* data, const char* chunk, bool is_final, const char* error_msg) {
            auto* cb = static_cast<TokenCallback*>(data);
            if (!cb) return;

            if (error_msg) {
                std::cerr << "[InferenceSession] Callback error: " << error_msg << std::endl;
                (*cb)(json{{"error", error_msg}}, true);
                std::thread([cb]() { delete cb; }).detach();
                return;
            }

            if (chunk && strlen(chunk) > 0) {
                try {
                    (*cb)(json::parse(chunk), false);
                } catch (...) {
                    (*cb)(json{{"content", chunk}}, false);
                }
            }

            if (is_final) {
                (*cb)(json::object(), true);
                std::thread([cb]() { 
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    delete cb; 
                }).detach();
            }
        },
        cb_ptr
    );

    if (status != 0) {
        std::cerr << "[InferenceSession] Failed to start stream: " << status << std::endl;
        callback(json{{"error", "Error starting stream."}}, true);
        delete cb_ptr;
    }
}

} // namespace inference
} // namespace bonsai
