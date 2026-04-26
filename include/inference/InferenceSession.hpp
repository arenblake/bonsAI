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
class InferenceSession : public std::enable_shared_from_this<InferenceSession> {
public:
    InferenceSession();
    ~InferenceSession();

    InferenceSession(const InferenceSession&) = delete;
    InferenceSession& operator=(const InferenceSession&) = delete;

    /**
     * @brief Initialize the session with optional tools and history.
     * @param toolsJson JSON string defining tools (OpenAI format).
     * @param historyJson JSON array of previous messages.
     * @param settings Sampler settings.
     * @return bool True if successful.
     */
    bool init(const std::string& toolsJson = "", const std::string& historyJson = "", const SamplerSettings& settings = {});

    nlohmann::json predict(const std::vector<Message>& messages, const SamplerSettings& settings = {}, const std::string& toolsJson = "");
    void predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings = {}, const std::string& toolsJson = "");

private:
    LiteRtLmConversation* m_conversation = nullptr;
    std::string m_toolsJson;
    bool m_hasLock = false;
    size_t m_lastMessageIndex = 0;
};

} // namespace inference
} // namespace bonsai

#endif // BONSAI_INFERENCE_SESSION_HPP
