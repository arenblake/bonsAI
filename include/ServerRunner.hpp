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
     * Starts the server on the current thread (blocks).
     */
    void run(const std::string& host, uint16_t port, const std::string& model_path);

    /**
     * Starts the server in a background thread (non-blocking).
     */
    void startAsync(const std::string& host, uint16_t port, const std::string& model_path);

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
