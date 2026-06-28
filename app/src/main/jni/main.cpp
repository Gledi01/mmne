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

static JavaVM* gJvm     = nullptr;
static jobject gContext = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_support_Loader_initNativeContext(JNIEnv* env, jclass clazz, jobject context) {
    if (gContext) env->DeleteGlobalRef(gContext);
    gContext = env->NewGlobalRef(context);
}

// ===== Toast Helper — aman dari background thread =====
static void showToast(const char* msg) {
    if (!gJvm || !gContext) return;
    JNIEnv* env = nullptr;
    bool att = false;
    if (gJvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        gJvm->AttachCurrentThread(&env, nullptr);
        att = true;
    }
    if (!env) return;

    // Pastikan thread ini punya Looper (Toast butuh Looper)
    jclass looperCls = env->FindClass("android/os/Looper");
    if (looperCls) {
        jmethodID myLooper = env->GetStaticMethodID(looperCls, "myLooper", "()Landroid/os/Looper;");
        jobject looper = env->CallStaticObjectMethod(looperCls, myLooper);
        if (!looper) {
            jmethodID prepare = env->GetStaticMethodID(looperCls, "prepare", "()V");
            env->CallStaticVoidMethod(looperCls, prepare);
            env->ExceptionClear();
        }
    }
    env->ExceptionClear();

    jclass toastCls = env->FindClass("android/widget/Toast");
    if (toastCls) {
        jmethodID makeText = env->GetStaticMethodID(toastCls, "makeText",
            "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
        jmethodID show = env->GetMethodID(toastCls, "show", "()V");
        jstring jmsg = env->NewStringUTF(msg);
        jobject toast = env->CallStaticObjectMethod(toastCls, makeText, gContext, jmsg, (jint)0);
        if (toast) { env->CallVoidMethod(toast, show); env->DeleteLocalRef(toast); }
        env->DeleteLocalRef(jmsg);
    }
    env->ExceptionClear();

    if (att) gJvm->DetachCurrentThread();
}


// ===== IL2CPP String =====
typedef void* (*il2cpp_string_new_t)(const char*);
static il2cpp_string_new_t fn_il2cpp_string_new = nullptr;
static void* mkstr(const char* s) {
    return fn_il2cpp_string_new ? fn_il2cpp_string_new(s) : nullptr;
}

// ===== IL2CPP API =====
typedef void* (*dom_get_t)();
typedef void* (*dom_asm_open_t)(void*, const char*);
typedef void* (*asm_get_image_t)(void*);
typedef void* (*class_from_name_t)(void*, const char*, const char*);
typedef void* (*get_field_t)(void*, const char*);
typedef void  (*field_static_get_t)(void*, void*);

static dom_get_t          il2cpp_domain_get                = nullptr;
static dom_asm_open_t     il2cpp_domain_assembly_open      = nullptr;
static asm_get_image_t    il2cpp_assembly_get_image        = nullptr;
static class_from_name_t  il2cpp_class_from_name           = nullptr;
static get_field_t        il2cpp_class_get_field_from_name = nullptr;
static field_static_get_t il2cpp_field_static_get_value    = nullptr;
static void* gImage = nullptr;

// ===== Method typedefs =====
typedef void (*OverwriteCurrentExp_t)(void*, int, void*, void*);
typedef void (*OverwritePlayerLevel_t)(void*, int, void*, void*);
typedef void (*OverwriteSkillPoints_t)(void*, int, void*, void*);
typedef void (*GiveItemStr_t)(void*, void*, int, void*, bool, void*);
typedef void* (*GenerateLootChest_t)(void*, void*, void*);
typedef void  (*AddManyLoots_t)(void*, int, int, void*, int, int, void*, void*, void*);
typedef int   (*TryRollCreature_t)(void*, void*);
typedef void  (*IncreaseCompanionExp_t)(void*, int, void*, void*);
typedef void  (*CycleInvincibility_t)(void*, void*);
typedef void  (*CycleInstaKill_t)(void*, void*);
typedef void  (*CycleEnemiesCanSeeYou_t)(void*, void*);
typedef void  (*SetLevel_t)(void*, int, void*);

static OverwriteCurrentExp_t   fn_OverwriteExp      = nullptr;
static OverwritePlayerLevel_t  fn_OverwriteLevel    = nullptr;
static OverwriteSkillPoints_t  fn_OverwriteSkillPts = nullptr;
static GiveItemStr_t           fn_GiveItem          = nullptr;
static GenerateLootChest_t     fn_GenLootChest      = nullptr;
static AddManyLoots_t          fn_AddManyLoots      = nullptr;
static TryRollCreature_t       fn_TryRollCreature   = nullptr;
static IncreaseCompanionExp_t  fn_IncCompanionExp   = nullptr;
static CycleInvincibility_t    fn_CycleInvincible   = nullptr;
static CycleInstaKill_t        fn_CycleInstaKill    = nullptr;
static CycleEnemiesCanSeeYou_t fn_CycleEnemySee     = nullptr;
static SetLevel_t              fn_SetLevel          = nullptr;

static bool gLibReady = false;

// ===== Instances =====
static void* inst_GameController      = nullptr;
static void* inst_inventory_ctr       = nullptr;
static void* inst_LootControl         = nullptr;
static void* inst_RewardsControl      = nullptr;
static void* inst_CompanionController = nullptr;
static void* inst_CreativeMode        = nullptr;
static void* inst_ConsoleControl      = nullptr;

static void* resolveInstance(const char* className) {
    if (!gImage || !il2cpp_class_from_name || !il2cpp_class_get_field_from_name || !il2cpp_field_static_get_value)
        return nullptr;
    void* cls = il2cpp_class_from_name(gImage, "", className);
    if (!cls) return nullptr;
    void* f = il2cpp_class_get_field_from_name(cls, "Instance");
    if (!f) return nullptr;
    void* inst = nullptr;
    il2cpp_field_static_get_value(f, &inst);
    return inst;
}

static void refreshInstances() {
    inst_GameController      = resolveInstance("GameController");
    inst_inventory_ctr       = resolveInstance("inventory_ctr");
    inst_LootControl         = resolveInstance("LootControl");
    inst_RewardsControl      = resolveInstance("RewardsControl");
    inst_CompanionController = resolveInstance("CompanionController");
    inst_CreativeMode        = resolveInstance("CreativeModeControl");
    inst_ConsoleControl      = resolveInstance("ConsoleControl");
}

// ===== Saved values =====
static int val_exp      = 0;
static int val_level    = 1;
static int val_skillpts = 0;
static int val_coin     = 0;
static int val_gem      = 0;
static int val_wood     = 0;
static int val_stone    = 0;
static int val_comp_exp = 0;
static int val_pet_lvl  = 1;

#define CHECK_READY() if (!gLibReady) { showToast("Tunggu! Mod belum siap..."); return; }

static void doSetExp(int v)      { CHECK_READY(); refreshInstances(); if (fn_OverwriteExp      && inst_GameController)      fn_OverwriteExp(inst_GameController, v, nullptr, nullptr); }
static void doSetLevel(int v)    { CHECK_READY(); refreshInstances(); if (fn_OverwriteLevel    && inst_GameController)      fn_OverwriteLevel(inst_GameController, v, nullptr, nullptr); }
static void doSetSkillPts(int v) { CHECK_READY(); refreshInstances(); if (fn_OverwriteSkillPts && inst_GameController)      fn_OverwriteSkillPts(inst_GameController, v, nullptr, nullptr); }
static void doGenLootChest()     { CHECK_READY(); refreshInstances(); if (fn_GenLootChest      && inst_LootControl)         fn_GenLootChest(inst_LootControl, nullptr, nullptr); }
static void doAddManyLoots()     { CHECK_READY(); refreshInstances(); if (fn_AddManyLoots      && inst_LootControl)         fn_AddManyLoots(inst_LootControl, 5, 10, nullptr, 3, 6, nullptr, nullptr, nullptr); }
static void doTryRollCreature()  { CHECK_READY(); refreshInstances(); if (fn_TryRollCreature   && inst_RewardsControl)      fn_TryRollCreature(inst_RewardsControl, nullptr); }
static void doCycleInvincible()  { CHECK_READY(); refreshInstances(); if (fn_CycleInvincible   && inst_CreativeMode)        fn_CycleInvincible(inst_CreativeMode, nullptr); }
static void doCycleInstaKill()   { CHECK_READY(); refreshInstances(); if (fn_CycleInstaKill    && inst_CreativeMode)        fn_CycleInstaKill(inst_CreativeMode, nullptr); }
static void doCycleEnemySee()    { CHECK_READY(); refreshInstances(); if (fn_CycleEnemySee     && inst_CreativeMode)        fn_CycleEnemySee(inst_CreativeMode, nullptr); }
static void doSetPetLevel(int v) { CHECK_READY(); refreshInstances(); if (fn_SetLevel          && inst_ConsoleControl)      fn_SetLevel(inst_ConsoleControl, v, nullptr); }

static void doGiveItem(const char* name, int count) {
    CHECK_READY();
    refreshInstances();
    if (fn_GiveItem && inst_inventory_ctr && fn_il2cpp_string_new)
        fn_GiveItem(inst_inventory_ctr, mkstr(name), count, nullptr, true, nullptr);
}
static void doIncCompanionExp(int v) {
    CHECK_READY();
    refreshInstances();
    if (!fn_IncCompanionExp || !inst_CompanionController) return;
    void** list_ptr = (void**)((uintptr_t)inst_CompanionController + 0x40);
    if (!*list_ptr) return;
    void** arr = (void**)((uintptr_t)*list_ptr + 0x10);
    if (!*arr) return;
    void* first = *(void**)((uintptr_t)*arr);
    if (!first) return;
    fn_IncCompanionExp(inst_CompanionController, v, first, nullptr);
}

// ===== Backup/Restore =====
static std::string getExternalFilesDir() {
    if (!gJvm || !gContext) return "";
    JNIEnv* env = nullptr; bool att = false;
    if (gJvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        gJvm->AttachCurrentThread(&env, nullptr); att = true;
    }
    if (!env) return "";
    jclass cls = env->GetObjectClass(gContext);
    jmethodID m = env->GetMethodID(cls, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    jobject f = env->CallObjectMethod(gContext, m, nullptr);
    if (!f) { m = env->GetMethodID(cls, "getFilesDir", "()Ljava/io/File;"); f = env->CallObjectMethod(gContext, m); }
    jclass fc = env->FindClass("java/io/File");
    jstring ps = (jstring)env->CallObjectMethod(f, env->GetMethodID(fc, "getAbsolutePath", "()Ljava/lang/String;"));
    const char* p = env->GetStringUTFChars(ps, nullptr);
    std::string result(p); env->ReleaseStringUTFChars(ps, p);
    if (att) gJvm->DetachCurrentThread();
    return result;
}
static bool fileExists(const char* p) { struct stat b; return stat(p, &b) == 0; }
static void copyFile(const char* s, const char* d) {
    std::ifstream in(s, std::ios::binary);
    std::ofstream out(d, std::ios::binary);
    out << in.rdbuf();
}
static void backupInventory() {
    std::string base = getExternalFilesDir(); if (base.empty()) return;
    std::string bdir = base + "/backup/"; mkdir(bdir.c_str(), 0777);
    const char* slots[] = {"Slot_0","Slot_1","Slot_2"};
    for (auto s : slots) {
        std::string src = base+"/"+s+"/the_inventory", dst = bdir+s+"_the_inventory";
        if (fileExists(src.c_str())) copyFile(src.c_str(), dst.c_str());
    }
    showToast("Backup selesai!");
}
static void loadBackup() {
    std::string base = getExternalFilesDir(); if (base.empty()) return;
    std::string bdir = base + "/backup/";
    const char* slots[] = {"Slot_0","Slot_1","Slot_2"};
    for (auto s : slots) {
        std::string src = bdir+s+"_the_inventory", dst = base+"/"+s+"/the_inventory";
        if (fileExists(src.c_str())) copyFile(src.c_str(), dst.c_str());
    }
    showToast("Backup di-load!");
}

// ===== JNI Exports =====
extern "C" {

JNIEXPORT jstring JNICALL
Java_com_android_support_Loader_setTitleText(JNIEnv* env, jobject obj) {
    return env->NewStringUTF("Hybrid Animals");
}
JNIEXPORT jstring JNICALL
Java_com_android_support_Loader_setHeadingText(JNIEnv* env, jobject obj) {
    return env->NewStringUTF("Mod Menu");
}

JNIEXPORT jobjectArray JNICALL
Java_com_android_support_Loader_GetFeatureList(JNIEnv* env, jobject obj) {
    const char* features[] = {
        "Category_⚡ Player Stats",
        "SeekBar_Set EXP_0_999999",        // 1
        "Button_Apply EXP",                // 2
        "SeekBar_Set Level_1_100",         // 3
        "Button_Apply Level",              // 4
        "SeekBar_Set Skill Points_0_500",  // 5
        "Button_Apply Skill Points",       // 6
        "Category_🎁 Give Item",
        "SeekBar_Coins_1_9999",            // 8
        "Button_Give Coins",               // 9
        "SeekBar_Gems_1_999",              // 10
        "Button_Give Gems",                // 11
        "SeekBar_Wood_1_999",              // 12
        "Button_Give Wood",                // 13
        "SeekBar_Stone_1_999",             // 14
        "Button_Give Stone",               // 15
        "Category_🎲 Loot & Random",
        "Button_Generate Chest Loot",      // 17
        "Button_Roll Random Creature",     // 18
        "Button_Add Many Loots",           // 19
        "Category_🐾 Companion",
        "SeekBar_Add Pet EXP_1_99999",     // 21
        "Button_Give EXP to Pet",          // 22
        "SeekBar_Set Pet Level_1_100",     // 23
        "Button_Apply Pet Level",          // 24
        "Category_⚔️ God Mode",
        "ButtonOnOff_Toggle Invincible",   // 26
        "ButtonOnOff_Toggle Insta Kill",   // 27
        "ButtonOnOff_Mobs Ignore Player",  // 28
        "Category_💾 Backup & Restore",
        "Button_Backup Inventory",         // 30
        "Button_Load Backup",              // 31
        "WhatsApp_6281241462583_Chat Owner",
        "Hide_Hide Icon",
        "Close_Close Menu",
    };
    int total = sizeof(features)/sizeof(features[0]);
    jobjectArray ret = env->NewObjectArray(total, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for (int i = 0; i < total; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
    return ret;
}

JNIEXPORT void JNICALL
Java_com_android_support_Loader_Changes(JNIEnv* env, jobject obj, jint feature, jint value) {
    switch (feature) {
        case 1:  val_exp      = value; break;
        case 2:  doSetExp(val_exp);           break;
        case 3:  val_level    = value; break;
        case 4:  doSetLevel(val_level);       break;
        case 5:  val_skillpts = value; break;
        case 6:  doSetSkillPts(val_skillpts); break;
        case 8:  val_coin  = value; break;
        case 9:  doGiveItem("coin",  val_coin);  break;
        case 10: val_gem   = value; break;
        case 11: doGiveItem("gem",   val_gem);   break;
        case 12: val_wood  = value; break;
        case 13: doGiveItem("wood",  val_wood);  break;
        case 14: val_stone = value; break;
        case 15: doGiveItem("stone", val_stone); break;
        case 17: doGenLootChest();    break;
        case 18: doTryRollCreature(); break;
        case 19: doAddManyLoots();    break;
        case 21: val_comp_exp = value; break;
        case 22: doIncCompanionExp(val_comp_exp); break;
        case 23: val_pet_lvl  = value; break;
        case 24: doSetPetLevel(val_pet_lvl); break;
        case 26: doCycleInvincible(); break;
        case 27: doCycleInstaKill();  break;
        case 28: doCycleEnemySee();   break;
        case 30: backupInventory(); break;
        case 31: loadBackup();      break;
        default: break;
    }
}

} // extern "C"

// ===== Hack Thread =====
void* hack_thread(void*) {
    // Tunggu gContext di-set dari Java side dulu
    int wait = 0;
    while (!gContext && wait < 30) { sleep(1); wait++; }

    showToast("Menginisialisasi libil2cpp...");

    do { sleep(1); } while (!isLibraryLoaded(libName));

    showToast("libil2cpp terdeteksi, mohon tunggu...");
    sleep(2);

    void* handle = dlopen(libName, RTLD_NOLOAD);
    if (!handle) {
        showToast("Gagal buka libil2cpp!");
        return nullptr;
    }

    fn_il2cpp_string_new             = (il2cpp_string_new_t)  dlsym(handle, "il2cpp_string_new");
    il2cpp_domain_get                = (dom_get_t)             dlsym(handle, "il2cpp_domain_get");
    il2cpp_domain_assembly_open      = (dom_asm_open_t)        dlsym(handle, "il2cpp_domain_assembly_open");
    il2cpp_assembly_get_image        = (asm_get_image_t)       dlsym(handle, "il2cpp_assembly_get_image");
    il2cpp_class_from_name           = (class_from_name_t)     dlsym(handle, "il2cpp_class_from_name");
    il2cpp_class_get_field_from_name = (get_field_t)           dlsym(handle, "il2cpp_class_get_field_from_name");
    il2cpp_field_static_get_value    = (field_static_get_t)    dlsym(handle, "il2cpp_field_static_get_value");

    if (il2cpp_domain_get && il2cpp_domain_assembly_open && il2cpp_assembly_get_image) {
        void* domain = il2cpp_domain_get();
        void* asm_   = il2cpp_domain_assembly_open(domain, "Assembly-CSharp");
        if (asm_) gImage = il2cpp_assembly_get_image(asm_);
    }

    fn_OverwriteExp      = (OverwriteCurrentExp_t) getAbsoluteAddress(libName, 0xCF4250);
    fn_OverwriteLevel    = (OverwritePlayerLevel_t)getAbsoluteAddress(libName, 0xCE8D2C);
    fn_OverwriteSkillPts = (OverwriteSkillPoints_t)getAbsoluteAddress(libName, 0xCF4198);
    fn_GiveItem          = (GiveItemStr_t)          getAbsoluteAddress(libName, 0xB56710);
    fn_GenLootChest      = (GenerateLootChest_t)    getAbsoluteAddress(libName, 0xB9367C);
    fn_AddManyLoots      = (AddManyLoots_t)         getAbsoluteAddress(libName, 0xB929CC);
    fn_TryRollCreature   = (TryRollCreature_t)      getAbsoluteAddress(libName, 0xC1B3B0);
    fn_IncCompanionExp   = (IncreaseCompanionExp_t) getAbsoluteAddress(libName, 0xC797E4);
    fn_SetLevel          = (SetLevel_t)             getAbsoluteAddress(libName, 0xCA6758);
    fn_CycleInvincible   = (CycleInvincibility_t)  getAbsoluteAddress(libName, 0xB08BA4);
    fn_CycleInstaKill    = (CycleInstaKill_t)       getAbsoluteAddress(libName, 0xB08C0C);
    fn_CycleEnemySee     = (CycleEnemiesCanSeeYou_t)getAbsoluteAddress(libName, 0xB085F0);

    gLibReady = true;
    showToast("Mod siap digunakan!");

    return nullptr;
}

__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);
}
