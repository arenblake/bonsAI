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

#include "absl/strings/escaping.h"

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
        
        std::string fullPath = bonsai::inference::ModelManager::getInstance().getModelName();
        size_t lastSlash = fullPath.find_last_of("/\\");
        model->id = (lastSlash == std::string::npos) ? fullPath.c_str() : fullPath.substr(lastSlash + 1).c_str();
        
        response->data = { model };
        return createDtoResponse(Status::CODE_200, response);
    }

    std::shared_ptr<OutgoingResponse> createErrorResponse(const Status& status, const std::string& message) {
        auto errorResponse = ErrorResponseDto::createShared();
        errorResponse->error = ErrorDetailDto::createShared();
        errorResponse->error->message = message.c_str();
        errorResponse->error->type = "invalid_request_error";
        errorResponse->error->code = std::to_string(status.code).c_str();
        return createDtoResponse(status, errorResponse);
    }

    ENDPOINT("POST", "/v1/chat/completions", chatCompletions,
             BODY_DTO(Object<ChatCompletionRequestDto>, request)) {
        
        // 1. Validation
        if (!request->messages || request->messages->empty()) {
            return createErrorResponse(Status::CODE_400, "Missing or empty 'messages' field.");
        }

        for (const auto& msgDto : *request->messages) {
            if (!msgDto->role) {
                return createErrorResponse(Status::CODE_400, "Message missing required 'role' field.");
            }
            std::string role = msgDto->role->c_str();
            if (role != "user" && role != "assistant" && role != "system" && role != "tool" && role != "developer") {
                return createErrorResponse(Status::CODE_400, "Invalid role: " + std::string(msgDto->role->c_str()));
            }
        }

        // Map common parameters
        bonsai::inference::SamplerSettings settings;
        if (request->temperature) settings.temperature = request->temperature;
        if (request->top_p) settings.top_p = request->top_p;
        if (request->max_tokens) settings.max_tokens = request->max_tokens;

        // Convert DTO messages to internal Message format
        std::vector<bonsai::inference::Message> messages;
        if (request->messages) {
            for (const auto& msgDto : *request->messages) {
                bonsai::inference::Message msg;
                msg.role = msgDto->role ? msgDto->role->c_str() : "";
                
                // Handle content (String or Multimodal Array)
                auto content_oatpp = m_objectMapper->writeToString(msgDto->content);
                nlohmann::ordered_json content_json = nlohmann::ordered_json::parse(content_oatpp->c_str());
                
                if (content_json.is_string()) {
                    msg.content = content_json.get<std::string>();
                } else if (content_json.is_array()) {
                    nlohmann::ordered_json multimodal_content = nlohmann::ordered_json::array();
                    for (auto& part : content_json) {
                        if (part.contains("type")) {
                            std::string type = part["type"];
                            if (type == "text" && part.contains("text")) {
                                multimodal_content.push_back({{"type", "text"}, {"text", part["text"]}});
                            } else if (type == "image_url" && part.contains("image_url")) {
                                std::string url = part["image_url"]["url"];
                                if (url.starts_with("data:image")) {
                                    size_t comma = url.find(",");
                                    if (comma != std::string::npos) {
                                        multimodal_content.push_back({{"type", "image"}, {"blob", url.substr(comma + 1)}});
                                    }
                                } else {
                                    multimodal_content.push_back({{"type", "image"}, {"path", url}});
                                }
                            } else if (type == "input_audio" && part.contains("input_audio")) {
                                multimodal_content.push_back({{"type", "audio"}, {"blob", part["input_audio"]["data"]}});
                            }
                        }
                    }
                    msg.content = multimodal_content;
                }

                if (msgDto->tool_call_id) msg.tool_call_id = msgDto->tool_call_id->c_str();
                
                if (msgDto->tool_calls && !msgDto->tool_calls->empty()) {
                    nlohmann::json tcs = nlohmann::json::array();
                    for (const auto& tcDto : *msgDto->tool_calls) {
                        nlohmann::json tc = {
                            {"id", tcDto->id ? tcDto->id->c_str() : ""},
                            {"type", tcDto->type ? tcDto->type->c_str() : "function"},
                            {"function", {
                                {"name", (tcDto->function && tcDto->function->name) ? tcDto->function->name->c_str() : ""},
                                {"arguments", (tcDto->function && tcDto->function->arguments) ? tcDto->function->arguments->c_str() : "{}"}
                            }}
                        };
                        tcs.push_back(tc);
                    }
                    msg.tool_calls = tcs;
                }
                messages.push_back(msg);
            }
        }

        std::string modelName = request->model ? request->model->c_str() : "bonsai-model";
        std::string id = "bonsai-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        // Prepare tools if present (keep OpenAI format as expected by Gemma-4 template)
        std::string toolsJsonStr = "";
        if (request->tools && !request->tools->empty()) {
            auto tools_oatpp = m_objectMapper->writeToString(request->tools);
            toolsJsonStr = tools_oatpp->c_str();
        }

        if (request->stream) {
            auto sseCallback = std::make_shared<bonsai::api::SseReadCallback>(m_objectMapper, id, modelName);
            
            auto session = std::make_shared<bonsai::inference::InferenceSession>();
            session->init(toolsJsonStr);
            session->predictAsync(messages, [sseCallback, session](const nlohmann::json& response, bool is_done) {
                sseCallback->pushToken(response, is_done);
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
        session.init(toolsJsonStr);
        nlohmann::json completion_json = session.predict(messages, settings);

        // Build response DTO
        auto response = ChatCompletionResponseDto::createShared();
        response->id = id;
        response->created = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
        response->model = modelName;

        auto choice = ChatChoiceDto::createShared();
        choice->index = 0;
        choice->message = ChatMessageDto::createShared();
        choice->message->role = "assistant";
        
        // Map content
        if (completion_json.contains("content")) {
            if (completion_json["content"].is_array()) {
                std::string text;
                for (const auto& item : completion_json["content"]) {
                    if (item.contains("text")) {
                        auto text_val = item["text"];
                        if (text_val.is_string()) text += text_val.get<std::string>();
                    }
                }
                choice->message->content = oatpp::String(text.c_str());
            } else if (completion_json["content"].is_string()) {
                choice->message->content = oatpp::String(completion_json["content"].get<std::string>().c_str());
            }
        }

        // Map tool calls
        if (completion_json.contains("tool_calls") && completion_json["tool_calls"].is_array()) {
            choice->message->tool_calls = oatpp::Vector<oatpp::Object<ToolCallDto>>::createShared();
            for (const auto& tc : completion_json["tool_calls"]) {
                auto tcDto = ToolCallDto::createShared();
                // Generate a random ID if missing
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
                    
                    // Strip Gemma-4 specific escaping tokens if they leaked into the string
                    size_t pos;
                    while ((pos = args_str.find("<|\"|>")) != std::string::npos) {
                        args_str.erase(pos, 5);
                    }
                    
                    tcDto->function->arguments = args_str.c_str();
                }
                choice->message->tool_calls->push_back(tcDto);
            }
            choice->finish_reason = "tool_calls";
        } else {
            choice->finish_reason = "stop";
        }

        response->choices = { choice };

        auto usage = UsageDto::createShared();
        usage->prompt_tokens = 0; 
        usage->completion_tokens = 0;
        usage->total_tokens = 0;
        response->usage = usage;

        return createDtoResponse(Status::CODE_200, response);
    }
};

#include OATPP_CODEGEN_END(ApiController)

#endif // BONSAI_CHAT_CONTROLLER_HPP
