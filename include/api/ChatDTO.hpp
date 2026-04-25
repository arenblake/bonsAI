#ifndef BONSAI_CHAT_DTO_HPP
#define BONSAI_CHAT_DTO_HPP

#include "oatpp/macro/codegen.hpp"
#include "oatpp/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * @brief Message DTO for Chat Completion Request.
 */
class ChatMessageDto : public oatpp::DTO {
    DTO_INIT(ChatMessageDto, DTO)

    DTO_FIELD(String, role);
    DTO_FIELD(String, content);
};

/**
 * @brief Request DTO for /v1/chat/completions.
 */
class ChatCompletionRequestDto : public oatpp::DTO {
    DTO_INIT(ChatCompletionRequestDto, DTO)

    DTO_FIELD(String, model);
    DTO_FIELD(Vector<Object<ChatMessageDto>>, messages);
    DTO_FIELD(Boolean, stream, "stream") = false;
    DTO_FIELD(Float32, temperature, "temperature") = 1.0f;
    DTO_FIELD(Float32, top_p, "top_p") = 1.0f;
    DTO_FIELD(Int32, max_tokens, "max_tokens");
};

/**
 * @brief Choice DTO for Chat Completion Response.
 */
class ChatChoiceDto : public oatpp::DTO {
    DTO_INIT(ChatChoiceDto, DTO)

    DTO_FIELD(Int32, index);
    DTO_FIELD(Object<ChatMessageDto>, message);
    DTO_FIELD(String, finish_reason, "finish_reason");
};

/**
 * @brief Usage DTO for Chat Completion Response.
 */
class UsageDto : public oatpp::DTO {
    DTO_INIT(UsageDto, DTO)

    DTO_FIELD(Int32, prompt_tokens, "prompt_tokens");
    DTO_FIELD(Int32, completion_tokens, "completion_tokens");
    DTO_FIELD(Int32, total_tokens, "total_tokens");
};

/**
 * @brief Response DTO for /v1/chat/completions.
 */
class ChatCompletionResponseDto : public oatpp::DTO {
    DTO_INIT(ChatCompletionResponseDto, DTO)

    DTO_FIELD(String, id);
    DTO_FIELD(String, object) = "chat.completion";
    DTO_FIELD(Int64, created);
    DTO_FIELD(String, model);
    DTO_FIELD(Vector<Object<ChatChoiceDto>>, choices);
    DTO_FIELD(Object<UsageDto>, usage);
};

/**
 * @brief Model metadata DTO.
 */
class ModelDto : public oatpp::DTO {
    DTO_INIT(ModelDto, DTO)

    DTO_FIELD(String, id);
    DTO_FIELD(String, object) = "model";
    DTO_FIELD(Int64, created) = 0L;
    DTO_FIELD(String, owned_by, "owned_by") = "bonsai";
};

/**
 * @brief List Models Response DTO.
 */
class ModelListDto : public oatpp::DTO {
    DTO_INIT(ModelListDto, DTO)

    DTO_FIELD(String, object) = "list";
    DTO_FIELD(Vector<Object<ModelDto>>, data);
};

/**
 * @brief Choice DTO for Chat Completion Chunk.
 */
class ChatChunkChoiceDto : public oatpp::DTO {
    DTO_INIT(ChatChunkChoiceDto, DTO)

    DTO_FIELD(Int32, index);
    DTO_FIELD(Object<ChatMessageDto>, delta);
    DTO_FIELD(String, finish_reason, "finish_reason");
};

/**
 * @brief Response DTO for streaming /v1/chat/completions chunks.
 */
class ChatCompletionChunkDto : public oatpp::DTO {
    DTO_INIT(ChatCompletionChunkDto, DTO)

    DTO_FIELD(String, id);
    DTO_FIELD(String, object) = "chat.completion.chunk";
    DTO_FIELD(Int64, created);
    DTO_FIELD(String, model);
    DTO_FIELD(Vector<Object<ChatChunkChoiceDto>>, choices);
};

#include OATPP_CODEGEN_END(DTO)

#endif // BONSAI_CHAT_DTO_HPP
