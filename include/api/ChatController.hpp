#ifndef BONSAI_CHAT_CONTROLLER_HPP
#define BONSAI_CHAT_CONTROLLER_HPP

#include "api/ChatDTO.hpp"
#include "api/SseReadCallback.hpp"
#include "inference/InferenceSession.hpp"
#include "inference/ModelManager.hpp"

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"

#include <chrono>

#include OATPP_CODEGEN_BEGIN(ApiController)

/**
 * @brief Controller for OpenAI-compatible chat endpoints.
 */
class ChatController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<oatpp::json::ObjectMapper> m_objectMapper;
public:
    ChatController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper), 
          m_objectMapper(std::static_pointer_cast<oatpp::json::ObjectMapper>(objectMapper)) {}

public:

    ENDPOINT("GET", "/v1/models", getModels) {
        auto response = ModelListDto::createShared();
        auto model = ModelDto::createShared();
        model->id = bonsai::inference::ModelManager::getInstance().getModelName();
        response->data = { model };
        return createDtoResponse(Status::CODE_200, response);
    }

    ENDPOINT("POST", "/v1/chat/completions", chatCompletions,
             BODY_DTO(Object<ChatCompletionRequestDto>, request)) {
        
        // Map common parameters
        bonsai::inference::SamplerSettings settings;
        if (request->temperature) settings.temperature = request->temperature;
        if (request->top_p) settings.top_p = request->top_p;
        if (request->max_tokens) settings.max_tokens = request->max_tokens;

        // Convert DTO messages to internal Message format
        std::vector<bonsai::inference::Message> messages;
        for (const auto& msgDto : *request->messages) {
            messages.push_back({msgDto->role, msgDto->content});
        }

        std::string modelName = request->model ? request->model->c_str() : "bonsai-model";
        std::string id = "bonsai-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        if (request->stream) {
            auto sseCallback = std::make_shared<bonsai::api::SseReadCallback>(m_objectMapper, id, modelName);
            
            // Note: In actual implementation, we'd need to ensure session outlives the async call
            auto session = std::make_shared<bonsai::inference::InferenceSession>();
            session->predictAsync(messages, [sseCallback, session](const std::string& token, bool is_done) {
                sseCallback->pushToken(token, is_done);
            }, settings);

            auto body = std::make_shared<oatpp::web::protocol::http::outgoing::StreamingBody>(sseCallback);
            auto response = oatpp::web::protocol::http::outgoing::Response::createShared(Status::CODE_200, body);
            response->putHeader("Content-Type", "text/event-stream");
            response->putHeader("Cache-Control", "no-cache");
            response->putHeader("Connection", "keep-alive");
            return response;
        }

        // Perform non-streaming inference
        bonsai::inference::InferenceSession session;
        std::string completion = session.predict(messages, settings);

        // Build response DTO
        auto response = ChatCompletionResponseDto::createShared();
        response->id = id;
        response->created = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
        response->model = modelName;

        auto choice = ChatChoiceDto::createShared();
        choice->index = 0;
        choice->message = ChatMessageDto::createShared();
        choice->message->role = "assistant";
        choice->message->content = completion;
        choice->finish_reason = "stop";

        response->choices = { choice };

        auto usage = UsageDto::createShared();
        usage->prompt_tokens = 0; // Token counting to be implemented
        usage->completion_tokens = 0;
        usage->total_tokens = 0;
        response->usage = usage;

        return createDtoResponse(Status::CODE_200, response);
    }
};

#include OATPP_CODEGEN_END(ApiController)

#endif // BONSAI_CHAT_CONTROLLER_HPP
