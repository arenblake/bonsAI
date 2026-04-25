#ifndef BONSAI_SSE_READ_CALLBACK_HPP
#define BONSAI_SSE_READ_CALLBACK_HPP

#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "api/ChatDTO.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>

namespace bonsai {
namespace api {

/**
 * @brief Thread-safe callback to handle SSE streaming for OpenAI-compatible chunks.
 */
class SseReadCallback : public oatpp::data::stream::ReadCallback {
private:
    std::shared_ptr<oatpp::json::ObjectMapper> m_mapper;
    std::string m_chunkId;
    std::string m_modelName;
    int64_t m_created;

    std::queue<std::string> m_tokens;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_finished = false;

    std::string m_buffer;
    size_t m_bufferPos = 0;

public:
    SseReadCallback(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& mapper,
                   const std::string& chunkId,
                   const std::string& modelName)
        : m_mapper(std::static_pointer_cast<oatpp::json::ObjectMapper>(mapper)), m_chunkId(chunkId), m_modelName(modelName) {
        m_created = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    }

    /**
     * @brief Push a new token into the SSE stream.
     */
    void pushToken(const std::string& token, bool is_done) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto chunk = ChatCompletionChunkDto::createShared();
        chunk->id = m_chunkId;
        chunk->created = m_created;
        chunk->model = m_modelName;

        auto choice = ChatChunkChoiceDto::createShared();
        choice->index = 0;
        choice->delta = ChatMessageDto::createShared();
        
        if (!is_done) {
            choice->delta->content = token;
        } else {
            choice->finish_reason = "stop";
            m_finished = true;
        }

        chunk->choices = { choice };

        auto json = m_mapper->writeToString(chunk);
        m_tokens.push("data: " + std::string(json->c_str()) + "\n\n");
        
        if (is_done) {
            m_tokens.push("data: [DONE]\n\n");
        }
        
        m_cv.notify_one();
    }

    oatpp::v_io_size read(void *buffer, v_buff_size count, oatpp::async::Action& action) override {
        std::unique_lock<std::mutex> lock(m_mutex);

        // If buffer is empty, wait for data
        while (m_buffer.empty() || m_bufferPos >= m_buffer.size()) {
            if (m_tokens.empty()) {
                if (m_finished) return 0; // End of stream
                m_cv.wait(lock);
            } else {
                m_buffer = m_tokens.front();
                m_tokens.pop();
                m_bufferPos = 0;
            }
        }

        // Copy from buffer to output
        v_buff_size toCopy = std::min((v_buff_size)count, (v_buff_size)(m_buffer.size() - m_bufferPos));
        std::memcpy(buffer, m_buffer.data() + m_bufferPos, toCopy);
        m_bufferPos += toCopy;

        return toCopy;
    }
};

} // namespace api
} // namespace bonsai

#endif // BONSAI_SSE_READ_CALLBACK_HPP
