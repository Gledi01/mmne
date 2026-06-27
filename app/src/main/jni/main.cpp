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

// ===== AUTO DETECT GAME FOLDER! =====
static std::string getInternalFilesDir(JNIEnv* env, jobject context) {
    jclass contextClass = env->GetObjectClass(context);
    jmethodID getFilesDir = env->GetMethodID(contextClass, "getFilesDir", "()Ljava/io/File;");
    jobject filesDir = env->CallObjectMethod(context, getFilesDir);
    
    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring pathStr = (jstring)env->CallObjectMethod(filesDir, getAbsolutePath);
    
    const char* path = env->GetStringUTFChars(pathStr, nullptr);
    std::string result(path);
    env->ReleaseStringUTFChars(pathStr, path);
    
    return result;
}

// ===== Backup/Restore Functions =====
static void backupInventory(JNIEnv* env, jobject context) {
    std::string basePath = getInternalFilesDir(env, context);
    std::string backupDir = basePath + "/backup/";
    
    mkdir(backupDir.c_str(), 0777);
    
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    int backupCount = 0;
    
    for (const char *slot : slots) {
        std::string src = basePath + "/" + slot + "/the_inventory";
        std::string dst = backupDir + slot + "_the_inventory";
        
        if (fileExists(src.c_str())) {
            if (copyFile(src.c_str(), dst.c_str())) {
                printf("Backup %s: OK\n", slot);
                backupCount++;
            } else {
                printf("Backup %s: FAILED\n", slot);
            }
        } else {
            printf("Backup %s: SKIPPED (no inventory)\n", slot);
        }
    }
    
    printf("Backup complete: %d/%d slots\n", backupCount, 3);
}

static void loadBackup(JNIEnv* env, jobject context) {
    std::string basePath = getInternalFilesDir(env, context);
    std::string backupDir = basePath + "/backup/";
    
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    int restoreCount = 0;
    
    for (const char *slot : slots) {
        std::string src = backupDir + slot + "_the_inventory";
        std::string dst = basePath + "/" + slot + "/the_inventory";
        
        if (fileExists(src.c_str())) {
            if (copyFile(src.c_str(), dst.c_str())) {
                printf("Restore %s: OK\n", slot);
                restoreCount++;
            } else {
                printf("Restore %s: FAILED\n", slot);
            }
        } else {
            printf("Restore %s: SKIPPED (backup not found)\n", slot);
        }
    }
    
    printf("Restore complete: %d/%d slots\n", restoreCount, 3);
}
// ===== End Backup/Restore =====

extern "C" {	
	
    JNIEXPORT jstring JNICALL
    Java_com_android_support_Loader_setTitleText(
        JNIEnv *env,
        jobject activityObject) {
        jstring str = env->NewStringUTF("Modded by FrostyDev");
        return str;
    }

    JNIEXPORT jstring JNICALL
    Java_com_android_support_Loader_setHeadingText(
        JNIEnv *env,
        jobject activityObject) {
        jstring str = env->NewStringUTF("Mod Dupe Item | Hybrid Animals");
        return str;
    }

    JNIEXPORT jobjectArray JNICALL
    Java_com_android_support_Loader_GetFeatureList(
        JNIEnv *env,
        jobject activityObject) {
        jobjectArray ret;
        const char *features[] = {          	   
            "Button_Backup Inventory",           // 0
            "Button_Load Backup",                // 1
            "Hide_Icon invisible",               // 2
            "Close_Close menu",                  // 3
        };

        int Total_Feature = (sizeof features / sizeof features[0]); 
        ret = (jobjectArray) env->NewObjectArray(
            Total_Feature, 
            env->FindClass("java/lang/String"),
            env->NewStringUTF("")
        );
        
        for (int i = 0; i < Total_Feature; i++) {
            env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
        }
        return (ret);
    } 

    JNIEXPORT void JNICALL
    Java_com_android_support_Loader_Changes(
        JNIEnv *env,
        jobject activityObject,
        jint feature,
        jint value) {
        
        jobject context = activityObject;
        
        switch (feature) {
            case 0:  // Backup Inventory
                backupInventory(env, context);
                break;
            case 1:  // Load Backup
                loadBackup(env, context);
                break;
            case 2:  // Hide Icon (toggle)
                // Handle di Java
                break;
            case 3:  // Close menu
                // Handle di Java
                break;
            default:
                break;
        }
    }
}

void *hack_thread(void *) {
    do {
        sleep(1);
    } while (!isLibraryLoaded(libName));
    return NULL;
}
    
__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}
