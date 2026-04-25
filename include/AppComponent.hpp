#ifndef BONSAI_APP_COMPONENT_HPP
#define BONSAI_APP_COMPONENT_HPP

#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "oatpp/macro/component.hpp"
#include "api/ErrorHandler.hpp"

namespace bonsai {

/**
 * @brief Configuration for the server.
 */
struct ServerConfig {
    std::string host;
    v_uint16 port;
};

/**
 * @brief Manages Oat++ components for the application.
 */
class AppComponent {
public:
    /**
     * @brief Create ConnectionProvider component.
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([] {
        OATPP_COMPONENT(std::shared_ptr<ServerConfig>, config);
        return oatpp::network::tcp::server::ConnectionProvider::createShared({config->host.c_str(), config->port, oatpp::network::Address::IP_4});
    }());

    /**
     * @brief Create Router component.
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
        return oatpp::web::server::HttpRouter::createShared();
    }());

    /**
     * @brief Create ObjectMapper component.
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::json::ObjectMapper>, apiObjectMapper)([] {
        return std::make_shared<oatpp::json::ObjectMapper>();
    }());

    /**
     * @brief Create ConnectionHandler component.
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
        OATPP_COMPONENT(std::shared_ptr<oatpp::json::ObjectMapper>, objectMapper);
        auto connectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);
        connectionHandler->setErrorHandler(std::make_shared<bonsai::api::ErrorHandler>(objectMapper));
        return connectionHandler;
    }());
};

} // namespace bonsai

#endif // BONSAI_APP_COMPONENT_HPP
