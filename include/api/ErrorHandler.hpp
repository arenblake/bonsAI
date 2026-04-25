#ifndef BONSAI_ERROR_HANDLER_HPP
#define BONSAI_ERROR_HANDLER_HPP

#include "oatpp/web/server/handler/ErrorHandler.hpp"
#include "oatpp/web/protocol/http/outgoing/ResponseFactory.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "api/ErrorDTO.hpp"

namespace bonsai {
namespace api {

/**
 * @brief Custom ErrorHandler to return OpenAI-compatible JSON error messages.
 */
class ErrorHandler : public oatpp::web::server::handler::DefaultErrorHandler {
private:
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_objectMapper;
public:
    ErrorHandler(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper)
        : m_objectMapper(objectMapper) {}

    std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
    renderError(const HttpServerErrorStacktrace& stacktrace) override {
        
        auto errorResponse = ErrorResponseDto::createShared();
        errorResponse->error = ErrorDetailDto::createShared();
        
        if (!stacktrace.stack.empty()) {
            errorResponse->error->message = stacktrace.stack.front();
        } else {
            errorResponse->error->message = stacktrace.status.description;
        }
        
        errorResponse->error->type = "invalid_request_error";
        errorResponse->error->code = std::to_string(stacktrace.status.code).c_str();

        auto response = oatpp::web::protocol::http::outgoing::ResponseFactory::createResponse(stacktrace.status, errorResponse, m_objectMapper);
        
        for(const auto& pair : stacktrace.headers.getAll()) {
            response->putHeader_Unsafe(pair.first, pair.second);
        }

        return response;
    }
};

} // namespace api
} // namespace bonsai

#endif // BONSAI_ERROR_HANDLER_HPP
