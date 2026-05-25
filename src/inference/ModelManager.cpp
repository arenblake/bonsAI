#include "inference/ModelManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

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

bool ModelManager::init(const std::string& modelPath, const std::string& backend, int maxNumTokens) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }

    std::cout << "[ModelManager] Initializing engine with model: " << modelPath 
              << ", backend: " << backend << ", maxNumTokens: " << maxNumTokens << std::endl;

    LiteRtLmEngineSettings* settings = litert_lm_engine_settings_create(modelPath.c_str(), 
                                                                       backend.c_str(), 
                                                                       nullptr, 
                                                                       nullptr);
    if (!settings) {
        std::cerr << "[ModelManager] Failed to create engine settings." << std::endl;
        return false;
    }

    if (maxNumTokens > 0) {
        litert_lm_engine_settings_set_max_num_tokens(settings, maxNumTokens);
    }

    m_engine = litert_lm_engine_create(settings);
    litert_lm_engine_settings_delete(settings);

    if (!m_engine) {
        std::cerr << "[ModelManager] Failed to create engine." << std::endl;
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

void ModelManager::acquireInferenceLock() {
    std::unique_lock<std::mutex> lock(m_inferenceMutex);
    m_inferenceCv.wait(lock, [this] { return !m_isGenerating; });
    m_isGenerating = true;
}

void ModelManager::releaseInferenceLock() {
    // Safety delay to allow LiteRT-LM internal threads to finish cleanup and avoid heap corruption on immediate reuse.
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    {
        std::lock_guard<std::mutex> lock(m_inferenceMutex);
        m_isGenerating = false;
    }
    m_inferenceCv.notify_all();
}

} // namespace inference
} // namespace bonsai
