#ifndef BONSAI_INFERENCE_SESSION_HPP
#define BONSAI_INFERENCE_SESSION_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "c/engine.h"

namespace bonsai {
namespace inference {

/**
 * @brief Represents a single turn/message in a conversation.
 */
struct Message {
    std::string role;
    std::string content;
};

/**
 * @brief Bridge for streaming callbacks.
 */
using TokenCallback = std::function<void(const std::string& token, bool is_done)>;

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

    std::string predict(const std::vector<Message>& messages, const SamplerSettings& settings = {});
    void predictAsync(const std::vector<Message>& messages, TokenCallback callback, const SamplerSettings& settings = {});

private:
    LiteRtLmConversation* m_conversation = nullptr;
};

} // namespace inference
} // namespace bonsai

#endif // BONSAI_INFERENCE_SESSION_HPP
