#ifndef BONSAI_MCP_HTTP_CLIENT_HPP
#define BONSAI_MCP_HTTP_CLIENT_HPP

#include "oatpp/web/client/ApiClient.hpp"
#include "oatpp/network/tcp/client/ConnectionProvider.hpp"
#include "oatpp/web/client/HttpRequestExecutor.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "mcp/McpDTO.hpp"
#include "api/ChatDTO.hpp"

#include OATPP_CODEGEN_BEGIN(ApiClient)

class McpApiClient : public oatpp::web::client::ApiClient {
  API_CLIENT_INIT(McpApiClient)

  API_CALL("POST", "{path}", callJsonRpc,
           PATH(String, path),
           BODY_DTO(Object<McpJsonRpcRequestDto>, request),
           HEADER(String, auth, "Authorization"))
};

#include OATPP_CODEGEN_END(ApiClient)

namespace bonsai {
namespace mcp {

class McpHttpClient {
private:
    std::shared_ptr<McpApiClient> m_client;
    std::shared_ptr<oatpp::json::ObjectMapper> m_objectMapper;
    std::string m_path;
    std::string m_authHeader;

public:
    McpHttpClient(const oatpp::Object<McpServerDto>& config, const std::shared_ptr<oatpp::json::ObjectMapper>& objectMapper);

    oatpp::Object<McpJsonRpcResponseDto> initialize();
    oatpp::Object<McpListToolsResultDto> listTools();
    oatpp::Object<McpCallToolResultDto> callTool(const oatpp::String& name, const oatpp::Any& arguments);
};

} // namespace mcp
} // namespace bonsai

#endif // BONSAI_MCP_HTTP_CLIENT_HPP
