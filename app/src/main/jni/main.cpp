#include <list>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <cstring>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>
#include "KittyMemory/MemoryPatch.h"
#include "Includes/Utils.h"
#include "Icon.h"

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

#define libName "libil2cpp.so"

struct My_Patches {   
    MemoryPatch GodMode;
} hexPatches; 

// ===== Global Context =====
static jobject gContext = nullptr;
static JavaVM* gJvm = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

// Dipanggil dari Loader.java setelah .so loaded dan context tersedia
extern "C" JNIEXPORT void JNICALL
Java_com_android_support_Loader_initNativeContext(JNIEnv* env, jclass clazz, jobject context) {
    if (gContext != nullptr) {
        env->DeleteGlobalRef(gContext);
    }
    gContext = env->NewGlobalRef(context);
}

// ===== Helper Functions =====
static bool fileExists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

static bool copyFile(const char *src, const char *dst) {
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    std::ofstream out(dst, std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return in && out;
}

// ===== Get External Files Dir via JNI =====
static std::string getExternalFilesDir() {
    if (gJvm == nullptr || gContext == nullptr) return "";

    JNIEnv* env = nullptr;
    bool attached = false;
    int status = gJvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        gJvm->AttachCurrentThread(&env, nullptr);
        attached = true;
    }
    if (env == nullptr) return "";

    jclass contextClass = env->GetObjectClass(gContext);
    jmethodID getExternalFilesDir = env->GetMethodID(contextClass,
        "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");

    jobject filesDir = env->CallObjectMethod(gContext, getExternalFilesDir, nullptr);

    if (filesDir == nullptr) {
        jmethodID getFilesDir = env->GetMethodID(contextClass, "getFilesDir", "()Ljava/io/File;");
        filesDir = env->CallObjectMethod(gContext, getFilesDir);
    }

    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring pathStr = (jstring)env->CallObjectMethod(filesDir, getAbsolutePath);

    const char* path = env->GetStringUTFChars(pathStr, nullptr);
    std::string result(path);
    env->ReleaseStringUTFChars(pathStr, path);

    if (attached) gJvm->DetachCurrentThread();

    return result;
}

// ===== Backup/Restore =====
static void backupInventory() {
    std::string basePath = getExternalFilesDir();
    if (basePath.empty()) {
        printf("backupInventory: context not ready\n");
        return;
    }

    std::string backupDir = basePath + "/backup/";
    mkdir(backupDir.c_str(), 0777);

    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    int count = 0;
    for (const char *slot : slots) {
        std::string src = basePath + "/" + slot + "/the_inventory";
        std::string dst = backupDir + slot + "_the_inventory";
        if (fileExists(src.c_str())) {
            if (copyFile(src.c_str(), dst.c_str())) {
                printf("Backup %s: OK\n", slot);
                count++;
            } else {
                printf("Backup %s: FAILED\n", slot);
            }
        } else {
            printf("Backup %s: SKIPPED\n", slot);
        }
    }
    printf("Backup done: %d/3 | %s\n", count, basePath.c_str());
}

static void loadBackup() {
    std::string basePath = getExternalFilesDir();
    if (basePath.empty()) {
        printf("loadBackup: context not ready\n");
        return;
    }

    std::string backupDir = basePath + "/backup/";
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    int count = 0;
    for (const char *slot : slots) {
        std::string src = backupDir + slot + "_the_inventory";
        std::string dst = basePath + "/" + slot + "/the_inventory";
        if (fileExists(src.c_str())) {
            if (copyFile(src.c_str(), dst.c_str())) {
                printf("Restore %s: OK\n", slot);
                count++;
            } else {
                printf("Restore %s: FAILED\n", slot);
            }
        } else {
            printf("Restore %s: SKIPPED\n", slot);
        }
    }
    printf("Restore done: %d/3 | %s\n", count, basePath.c_str());
}

extern "C" {	
    JNIEXPORT jstring JNICALL
    Java_com_android_support_Loader_setTitleText(JNIEnv *env, jobject obj) {
        return env->NewStringUTF("Hybrid Animals");
    }

    JNIEXPORT jstring JNICALL
    Java_com_android_support_Loader_setHeadingText(JNIEnv *env, jobject obj) {
        return env->NewStringUTF("Dupe Item Tool");
    }

    JNIEXPORT jobjectArray JNICALL
    Java_com_android_support_Loader_GetFeatureList(JNIEnv *env, jobject obj) {
        const char *features[] = {
            "Button_Backup Inventory",
            "Button_Load Backup",
            "WhatsApp_6281241462583_Chat Owner",  // ganti 62xxxxxxxx dengan nomor WA lo
            "Hide_Icon invisible",
            "Close_Close menu",
        };

        int total = sizeof(features) / sizeof(features[0]);
        jobjectArray ret = env->NewObjectArray(
            total,
            env->FindClass("java/lang/String"),
            env->NewStringUTF("")
        );
        for (int i = 0; i < total; i++) {
            env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
        }
        return ret;
    } 

    JNIEXPORT void JNICALL
    Java_com_android_support_Loader_Changes(JNIEnv *env, jobject obj, jint feature, jint value) {
        switch (feature) {
            case 0: backupInventory(); break;
            case 1: loadBackup(); break;
            default: break;
        }
    }
}

void *hack_thread(void *) {
    do { sleep(1); } while (!isLibraryLoaded(libName));
    return NULL;
}
    
__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}
