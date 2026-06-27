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
#include <stdio.h>  // Tambahin buat printf
#include "KittyMemory/MemoryPatch.h"
#include "Includes/Utils.h"
#include "Icon.h"
	 
#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

#define libName "libil2cpp.so"

struct My_Patches {   
    MemoryPatch GodMode;
} hexPatches; 

// ===== Backup/Restore Functions =====
static bool copyFile(const char *src, const char *dst) {
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    std::ofstream out(dst, std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return in && out;
}

static void backupInventory() {
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    const char *base = "/data/data/com.abstractsoft.hybridanimals/files/";
    std::string backupDir = std::string(base) + "backup/";
    mkdir(backupDir.c_str(), 0777);
    
    for (const char *slot : slots) {
        std::string src = std::string(base) + slot + "/the_inventory";
        std::string dst = backupDir + slot + "_the_inventory";
        bool ok = copyFile(src.c_str(), dst.c_str());
        printf("Backup %s: %s\n", slot, ok ? "OK" : "FAILED");
    }
}

static void loadBackup() {
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    const char *base = "/data/data/com.abstractsoft.hybridanimals/files/";
    std::string backupDir = std::string(base) + "backup/";
    
    for (const char *slot : slots) {
        std::string src = backupDir + slot + "_the_inventory";
        std::string dst = std::string(base) + slot + "/the_inventory";
        bool ok = copyFile(src.c_str(), dst.c_str());
        printf("Restore %s: %s\n", slot, ok ? "OK" : "FAILED");
    }
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
        switch (feature) {
            case 0:  // Backup Inventory
                backupInventory();
                break;
            case 1:  // Load Backup
                loadBackup();
                break;
            case 2:  // Hide Icon (toggle)
                // Fungsi hide icon - nanti dihandle di Java
                break;
            case 3:  // Close menu
                // Fungsi close - nanti dihandle di Java
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
