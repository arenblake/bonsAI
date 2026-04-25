#ifndef BONSAI_MCP_DTO_HPP
#define BONSAI_MCP_DTO_HPP

#include "oatpp/macro/codegen.hpp"
#include "oatpp/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class McpJsonRpcRequestDto : public oatpp::DTO {
    DTO_INIT(McpJsonRpcRequestDto, DTO)

    DTO_FIELD(String, jsonrpc) = "2.0";
    DTO_FIELD(String, id);
    DTO_FIELD(String, method);
    DTO_FIELD(Any, params);
};

class McpJsonRpcResponseDto : public oatpp::DTO {
    DTO_INIT(McpJsonRpcResponseDto, DTO)

    DTO_FIELD(String, jsonrpc) = "2.0";
    DTO_FIELD(String, id);
    DTO_FIELD(Any, result);
    DTO_FIELD(Any, error);
};

class McpToolDto : public oatpp::DTO {
    DTO_INIT(McpToolDto, DTO)

    DTO_FIELD(String, name);
    DTO_FIELD(String, description);
    DTO_FIELD(Any, inputSchema);
};

class McpListToolsResultDto : public oatpp::DTO {
    DTO_INIT(McpListToolsResultDto, DTO)

    DTO_FIELD(Vector<Object<McpToolDto>>, tools);
    DTO_FIELD(String, nextCursor);
};

class McpCallToolResultContentDto : public oatpp::DTO {
    DTO_INIT(McpCallToolResultContentDto, DTO)
    
    DTO_FIELD(String, type) = "text";
    DTO_FIELD(String, text);
};

class McpCallToolResultDto : public oatpp::DTO {
    DTO_INIT(McpCallToolResultDto, DTO)

    DTO_FIELD(Vector<Object<McpCallToolResultContentDto>>, content);
    DTO_FIELD(Boolean, isError) = false;
};

class McpCallToolParamsDto : public oatpp::DTO {
    DTO_INIT(McpCallToolParamsDto, DTO)
    
    DTO_FIELD(String, name);
    DTO_FIELD(Any, arguments);
};

#include OATPP_CODEGEN_END(DTO)

#endif // BONSAI_MCP_DTO_HPP
