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

bool ModelManager::init(const std::string& modelPath, bool useGpu, bool useVisionGpu, bool useAudioGpu) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }

    std::cout << "[ModelManager] Initializing engine with model: " << modelPath << std::endl;

    // Pass "CPU" for backends to ensure they are loaded if present in the model.
    LiteRtLmEngineSettings* settings = litert_lm_engine_settings_create(modelPath.c_str(), 
                                                                       useGpu ? "GPU" : "CPU", 
                                                                       useVisionGpu ? "GPU" : "CPU", 
                                                                       useAudioGpu ? "GPU" : "CPU");
    if (!settings) {
        std::cerr << "[ModelManager] Failed to create engine settings." << std::endl;
        return false;
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
