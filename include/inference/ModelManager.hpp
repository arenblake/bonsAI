#ifndef BONSAI_MODEL_MANAGER_HPP
#define BONSAI_MODEL_MANAGER_HPP

#include <memory>
#include <string>
#include <mutex>

#include "c/engine.h"

namespace bonsai {
namespace inference {

/**
 * @brief Singleton manager for the LiteRT-LM Engine.
 */
class ModelManager {
public:
    static ModelManager& getInstance();

    bool init(const std::string& modelPath, bool useGpu = false);
    bool isInitialized() const;
    std::string getModelName() const { return m_modelPath; }

    LiteRtLmEngine* getEngine() { return m_engine; }

private:
    ModelManager() = default;
    ~ModelManager();

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    std::string m_modelPath;
    bool m_initialized = false;
    mutable std::mutex m_mutex;

    LiteRtLmEngine* m_engine = nullptr;
};

} // namespace inference
} // namespace bonsai

#endif // BONSAI_MODEL_MANAGER_HPP
