#ifndef BONSAI_ERROR_DTO_HPP
#define BONSAI_ERROR_DTO_HPP

#include "oatpp/macro/codegen.hpp"
#include "oatpp/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

/**
 * @brief Error detail DTO.
 */
class ErrorDetailDto : public oatpp::DTO {
    DTO_INIT(ErrorDetailDto, DTO)

    DTO_FIELD(String, message);
    DTO_FIELD(String, type);
    DTO_FIELD(String, param);
    DTO_FIELD(String, code);
};

/**
 * @brief Top-level Error Response DTO.
 */
class ErrorResponseDto : public oatpp::DTO {
    DTO_INIT(ErrorResponseDto, DTO)

    DTO_FIELD(Object<ErrorDetailDto>, error);
};

#include OATPP_CODEGEN_END(DTO)

#endif // BONSAI_ERROR_DTO_HPP
