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
void run(const std::string& model_path) {
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

    std::cout << "BonsAI Server is listening on port " << connectionProvider->getProperty("port").toString()->c_str() << "..." << std::endl;

    server.run();
}

int main(int argc, char** argv) {
    oatpp::Environment::init();

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_model.litertlm>" << std::endl;
        oatpp::Environment::destroy();
        return 0;
    }

    std::string model_path = argv[1];

    try {
        run(model_path);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        oatpp::Environment::destroy();
        return 1;
    }

    oatpp::Environment::destroy();
    return 0;
}
