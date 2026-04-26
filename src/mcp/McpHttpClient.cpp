#include "mcp/McpHttpClient.hpp"
#include "oatpp/network/Url.hpp"
#include "oatpp/web/protocol/http/outgoing/BufferBody.hpp"
#include <iostream>

namespace bonsai {
namespace mcp {

McpHttpClient::McpHttpClient(const oatpp::Object<McpServerDto>& config, const std::shared_ptr<oatpp::json::ObjectMapper>& objectMapper)
    : m_objectMapper(objectMapper) {
    
    oatpp::String url = config->url;
    auto parsedUrl = oatpp::network::Url::Parser::parseUrl(url);
    
    oatpp::String host = parsedUrl.authority.host;
    v_uint16 port = parsedUrl.authority.port;
    if (port == 0) {
        port = (parsedUrl.scheme == "https") ? 443 : 80;
    }
    
    m_path = parsedUrl.path ? parsedUrl.path->c_str() : "";
    if (!m_path.empty() && m_path[0] == '/') {
        m_path.erase(0, 1);
    }
    
    auto connectionProvider = oatpp::network::tcp::client::ConnectionProvider::createShared({host, port, oatpp::network::Address::IP_4});
    m_requestExecutor = oatpp::web::client::HttpRequestExecutor::createShared(connectionProvider);
    
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
    
    auto params = oatpp::Fields<oatpp::Any>::createShared();
    req->params = params;
    
    try {
        auto jsonBody = m_objectMapper->writeToString(req);
        oatpp::web::client::HttpRequestExecutor::Headers headers;
        headers.put("Content-Type", "application/json");
        if (!m_authHeader.empty()) {
            headers.put("Authorization", m_authHeader.c_str());
        }

        auto response = m_requestExecutor->execute("POST", m_path.c_str(), headers, oatpp::web::protocol::http::outgoing::BufferBody::createShared(jsonBody, "application/json"), nullptr);
        
        if (response && response->getStatusCode() == 200) {
            auto body = response->readBodyToString();
            if (body) {
                return m_objectMapper->readFromString<oatpp::Object<McpJsonRpcResponseDto>>(body);
            }
        }
    } catch (...) {
        // Silently fail for now as per orchestrator pattern
    }
    return nullptr;
}

oatpp::Object<McpListToolsResultDto> McpHttpClient::listTools() {
    auto req = McpJsonRpcRequestDto::createShared();
    req->id = "2";
    req->method = "tools/list";
    req->params = oatpp::Fields<oatpp::Any>::createShared();
    
    try {
        auto jsonBody = m_objectMapper->writeToString(req);
        oatpp::web::client::HttpRequestExecutor::Headers headers;
        headers.put("Content-Type", "application/json");
        if (!m_authHeader.empty()) {
            headers.put("Authorization", m_authHeader.c_str());
        }

        auto response = m_requestExecutor->execute("POST", m_path.c_str(), headers, oatpp::web::protocol::http::outgoing::BufferBody::createShared(jsonBody, "application/json"), nullptr);
        
        if (response && response->getStatusCode() == 200) {
            auto body = response->readBodyToString();
            if (body) {
                auto rpcResponse = m_objectMapper->readFromString<oatpp::Object<McpJsonRpcResponseDto>>(body);
                if (rpcResponse && rpcResponse->result) {
                    oatpp::String resJson = m_objectMapper->writeToString(rpcResponse->result);
                    return m_objectMapper->readFromString<oatpp::Object<McpListToolsResultDto>>(resJson);
                }
            }
        }
    } catch (...) {}
    return nullptr;
}

oatpp::Object<McpCallToolResultDto> McpHttpClient::callTool(const oatpp::String& name, const oatpp::Any& arguments) {
    auto req = McpJsonRpcRequestDto::createShared();
    req->id = "3";
    req->method = "tools/call";
    
    auto callParams = McpCallToolParamsDto::createShared();
    callParams->name = name;
    callParams->arguments = arguments;
    req->params = callParams;
    
    try {
        auto jsonBody = m_objectMapper->writeToString(req);
        oatpp::web::client::HttpRequestExecutor::Headers headers;
        headers.put("Content-Type", "application/json");
        if (!m_authHeader.empty()) {
            headers.put("Authorization", m_authHeader.c_str());
        }

        auto response = m_requestExecutor->execute("POST", m_path.c_str(), headers, oatpp::web::protocol::http::outgoing::BufferBody::createShared(jsonBody, "application/json"), nullptr);
        
        if (response && response->getStatusCode() == 200) {
            auto body = response->readBodyToString();
            if (body) {
                auto rpcResponse = m_objectMapper->readFromString<oatpp::Object<McpJsonRpcResponseDto>>(body);
                if (rpcResponse && rpcResponse->result) {
                    oatpp::String resJson = m_objectMapper->writeToString(rpcResponse->result);
                    return m_objectMapper->readFromString<oatpp::Object<McpCallToolResultDto>>(resJson);
                }
            }
        }
    } catch (...) {}
    return nullptr;
}

} // namespace mcp
} // namespace bonsai
