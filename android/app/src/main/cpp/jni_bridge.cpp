/**
 * jni_bridge.cpp — JNI entry points called from Kotlin (TomodachiNative.kt).
 *
 * Provides:
 *   - install()           Run the native install pipeline (extract NSP, set up data dir).
 *   - onWindowFocusChanged()  Forward focus events from GameActivity.
 *   - nativeVersion()     Return the runtime version string.
 */

#include <jni.h>
#include <android/log.h>
#include <string>

#define LOG_TAG "TomodachiJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations for the native install pipeline.
// These bridge to the Python-originated install logic, re-implemented as a C
// library call for Android (see scripts/android/android_install.cpp).
extern "C" {
    int  tomodachi_android_install(const char* nsp_path,
                                   const char* keys_dir,
                                   const char* data_dir,
                                   const char* external_dir);
    void tomodachi_android_set_focus(bool has_focus);
    const char* tomodachi_android_version();
}

// ── JNI helpers ───────────────────────────────────────────────────────────────

static std::string jstring_to_std(JNIEnv* env, jstring js) {
    if (!js) return {};
    const char* chars = env->GetStringUTFChars(js, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(js, chars);
    return result;
}

// ── Exported JNI functions ────────────────────────────────────────────────────

extern "C" {

JNIEXPORT jint JNICALL
Java_com_tomodachi_recomp_TomodachiNative_install(
    JNIEnv* env, jobject /*thiz*/,
    jstring nspPath, jstring keysDir,
    jstring dataDir, jstring externalDir)
{
    const auto nsp  = jstring_to_std(env, nspPath);
    const auto keys = jstring_to_std(env, keysDir);
    const auto data = jstring_to_std(env, dataDir);
    const auto ext  = jstring_to_std(env, externalDir);

    LOGI("install: nsp=%s keys=%s data=%s ext=%s",
         nsp.c_str(), keys.c_str(), data.c_str(), ext.c_str());

    return tomodachi_android_install(nsp.c_str(), keys.c_str(),
                                     data.c_str(), ext.c_str());
}

JNIEXPORT void JNICALL
Java_com_tomodachi_recomp_TomodachiNative_onWindowFocusChanged(
    JNIEnv* /*env*/, jobject /*thiz*/, jboolean hasFocus)
{
    tomodachi_android_set_focus(static_cast<bool>(hasFocus));
}

JNIEXPORT jstring JNICALL
Java_com_tomodachi_recomp_TomodachiNative_nativeVersion(
    JNIEnv* env, jobject /*thiz*/)
{
    return env->NewStringUTF(tomodachi_android_version());
}

} // extern "C"
