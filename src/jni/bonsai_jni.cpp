#include <jni.h>
#include <string>
#include <memory>
#include "ServerRunner.hpp"

// Global instance to hold the running server
static std::unique_ptr<bonsai::ServerRunner> g_serverRunner = nullptr;

extern "C" {

JNIEXPORT void JNICALL
Java_com_bonsai_server_BonsaiServer_initEnvironment(JNIEnv* env, jclass clazz) {
    bonsai::ServerRunner::initEnvironment();
}

JNIEXPORT void JNICALL
Java_com_bonsai_server_BonsaiServer_destroyEnvironment(JNIEnv* env, jclass clazz) {
    bonsai::ServerRunner::destroyEnvironment();
}

JNIEXPORT void JNICALL
Java_com_bonsai_server_BonsaiServer_startServer(JNIEnv* env, jobject thiz, jstring host, jint port, jstring model_path) {
    if (g_serverRunner) {
        // Server is already started or not properly stopped
        return;
    }
    
    g_serverRunner = std::make_unique<bonsai::ServerRunner>();

    const char* nativeHost = env->GetStringUTFChars(host, 0);
    const char* nativeModelPath = env->GetStringUTFChars(model_path, 0);

    // Start server in a background thread to avoid blocking the JNI call
    g_serverRunner->startAsync(nativeHost, static_cast<uint16_t>(port), nativeModelPath);

    env->ReleaseStringUTFChars(host, nativeHost);
    env->ReleaseStringUTFChars(model_path, nativeModelPath);
}

JNIEXPORT void JNICALL
Java_com_bonsai_server_BonsaiServer_stopServer(JNIEnv* env, jobject thiz) {
    if (g_serverRunner) {
        g_serverRunner->stop();
        g_serverRunner.reset();
    }
}

}
