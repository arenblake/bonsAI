#ifndef BONSAI_ERROR_HANDLER_HPP
#define BONSAI_ERROR_HANDLER_HPP

#include "oatpp/web/server/handler/ErrorHandler.hpp"
#include "oatpp/web/protocol/http/outgoing/ResponseFactory.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "api/ErrorDTO.hpp"
#include <iostream>

namespace bonsai {
namespace api {

/**
 * @brief Custom ErrorHandler to return OpenAI-compatible JSON error messages.
 */
class ErrorHandler : public oatpp::web::server::handler::DefaultErrorHandler {
private:
    std::shared_ptr<oatpp::json::ObjectMapper> m_objectMapper;
public:
    ErrorHandler(const std::shared_ptr<oatpp::json::ObjectMapper>& objectMapper)
        : m_objectMapper(objectMapper) {}

    std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
    renderError(const HttpServerErrorStacktrace& stacktrace) override {
        
        auto errorResponse = ErrorResponseDto::createShared();
        errorResponse->error = ErrorDetailDto::createShared();
        
        oatpp::web::protocol::http::Status status = stacktrace.status;

        if (!stacktrace.stack.empty()) {
            errorResponse->error->message = stacktrace.stack.front();
            // If we have a stack trace, it might be a parsing error which should be 400
            if (errorResponse->error->message->find("json") != std::string::npos || 
                errorResponse->error->message->find("DTO") != std::string::npos) {
                status = oatpp::web::protocol::http::Status::CODE_400;
            }
        } else {
            errorResponse->error->message = stacktrace.status.description;
        }
        
        errorResponse->error->type = "invalid_request_error";
        errorResponse->error->code = std::to_string(status.code).c_str();

        auto response = oatpp::web::protocol::http::outgoing::ResponseFactory::createResponse(status, errorResponse, m_objectMapper);
        
        for(const auto& pair : stacktrace.headers.getAll()) {
            response->putHeader_Unsafe(pair.first, pair.second);
        }

        return response;
    }
};

} // namespace api
} // namespace bonsai

#endif // BONSAI_ERROR_HANDLER_HPP
