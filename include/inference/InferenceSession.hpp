#ifndef BONSAI_INFERENCE_SESSION_HPP
#define BONSAI_INFERENCE_SESSION_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

#include "c/engine.h"
#include "nlohmann/json.hpp"

namespace bonsai {
namespace inference {

/**
 * @brief Represents a single turn/message in a conversation.
 */
struct Message {
    std::string role;
    nlohmann::ordered_json content;
    std::string tool_call_id;
    nlohmann::ordered_json tool_calls;
};

/**
 * @brief Bridge for streaming callbacks.
 */
using TokenCallback = std::function<void(const nlohmann::json& response, bool is_done)>;

/**
 * @brief Sampler settings for the inference engine.
 */
struct SamplerSettings {
    float temperature = 1.0f;
    float top_p = 1.0f;
    int max_tokens = -1;
};

/**
 * @brief Wraps a LiteRT-LM Conversation session.
 */
class InferenceSession {
public:
    InferenceSession();
    ~InferenceSession();

    /**
     * @brief Initialize the session with optional tools and history.
     * @param toolsJson JSON string defining tools (OpenAI format).
     * @param historyJson JSON array of previous messages.
     * @return bool True if successful.
     */
    bool init(const std::string& toolsJson = "", const std::string& historyJson = "");

    nlohmann::json predict(const std::vector<Message>& messages, const SamplerSettings& settings = {});
    void predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings = {});

private:
    LiteRtLmConversation* m_conversation = nullptr;
    std::unique_lock<std::mutex> m_engineLock;
};

} // namespace inference
} // namespace bonsai

#endif // BONSAI_INFERENCE_SESSION_HPP
