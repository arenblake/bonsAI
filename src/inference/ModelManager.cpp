#include "inference/ModelManager.hpp"
#include <iostream>

namespace bonsai {
namespace inference {

ModelManager& ModelManager::getInstance() {
    static ModelManager instance;
    return instance;
}

ModelManager::~ModelManager() {
    if (m_engine) {
        litert_lm_engine_delete(m_engine);
    }
}

bool ModelManager::init(const std::string& modelPath, bool useGpu) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }

    std::cout << "[ModelManager] Initializing engine with model: " << modelPath << std::endl;

    LiteRtLmEngineSettings* settings = litert_lm_engine_settings_create(modelPath.c_str(), 
                                                                       useGpu ? "GPU" : "CPU", 
                                                                       nullptr, nullptr);
    if (!settings) {
        std::cerr << "Failed to create engine settings." << std::endl;
        return false;
    }

    m_engine = litert_lm_engine_create(settings);
    litert_lm_engine_settings_delete(settings);

    if (!m_engine) {
        std::cerr << "Failed to create engine." << std::endl;
        return false;
    }

    m_modelPath = modelPath;
    m_initialized = true;
    
    std::cout << "[ModelManager] Engine initialized successfully." << std::endl;
    return true;
}

bool ModelManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

} // namespace inference
} // namespace bonsai
