#include "ServerRunner.hpp"

#include <iostream>
#include <stdexcept>

#include "oatpp/Environment.hpp"
#include "oatpp/network/Server.hpp"

#include "AppComponent.hpp"
#include "api/ChatController.hpp"
#include "inference/ModelManager.hpp"

namespace bonsai {

using namespace bonsai::inference;

ServerRunner::ServerRunner() : m_isRunning(false) {}

ServerRunner::~ServerRunner() {
    stop();
}

void ServerRunner::initEnvironment() {
    oatpp::Environment::init();
}

void ServerRunner::destroyEnvironment() {
    oatpp::Environment::destroy();
}

void ServerRunner::run(const std::string& host, uint16_t port, const std::string& model_path,
                       const std::string& backend, int max_num_tokens) {
    if (m_isRunning.exchange(true)) {
        std::cerr << "Server is already running." << std::endl;
        return;
    }

    try {
        // 0. Register host/port components using the ServerConfig struct
        auto serverConfig = std::make_shared<bonsai::ServerConfig>();
        serverConfig->host = host;
        serverConfig->port = port;
        oatpp::Environment::Component<std::shared_ptr<bonsai::ServerConfig>> configComponent(serverConfig);

        // 1. Initialize Components
        bonsai::AppComponent components;

        // 2. Initialize Model Manager
        auto& manager = ModelManager::getInstance();
        if (!manager.init(model_path, backend, max_num_tokens)) {
            throw std::runtime_error("Failed to initialize engine.");
        }

        // 3. Create Controllers and add endpoints to router
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
        OATPP_COMPONENT(std::shared_ptr<oatpp::json::ObjectMapper>, objectMapper);
        auto chatController = std::make_shared<ChatController>(objectMapper);
        router->addController(chatController);

        // 4. Get connection handler and provider
        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

        // 5. Create server
        m_server = std::make_shared<oatpp::network::Server>(connectionProvider, connectionHandler);

        std::cout << "BonsAI Server is listening on " << host << ":" << port << "..." << std::endl;

        m_server->run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR during server run: " << e.what() << std::endl;
    }

    m_isRunning = false;
}

void ServerRunner::startAsync(const std::string& host, uint16_t port, const std::string& model_path,
                              const std::string& backend, int max_num_tokens) {
    if (m_isRunning) {
        return;
    }
    m_serverThread = std::make_unique<std::thread>(&ServerRunner::run, this, host, port, model_path, backend, max_num_tokens);
}

void ServerRunner::stop() {
    if (m_isRunning && m_server) {
        m_server->stop();
    }
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();
    m_isRunning = false;
}

} // namespace bonsai
