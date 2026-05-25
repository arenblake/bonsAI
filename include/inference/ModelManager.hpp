#ifndef BONSAI_MODEL_MANAGER_HPP
#define BONSAI_MODEL_MANAGER_HPP

#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "c/engine.h"

namespace bonsai {
namespace inference {

/**
 * @brief Singleton manager for the LiteRT-LM Engine.
 */
class ModelManager {
public:
    static ModelManager& getInstance();

    /**
     * @brief Initializes the LiteRT-LM Engine with the specified model and configurations.
     * @param modelPath The file path to the .litertlm model.
     * @param backend The hardware backend to use ("CPU", "GPU", etc.).
     * @param maxNumTokens The maximum context window size (default: 4096).
     * @return True if initialization succeeded, false otherwise.
     */
    bool init(const std::string& modelPath, const std::string& backend = "CPU", int maxNumTokens = 4096);
    bool isInitialized() const;
    std::string getModelName() const { return m_modelPath; }

    LiteRtLmEngine* getEngine() { return m_engine; }

    /**
     * @brief Acquire the inference lock. Blocks if another inference is in progress.
     * This is thread-agnostic and can be released from a different thread.
     */
    void acquireInferenceLock();

    /**
     * @brief Release the inference lock.
     */
    void releaseInferenceLock();

private:
    ModelManager() : m_isGenerating(false) {}
    ~ModelManager();

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    std::string m_modelPath;
    bool m_initialized = false;
    mutable std::mutex m_mutex;

    bool m_isGenerating;
    std::mutex m_inferenceMutex;
    std::condition_variable m_inferenceCv;

    LiteRtLmEngine* m_engine = nullptr;
};

} // namespace inference
} // namespace bonsai

#endif // BONSAI_MODEL_MANAGER_HPP
