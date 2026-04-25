#ifndef BONSAI_SSE_READ_CALLBACK_HPP
#define BONSAI_SSE_READ_CALLBACK_HPP

#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "api/ChatDTO.hpp"
#include "nlohmann/json.hpp"

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
     * @brief Push a new token/response into the SSE stream.
     */
    void pushToken(const nlohmann::json& response, bool is_done) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (is_done) {
            if (!m_finished) {
                m_tokens.push("data: [DONE]\n\n");
                m_finished = true;
                m_cv.notify_one();
            }
            return;
        }

        auto chunk = ChatCompletionChunkDto::createShared();
        chunk->id = m_chunkId;
        chunk->created = m_created;
        chunk->model = m_modelName;

        auto choice = ChatChunkChoiceDto::createShared();
        choice->index = 0;
        choice->delta = ChatMessageDto::createShared();
        
        // Extract content array if present (LiteRT-LM format)
        if (response.contains("content")) {
            if (response["content"].is_array()) {
                std::string text;
                for (const auto& item : response["content"]) {
                    if (item.contains("text")) {
                        auto text_val = item["text"];
                        if (text_val.is_string()) text += text_val.get<std::string>();
                    }
                }
                choice->delta->content = text.c_str();
            } else if (response["content"].is_string()) {
                choice->delta->content = response["content"].get<std::string>().c_str();
            }
        }

        // Handle tool calls in chunks
        if (response.contains("tool_calls") && response["tool_calls"].is_array()) {
            choice->delta->tool_calls = oatpp::Vector<oatpp::Object<ToolCallDto>>::createShared();
            for (const auto& tc : response["tool_calls"]) {
                auto tcDto = ToolCallDto::createShared();
                tcDto->id = "call_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                tcDto->type = "function";
                tcDto->function = ToolCallFunctionDto::createShared();
                if (tc.contains("function")) {
                    auto fn = tc["function"];
                    tcDto->function->name = fn.value("name", "").c_str();
                    
                    auto args = fn["arguments"];
                    std::string args_str;
                    if (args.is_string()) {
                        args_str = args.get<std::string>();
                    } else {
                        args_str = args.dump();
                    }

                    size_t pos;
                    while ((pos = args_str.find("<|\"|>")) != std::string::npos) {
                        args_str.erase(pos, 5);
                    }

                    tcDto->function->arguments = args_str.c_str();
                }
                choice->delta->tool_calls->push_back(tcDto);
            }
        }

        chunk->choices = { choice };

        try {
            auto json_str = m_mapper->writeToString(chunk);
            m_tokens.push("data: " + std::string(json_str->c_str()) + "\n\n");
            m_cv.notify_one();
        } catch (const std::exception& e) {
            std::cerr << "[SseReadCallback] Error writing chunk: " << e.what() << std::endl;
        }
    }

    oatpp::v_io_size read(void *buffer, v_buff_size count, oatpp::async::Action& action) override {
        std::unique_lock<std::mutex> lock(m_mutex);

        while (m_buffer.empty() || m_bufferPos >= m_buffer.size()) {
            if (m_tokens.empty()) {
                if (m_finished) return 0;
                m_cv.wait(lock);
            } else {
                m_buffer = m_tokens.front();
                m_tokens.pop();
                m_bufferPos = 0;
            }
        }

        v_buff_size toCopy = std::min((v_buff_size)count, (v_buff_size)(m_buffer.size() - m_bufferPos));
        std::memcpy(buffer, m_buffer.data() + m_bufferPos, toCopy);
        m_bufferPos += toCopy;

        return toCopy;
    }
};

} // namespace api
} // namespace bonsai

#endif // BONSAI_SSE_READ_CALLBACK_HPP
