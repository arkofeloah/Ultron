#include <jni.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <cstring>
#include <android/log.h>
#include <chrono>
#include "llama.h"

// Global model pointer
static llama_model* g_model = nullptr;

// Route llama.cpp logs to Android Logcat
static void llama_log_callback(enum ggml_log_level level, const char * text, void * user_data) {
    int priority = ANDROID_LOG_INFO;
    switch (level) {
        case GGML_LOG_LEVEL_DEBUG: priority = ANDROID_LOG_DEBUG; break;
        case GGML_LOG_LEVEL_INFO:  priority = ANDROID_LOG_INFO;  break;
        case GGML_LOG_LEVEL_WARN:  priority = ANDROID_LOG_WARN;  break;
        case GGML_LOG_LEVEL_ERROR: priority = ANDROID_LOG_ERROR; break;
        default:                   priority = ANDROID_LOG_VERBOSE;
    }
    __android_log_print(priority, "ULTRON_LLAMA", "%s", text);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ultron_NativeBridge_helloFromNative(
        JNIEnv *env,
        jobject /* this */) {
    return env->NewStringUTF("JNI OK");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ultron_NativeBridge_initializeLlamaBackend(
        JNIEnv *env,
        jobject /* this */) {
    llama_log_set(llama_log_callback, nullptr);
    llama_backend_init();
    __android_log_print(ANDROID_LOG_INFO, "ULTRON", "System Info: %s", llama_print_system_info());
    return env->NewStringUTF("LLAMA BACKEND INITIALIZED");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ultron_NativeBridge_checkModelFile(
        JNIEnv *env,
        jobject /* this */,
        jstring modelPath) {
    const char *path = env->GetStringUTFChars(modelPath, nullptr);
    bool exists = (access(path, F_OK) == 0);
    if (exists) {
        env->ReleaseStringUTFChars(modelPath, path);
        return env->NewStringUTF("FILE FOUND");
    } else {
        std::string response = "FILE NOT FOUND: ";
        response += path;
        env->ReleaseStringUTFChars(modelPath, path);
        return env->NewStringUTF(response.c_str());
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ultron_NativeBridge_loadModel(
        JNIEnv *env,
        jobject /* this */,
        jstring modelPath) {

    if (g_model != nullptr) {
        llama_model_free(g_model);
        g_model = nullptr;
    }

    const char *path = env->GetStringUTFChars(modelPath, nullptr);
    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = 0; // Ensure CPU-only loading

    g_model = llama_model_load_from_file(path, mparams);

    env->ReleaseStringUTFChars(modelPath, path);

    if (g_model == nullptr) {
        return env->NewStringUTF("MODEL LOAD FAILED");
    }

    // LOGGING: Check for template presence in metadata
    const char * tmpl = llama_model_chat_template(g_model, nullptr);
    if (tmpl != nullptr) {
        __android_log_print(ANDROID_LOG_INFO, "ULTRON", "Chat template found: %.200s", tmpl);
    } else {
        __android_log_print(ANDROID_LOG_WARN, "ULTRON", "No chat template found in model metadata.");
    }

    return env->NewStringUTF("MODEL LOADED");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ultron_NativeBridge_generate(
        JNIEnv *env,
        jobject /* this */,
        jstring prompt,
        jobject listener) {

    auto t_start_total = std::chrono::high_resolution_clock::now();

    if (g_model == nullptr) {
        return env->NewStringUTF("ERROR: Model not loaded");
    }

    // Cache JNI metadata
    jclass listenerClass = env->GetObjectClass(listener);
    jmethodID onTokenMethod = env->GetMethodID(listenerClass, "onToken", "(Ljava/lang/String;)V");
    jmethodID onTraceUpdateMethod = env->GetMethodID(listenerClass, "onTraceUpdate", "(IJF)V");

    const char *prompt_str = env->GetStringUTFChars(prompt, nullptr);
    std::string prompt_cpp(prompt_str);

    // --- Measure Template Application ---
    auto t_start_template = std::chrono::high_resolution_clock::now();
    const char * tmpl = llama_model_chat_template(g_model, nullptr);

    std::vector<char> formatted_prompt;
    int32_t prompt_len = 0;
    bool use_template = false;

    if (tmpl != nullptr) {
        llama_chat_message msg = {"user", prompt_str};

        int32_t n_chars = llama_chat_apply_template(tmpl, &msg, 1, true, nullptr, 0);
        if (n_chars > 0) {
            formatted_prompt.resize(n_chars + 1);
            int32_t res = llama_chat_apply_template(tmpl, &msg, 1, true, formatted_prompt.data(), (int32_t)formatted_prompt.size());
            if (res > 0) {
                prompt_len = n_chars;
                use_template = true;

                if (prompt_cpp == "Hello") {
                    __android_log_print(ANDROID_LOG_INFO, "ULTRON", "Original prompt: %s", prompt_str);
                    __android_log_print(ANDROID_LOG_INFO, "ULTRON", "Formatted prompt: %s", formatted_prompt.data());
                }
            } else {
                __android_log_print(ANDROID_LOG_WARN, "ULTRON", "llama_chat_apply_template second pass failed, falling back to raw prompt");
            }
        } else {
            __android_log_print(ANDROID_LOG_WARN, "ULTRON", "llama_chat_apply_template first pass failed, falling back to raw prompt");
        }
    } else {
        __android_log_print(ANDROID_LOG_WARN, "ULTRON", "No chat template found, using raw prompt");
    }
    auto t_end_template = std::chrono::high_resolution_clock::now();

    // --- Measure Context Creation ---
    auto t_start_ctx = std::chrono::high_resolution_clock::now();
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 2048;
    cparams.n_threads = 4;
    cparams.n_threads_batch = 4;

    struct llama_context * ctx = llama_init_from_model(g_model, cparams);
    auto t_end_ctx = std::chrono::high_resolution_clock::now();

    if (ctx == nullptr) {
        env->ReleaseStringUTFChars(prompt, prompt_str);
        return env->NewStringUTF("ERROR: Failed to create context");
    }

    const struct llama_vocab * vocab = llama_model_get_vocab(g_model);
    const char * final_prompt_ptr = use_template ? formatted_prompt.data() : prompt_str;
    int32_t final_prompt_len = use_template ? prompt_len : (int32_t)prompt_cpp.length();

    // --- Measure Tokenization ---
    auto t_start_tokenization = std::chrono::high_resolution_clock::now();
    int32_t n_tokens_req = -llama_tokenize(vocab, final_prompt_ptr, final_prompt_len, nullptr, 0, true, true);
    std::vector<llama_token> tokens(n_tokens_req);
    if (llama_tokenize(vocab, final_prompt_ptr, final_prompt_len, tokens.data(), (int32_t)tokens.size(), true, true) < 0) {
        llama_free(ctx);
        env->ReleaseStringUTFChars(prompt, prompt_str);
        return env->NewStringUTF("ERROR: Tokenization failed");
    }
    auto t_end_tokenization = std::chrono::high_resolution_clock::now();

    env->ReleaseStringUTFChars(prompt, prompt_str);

    struct llama_sampler * smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    if (smpl == nullptr) {
        llama_free(ctx);
        return env->NewStringUTF("ERROR: Failed to create sampler");
    }

    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    __android_log_print(ANDROID_LOG_INFO, "ULTRON", "Sampler: top_k=40 top_p=0.95 temp=0.7 seed=%u", LLAMA_DEFAULT_SEED);

    // --- Measure Initial Decode ---
    auto t_start_decode = std::chrono::high_resolution_clock::now();
    struct llama_batch batch = llama_batch_get_one(tokens.data(), (int32_t)tokens.size());
    if (llama_decode(ctx, batch) != 0) {
        llama_sampler_free(smpl);
        llama_free(ctx);
        return env->NewStringUTF("ERROR: Decode failed");
    }
    auto t_end_decode = std::chrono::high_resolution_clock::now();

    // --- Measure Generation ---
    auto t_start_gen = std::chrono::high_resolution_clock::now();
    auto t_last_trace = std::chrono::high_resolution_clock::now();
    std::string response;
    int n_gen = 0;
    llama_token curr_token = llama_sampler_sample(smpl, ctx, -1);
    for (int i = 0; i < 128; i++) {
        if (llama_vocab_is_eog(vocab, curr_token)) break;
        n_gen++;

        char piece_buf[128];
        int n_piece = llama_token_to_piece(vocab, curr_token, piece_buf, sizeof(piece_buf), 0, true);
        std::string piece_str;
        if (n_piece < 0) {
             std::vector<char> big_buf(-n_piece);
             n_piece = llama_token_to_piece(vocab, curr_token, big_buf.data(), (int32_t)big_buf.size(), 0, true);
             piece_str = std::string(big_buf.data(), n_piece);
        } else {
             piece_str = std::string(piece_buf, n_piece);
        }
        response.append(piece_str);

        // Stream token piece
        jstring jpiece = env->NewStringUTF(piece_str.c_str());
        env->CallVoidMethod(listener, onTokenMethod, jpiece);
        env->DeleteLocalRef(jpiece);

        // Throttle trace updates
        auto t_now = std::chrono::high_resolution_clock::now();
        auto d_since_last_trace = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last_trace).count();
        if (d_since_last_trace >= 400) {
            auto d_gen_current = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_start_gen).count();
            float tps = d_gen_current > 0 ? (float)n_gen * 1000.0f / (float)d_gen_current : 0.0f;
            env->CallVoidMethod(listener, onTraceUpdateMethod, (jint)n_gen, (jlong)d_gen_current, (jfloat)tps);
            t_last_trace = t_now;
        }

        batch = llama_batch_get_one(&curr_token, 1);
        if (llama_decode(ctx, batch) != 0) break;
        curr_token = llama_sampler_sample(smpl, ctx, -1);
    }
    auto t_end_gen = std::chrono::high_resolution_clock::now();
    auto t_end_total = std::chrono::high_resolution_clock::now();

    // Final trace update
    auto d_gen_total = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_gen - t_start_gen).count();
    float tps_final = d_gen_total > 0 ? (float)n_gen * 1000.0f / (float)d_gen_total : 0.0f;
    env->CallVoidMethod(listener, onTraceUpdateMethod, (jint)n_gen, (jlong)d_gen_total, (jfloat)tps_final);

    // --- Logging Performance Results ---
    auto d_template = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_template - t_start_template).count();
    auto d_ctx = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_ctx - t_start_ctx).count();
    auto d_tok = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_tokenization - t_start_tokenization).count();
    auto d_dec = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_decode - t_start_decode).count();
    auto d_gen = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_gen - t_start_gen).count();
    auto d_total = std::chrono::duration_cast<std::chrono::milliseconds>(t_end_total - t_start_total).count();

    __android_log_print(ANDROID_LOG_INFO, "ULTRON_PERF", "Template: %lld ms", (long long)d_template);
    __android_log_print(ANDROID_LOG_INFO, "ULTRON_PERF", "Context: %lld ms", (long long)d_ctx);
    __android_log_print(ANDROID_LOG_INFO, "ULTRON_PERF", "Tokenization: %lld ms (%d tokens)", (long long)d_tok, (int)tokens.size());
    __android_log_print(ANDROID_LOG_INFO, "ULTRON_PERF", "Initial Decode: %lld ms", (long long)d_dec);
    __android_log_print(ANDROID_LOG_INFO, "ULTRON_PERF", "Generation: %lld ms for %d tokens (Avg: %.2f ms/tok)",
                        (long long)d_gen, n_gen, n_gen > 0 ? (double)d_gen / n_gen : 0);
    __android_log_print(ANDROID_LOG_INFO, "ULTRON_PERF", "Total Inference: %lld ms", (long long)d_total);

    llama_sampler_free(smpl);
    llama_free(ctx);

    return env->NewStringUTF(response.c_str());
}
