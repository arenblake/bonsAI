#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace oatpp { namespace network { class Server; } }

namespace bonsai {

class ServerRunner {
public:
    ServerRunner();
    ~ServerRunner();

    /**
     * Initializes the Oat++ environment. Must be called before start().
     */
    static void initEnvironment();

    /**
     * Destroys the Oat++ environment.
     */
    static void destroyEnvironment();

    /**
     * @brief Starts the server on the current thread (blocks).
     * @param host Host address to bind to.
     * @param port Port number to listen on.
     * @param model_path Path to the .litertlm model file.
     * @param backend Hardware backend to use (default: "CPU").
     * @param max_num_tokens Maximum context token size (default: 4096).
     */
    void run(const std::string& host, uint16_t port, const std::string& model_path,
             const std::string& backend = "CPU", int max_num_tokens = 4096);

    /**
     * @brief Starts the server in a background thread (non-blocking).
     * @param host Host address to bind to.
     * @param port Port number to listen on.
     * @param model_path Path to the .litertlm model file.
     * @param backend Hardware backend to use (default: "CPU").
     * @param max_num_tokens Maximum context token size (default: 4096).
     */
    void startAsync(const std::string& host, uint16_t port, const std::string& model_path,
                    const std::string& backend = "CPU", int max_num_tokens = 4096);

    /**
     * Stops the running server.
     */
    void stop();

private:
    std::shared_ptr<oatpp::network::Server> m_server;
    std::unique_ptr<std::thread> m_serverThread;
    std::atomic<bool> m_isRunning;
};

} // namespace bonsai
