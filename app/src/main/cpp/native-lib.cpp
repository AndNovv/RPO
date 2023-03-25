#include <jni.h>
#include <string>
#include <android/log.h>
#include <C:\\Users\andre\AndroidStudioProjects\libs\spdlog\spdlog\include\spdlog\spdlog.h>
#include <C:\\Users\andre\AndroidStudioProjects\libs\spdlog\spdlog\include\spdlog\sinks\android_sink.h>
#include <C:\\Users\andre\AndroidStudioProjects\libs\mbedtls\mbedtls\include\mbedtls\entropy.h>
#include <C:\\Users\andre\AndroidStudioProjects\libs\mbedtls\mbedtls\include\mbedtls\ctr_drbg.h>
#include <C:\\Users\andre\AndroidStudioProjects\libs\mbedtls\mbedtls\include\mbedtls\des.h>

mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
char *personalization = "fclient-sample-app";

#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "fclient_ndk", __VA_ARGS__)

#define SLOG_INFO(...) android_logger->info( __VA_ARGS__ )
auto android_logger = spdlog::android_logger_mt("android", "fclient_ndk");

JavaVM* gJvm = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM* pjvm, void* reserved)
{
    gJvm = pjvm;
    return JNI_VERSION_1_6;
}

JNIEnv* getEnv (bool& detach)
{
    JNIEnv* env = nullptr;
    int status = gJvm->GetEnv ((void**)&env, JNI_VERSION_1_6);
    detach = false;
    if (status == JNI_EDETACHED)
    {
        status = gJvm->AttachCurrentThread (&env, NULL);
        if (status < 0)
        {
            return nullptr;
        }
        detach = true;
    }
    return env;
}

void releaseEnv (bool detach, JNIEnv* env)
{
    if (detach && (gJvm != nullptr))
    {
        gJvm->DetachCurrentThread ();
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_ru_iu3_fclient_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "String from JNI";
    LOG_INFO("Hello from c++ %d", 2023);
    SLOG_INFO("Hello from spdlog {0}", 2023);
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_ru_iu3_fclient_MainActivity_initRng(JNIEnv *env, jclass clazz) {
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    return mbedtls_ctr_drbg_seed( &ctr_drbg , mbedtls_entropy_func, &entropy,
                                  (const unsigned char *) personalization,
                                  strlen( personalization ) );
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_randomBytes(JNIEnv *env, jclass, jint no) {
    uint8_t * buf = new uint8_t [no];
    mbedtls_ctr_drbg_random(&ctr_drbg, buf, no);
    jbyteArray rnd = env->NewByteArray(no);
    env->SetByteArrayRegion(rnd, 0, no, (jbyte *)buf);
    delete[] buf;
    return rnd;
}

// https://tls.mbed.org/api/des_8h.html
extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_encrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data)
{
    jsize ksz = env->GetArrayLength(key);
    jsize dsz = env->GetArrayLength(data);
    if ((ksz != 16) || (dsz <= 0)) {
        return env->NewByteArray(0);
    }
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte * pkey = env->GetByteArrayElements(key, 0);

    // Паддинг PKCS#5
    int rst = dsz % 8;
    int sz = dsz + 8 - rst;
    uint8_t * buf = new uint8_t[sz];
    for (int i = 7; i > rst; i--)
        buf[dsz + i] = rst;
    jbyte * pdata = env->GetByteArrayElements(data, 0);
    std::copy(pdata, pdata + dsz, buf);
    mbedtls_des3_set2key_enc(&ctx, (uint8_t *)pkey);
    int cn = sz / 8;
    for (int i = 0; i < cn; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i*8, buf + i*8);
    jbyteArray dout = env->NewByteArray(sz);
    env->SetByteArrayRegion(dout, 0, sz, (jbyte *)buf);
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, 0);
    env->ReleaseByteArrayElements(data, pdata, 0);
    return dout;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_decrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data)
{
    jsize ksz = env->GetArrayLength(key);
    jsize dsz = env->GetArrayLength(data);
    if ((ksz != 16) || (dsz <= 0) || ((dsz % 8) != 0)) {
        return env->NewByteArray(0);
    }
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte * pkey = env->GetByteArrayElements(key, 0);

    uint8_t * buf = new uint8_t[dsz];

    jbyte * pdata = env->GetByteArrayElements(data, 0);
    std::copy(pdata, pdata + dsz, buf);
    mbedtls_des3_set2key_dec(&ctx, (uint8_t *)pkey);
    int cn = dsz / 8;
    for (int i = 0; i < cn; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i*8, buf +i*8);

    //PKCS#5. упрощено. по соображениям безопасности надо проверить каждый байт паддинга
    int sz = dsz - 8 + buf[dsz-1];

    jbyteArray dout = env->NewByteArray(sz);
    env->SetByteArrayRegion(dout, 0, sz, (jbyte *)buf);
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, 0);
    env->ReleaseByteArrayElements(data, pdata, 0);
    return dout;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_ru_iu3_fclient_MainActivity_transaction(JNIEnv *xenv, jobject xthiz, jbyteArray xtrd) {
    jobject thiz = xenv->NewGlobalRef(xthiz);
    jbyteArray trd = (jbyteArray) xenv->NewGlobalRef(xtrd);

    std::thread t([thiz, trd] {
        bool detach = false;
        JNIEnv *env = getEnv(detach);
        jclass cls = env->GetObjectClass(thiz);
        jmethodID id = env->GetMethodID(cls, "enterPin", "(ILjava/lang/String;)Ljava/lang/String;");
        //TRD 9F0206000000000100 = amount = 1р
        uint8_t *p = (uint8_t *) env->GetByteArrayElements(trd, 0);
        jsize sz = env->GetArrayLength(trd);
        if ((sz != 9) || (p[0] != 0x9F) || (p[1] != 0x02) || (p[2] != 0x06))
            return false;

        char buf[13];
        for (int i = 0; i < 6; i++) {
            uint8_t n = *(p + 3 + i);
            buf[i * 2] = ((n & 0xF0) >> 4) + '0';
            buf[i * 2 + 1] = (n & 0x0F) + '0';
        }
        buf[12] = 0x00;
        jstring jamount = (jstring) env->NewStringUTF(buf);

        int ptc = 3;
        while (ptc > 0) {
            jstring pin = (jstring) env->CallObjectMethod(thiz, id, ptc, jamount);
            const char *utf = env->GetStringUTFChars(pin, nullptr);
            env->ReleaseStringUTFChars(pin, utf);
            if ((utf != nullptr) && (strcmp(utf, "1234") == 0))
                break;
            ptc--;
        }

        id = env->GetMethodID(cls, "transactionResult", "(Z)V");
        env->CallVoidMethod(thiz, id, ptc > 0);
        env->ReleaseByteArrayElements(trd, (jbyte *) p, 0);
        env->DeleteGlobalRef(thiz);
        env->DeleteGlobalRef(trd);
        releaseEnv(detach, env);
        return true;
    });

    t.detach();
    return true;
}
