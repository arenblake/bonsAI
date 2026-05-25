#include <iostream>
#include <string>
#include <vector>

#include "ServerRunner.hpp"

int main(int argc, char** argv) {
    bonsai::ServerRunner::initEnvironment();

    std::string model_path;
    std::string host = "127.0.0.1";
    uint16_t port = 8080;
    std::string backend = "CPU";
    int max_num_tokens = 4096;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--backend" && i + 1 < argc) {
            backend = argv[++i];
            if (backend == "cpu") backend = "CPU";
            else if (backend == "gpu") backend = "GPU";
        } else if (arg == "--max-num-tokens" && i + 1 < argc) {
            max_num_tokens = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options] <path_to_model.litertlm>\n"
                      << "Options:\n"
                      << "  --host <host>            Host address to bind to (default: 127.0.0.1)\n"
                      << "  --port <port>            Port number to listen on (default: 8080)\n"
                      << "  --backend <backend>      Hardware backend to use: CPU, GPU (default: CPU)\n"
                      << "  --max-num-tokens <size>  Context window size limit (default: 4096)\n"
                      << "  --help, -h               Show this help message\n";
            bonsai::ServerRunner::destroyEnvironment();
            return 0;
        } else if (model_path.empty()) {
            model_path = arg;
        }
    }

    if (model_path.empty()) {
        std::cerr << "Error: No model path provided.\n";
        std::cout << "Usage: " << argv[0] << " [options] <path_to_model.litertlm>\n";
        bonsai::ServerRunner::destroyEnvironment();
        return 1;
    }

    try {
        bonsai::ServerRunner runner;
        runner.run(host, port, model_path, backend, max_num_tokens);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        bonsai::ServerRunner::destroyEnvironment();
        return 1;
    }

    bonsai::ServerRunner::destroyEnvironment();
    return 0;
}
