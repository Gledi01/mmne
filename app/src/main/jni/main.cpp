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

extern "C" JNIEXPORT void JNICALL
Java_com_android_support_Loader_initNativeContext(JNIEnv* env, jclass clazz, jobject context) {
    if (gContext != nullptr) {
        env->DeleteGlobalRef(gContext);
    }
    gContext = env->NewGlobalRef(context);
    printf("[MOD] Context initialized\n");
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
        printf("[MOD] backupInventory: context not ready\n");
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
                printf("[MOD] Backup %s: OK\n", slot);
                count++;
            }
        }
    }
    printf("[MOD] Backup complete: %d/3\n", count);
}

static void loadBackup() {
    std::string basePath = getExternalFilesDir();
    if (basePath.empty()) {
        printf("[MOD] loadBackup: context not ready\n");
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
                printf("[MOD] Restore %s: OK\n", slot);
                count++;
            }
        }
    }
    printf("[MOD] Restore complete: %d/3\n", count);
}

// ===== Hook Typedefs =====
typedef void (*OverwriteCurrentExp_t)(void* instance, int new_value, const char* validator);
typedef void (*OverwritePlayerLevel_t)(void* instance, int new_value, const char* validator);
typedef void (*OverwriteSkillPointsSpendable_t)(void* instance, int new_value, const char* validator);
typedef void (*GiveItem_t)(void* instance, const char* item_name, int count, const char* validator, bool visual);
typedef void (*GenerateLootChest_t)(void* instance);

static OverwriteCurrentExp_t orig_OverwriteCurrentExp = nullptr;
static OverwritePlayerLevel_t orig_OverwritePlayerLevel = nullptr;
static OverwriteSkillPointsSpendable_t orig_OverwriteSkillPointsSpendable = nullptr;
static GiveItem_t orig_GiveItem = nullptr;
static GenerateLootChest_t orig_GenerateLootChest = nullptr;

static void* gGameControllerInstance = nullptr;

// Find instance via singleton pattern
static void* findGameControllerInstance() {
    if (gGameControllerInstance != nullptr) return gGameControllerInstance;
    // Biasanya Instance adalah static field di class
    // Gw asumin sudah ada via singleton, return nullptr jika belum ditemukan
    return nullptr;
}

extern "C" {	
    JNIEXPORT jstring JNICALL
    Java_com_android_support_Loader_setTitleText(JNIEnv *env, jobject obj) {
        return env->NewStringUTF("Mod By FrostyDev");
    }

    JNIEXPORT jstring JNICALL
    Java_com_android_support_Loader_setHeadingText(JNIEnv *env, jobject obj) {
        return env->NewStringUTF("Advanced Mod Menu");
    }

    JNIEXPORT jobjectArray JNICALL
    Java_com_android_support_Loader_GetFeatureList(JNIEnv *env, jobject obj) {
        const char *features[] = {
            "Text_🔧 STATS SECTION",
            "Text_━━━━━━━━━━━━━━",
            "Text_💾 INVENTORY BACKUP/RESTORE",
            "Text_📦 ITEM GENERATOR",
            "Text_🎲 RANDOM LOOT",
            "Text_⚙️ CONFIG",
            "WhatsApp_6281241462583_Chat Owner",
        };

        int total = sizeof(features) / sizeof(features[0]);
        jobjectArray ret = env->NewObjectArray(total, env->FindClass("java/lang/String"), env->NewStringUTF(""));
        for (int i = 0; i < total; i++) {
            env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
        }
        return ret;
    }

    JNIEXPORT void JNICALL
    Java_com_android_support_Loader_Changes(JNIEnv *env, jobject obj, jint feature, jint value) {
        printf("[MOD] Changes called: feature=%d, value=%d\n", feature, value);

        // Feature 0 = Set EXP
        // Feature 1 = Set Level  
        // Feature 2 = Set Skill Points
        // Feature 3 = Give Item (value = count)
        // Feature 4 = Generate Loot
        // Feature 100 = Backup Inventory
        // Feature 101 = Restore Inventory

        switch (feature) {
            case 0: // EXP
                if (orig_OverwriteCurrentExp) {
                    void* instance = findGameControllerInstance();
                    if (instance) {
                        orig_OverwriteCurrentExp(instance, value, "MOD");
                        printf("[MOD] EXP set to %d\n", value);
                    }
                }
                break;

            case 1: // Level
                if (orig_OverwritePlayerLevel) {
                    void* instance = findGameControllerInstance();
                    if (instance) {
                        orig_OverwritePlayerLevel(instance, value, "MOD");
                        printf("[MOD] Level set to %d\n", value);
                    }
                }
                break;

            case 2: // Skill Points
                if (orig_OverwriteSkillPointsSpendable) {
                    void* instance = findGameControllerInstance();
                    if (instance) {
                        orig_OverwriteSkillPointsSpendable(instance, value, "MOD");
                        printf("[MOD] Skill Points set to %d\n", value);
                    }
                }
                break;

            case 3: // Give Item
                // Nama item di-pass via static variable dari Java side
                // Untuk simplifikasi, gw pake default item names
                if (orig_GiveItem) {
                    void* instance = findGameControllerInstance();
                    if (instance) {
                        // Coba beberapa item name yang umum
                        const char* items[] = {"coin", "gem", "wood", "stone"};
                        orig_GiveItem(instance, items[0], value, "MOD", true);
                        printf("[MOD] Item given: count=%d\n", value);
                    }
                }
                break;

            case 4: // Generate Loot
                if (orig_GenerateLootChest) {
                    void* instance = findGameControllerInstance();
                    if (instance) {
                        orig_GenerateLootChest(instance);
                        printf("[MOD] Loot generated\n");
                    }
                }
                break;

            case 100: // Backup
                backupInventory();
                break;

            case 101: // Restore
                loadBackup();
                break;

            default:
                break;
        }
    }
}

void *hack_thread(void *) {
    do { sleep(1); } while (!isLibraryLoaded(libName));

    // Hook functions setelah libil2cpp loaded
    void* handle = dlopen(libName, RTLD_LAZY);
    if (handle) {
        // RVA relative ke offset dalam libil2cpp.so
        void* base = (void*)((uintptr_t)handle);
        
        // Hook OverwriteCurrentExp - RVA: 0xCF4250
        orig_OverwriteCurrentExp = (OverwriteCurrentExp_t)((uintptr_t)base + 0xCF4250);
        
        // Hook OverwritePlayerLevel - RVA: 0xCE8D2C
        orig_OverwritePlayerLevel = (OverwritePlayerLevel_t)((uintptr_t)base + 0xCE8D2C);
        
        // Hook OverwriteSkillPointsSpendable - RVA: 0xCF4198
        orig_OverwriteSkillPointsSpendable = (OverwriteSkillPointsSpendable_t)((uintptr_t)base + 0xCF4198);
        
        // Hook GiveItem - RVA: 0xB56710
        orig_GiveItem = (GiveItem_t)((uintptr_t)base + 0xB56710);
        
        // Hook GenerateLootChest - RVA: 0xB56794
        orig_GenerateLootChest = (GenerateLootChest_t)((uintptr_t)base + 0xB56794);

        printf("[MOD] Hooks installed\n");
    }

    return NULL;
}
    
__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}
