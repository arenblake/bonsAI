#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "oatpp/Environment.hpp"
#include "oatpp/network/Server.hpp"

#include "AppComponent.hpp"
#include "api/ChatController.hpp"
#include "inference/ModelManager.hpp"

using namespace bonsai::inference;

/**
 * @brief Main execution function for the BonsAI server.
 */
void run(const std::string& host, uint16_t port, const std::string& model_path) {
    // 0. Register host/port components using the ServerConfig struct
    auto serverConfig = std::make_shared<bonsai::ServerConfig>();
    serverConfig->host = host;
    serverConfig->port = port;
    oatpp::Environment::Component<std::shared_ptr<bonsai::ServerConfig>> configComponent(serverConfig);

    // 1. Initialize Components
    bonsai::AppComponent components;

    // 2. Initialize Model Manager
    auto& manager = ModelManager::getInstance();
    // Defaulting to enabling vision/audio support if model has them
    if (!manager.init(model_path, false, false, false)) {
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

    // 5. Create server and run
    oatpp::network::Server server(connectionProvider, connectionHandler);

    std::cout << "BonsAI Server is listening on " << host << ":" << port << "..." << std::endl;

    server.run();
}

int main(int argc, char** argv) {
    oatpp::Environment::init();

    std::string model_path;
    std::string host = "127.0.0.1";
    uint16_t port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options] <path_to_model.litertlm>\n"
                      << "Options:\n"
                      << "  --host <host>  Host address to bind to (default: 127.0.0.1)\n"
                      << "  --port <port>  Port number to listen on (default: 8080)\n"
                      << "  --help, -h     Show this help message\n";
            oatpp::Environment::destroy();
            return 0;
        } else if (model_path.empty()) {
            model_path = arg;
        }
    }

    if (model_path.empty()) {
        std::cerr << "Error: No model path provided.\n";
        std::cout << "Usage: " << argv[0] << " [options] <path_to_model.litertlm>\n";
        oatpp::Environment::destroy();
        return 1;
    }

    try {
        run(host, port, model_path);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        oatpp::Environment::destroy();
        return 1;
    }

    oatpp::Environment::destroy();
    return 0;
}
