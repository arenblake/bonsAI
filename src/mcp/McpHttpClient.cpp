#include "mcp/McpHttpClient.hpp"
#include "oatpp/network/Url.hpp"

namespace bonsai {
namespace mcp {

McpHttpClient::McpHttpClient(const oatpp::Object<McpServerDto>& config, const std::shared_ptr<oatpp::json::ObjectMapper>& objectMapper)
    : m_objectMapper(objectMapper) {
    
    oatpp::String url = config->url;
    auto parsedUrl = oatpp::network::Url::Parser::parseUrl(url);
    
    oatpp::String host = parsedUrl.host;
    v_uint16 port = parsedUrl.port;
    if (port == 0) {
        port = (parsedUrl.scheme == "https") ? 443 : 80;
    }
    m_path = parsedUrl.path ? parsedUrl.path->std_str() : "/";
    
    auto connectionProvider = oatpp::network::tcp::client::ConnectionProvider::createShared({host, port, oatpp::network::Address::IP_4});
    auto requestExecutor = oatpp::web::client::HttpRequestExecutor::createShared(connectionProvider);
    
    m_client = McpApiClient::createShared(requestExecutor, m_objectMapper);
    
    if (config->headers && config->headers->size() > 0) {
        for (const auto& kv : *config->headers) {
            if (kv.first == "Authorization") {
                m_authHeader = kv.second.getValue("");
            }
        }
    }
}

oatpp::Object<McpJsonRpcResponseDto> McpHttpClient::initialize() {
    auto req = McpJsonRpcRequestDto::createShared();
    req->id = "1";
    req->method = "initialize";
    req->params = oatpp::Any(oatpp::String("{}"));
    
    auto response = m_client->callJsonRpc(m_path, req, m_authHeader.empty() ? nullptr : oatpp::String(m_authHeader));
    if (response->getStatusCode() == 200) {
        return response->readBodyToDto<oatpp::Object<McpJsonRpcResponseDto>>(m_objectMapper.get());
    }
    return nullptr;
}

oatpp::Object<McpListToolsResultDto> McpHttpClient::listTools() {
    auto req = McpJsonRpcRequestDto::createShared();
    req->id = "2";
    req->method = "tools/list";
    
    auto response = m_client->callJsonRpc(m_path, req, m_authHeader.empty() ? nullptr : oatpp::String(m_authHeader));
    if (response->getStatusCode() == 200) {
        auto rpcResponse = response->readBodyToDto<oatpp::Object<McpJsonRpcResponseDto>>(m_objectMapper.get());
        if (rpcResponse && rpcResponse->result) {
            oatpp::String jsonStr = m_objectMapper->writeToString(rpcResponse->result);
            return m_objectMapper->readFromString<oatpp::Object<McpListToolsResultDto>>(jsonStr);
        }
    }
    return nullptr;
}

oatpp::Object<McpCallToolResultDto> McpHttpClient::callTool(const oatpp::String& name, const oatpp::Any& arguments) {
    auto req = McpJsonRpcRequestDto::createShared();
    req->id = "3";
    req->method = "tools/call";
    
    auto callParams = McpCallToolParamsDto::createShared();
    callParams->name = name;
    callParams->arguments = arguments;
    req->params = oatpp::Any(callParams);
    
    auto response = m_client->callJsonRpc(m_path, req, m_authHeader.empty() ? nullptr : oatpp::String(m_authHeader));
    if (response->getStatusCode() == 200) {
        auto rpcResponse = response->readBodyToDto<oatpp::Object<McpJsonRpcResponseDto>>(m_objectMapper.get());
        if (rpcResponse && rpcResponse->result) {
            oatpp::String jsonStr = m_objectMapper->writeToString(rpcResponse->result);
            return m_objectMapper->readFromString<oatpp::Object<McpCallToolResultDto>>(jsonStr);
        }
    }
    return nullptr;
}

} // namespace mcp
} // namespace bonsai
