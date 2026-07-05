#include <jni.h>

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ultron_NativeBridge_helloFromNative(
        JNIEnv *env,
        jobject /* this */) {

    return env->NewStringUTF("JNI OK");
}