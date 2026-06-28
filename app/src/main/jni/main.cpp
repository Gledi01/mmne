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

static JavaVM* gJvm = nullptr;
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

// ===== IL2CPP String =====
typedef void* (*il2cpp_string_new_t)(const char*);
static il2cpp_string_new_t fn_il2cpp_string_new = nullptr;
static void* mkstr(const char* s) {
    return fn_il2cpp_string_new ? fn_il2cpp_string_new(s) : nullptr;
}

// ===== Typedefs =====
// GameController
typedef void (*OverwriteCurrentExp_t)(void*, int, void*, void*);
typedef void (*OverwritePlayerLevel_t)(void*, int, void*, void*);
typedef void (*OverwriteSkillPoints_t)(void*, int, void*, void*);
// inventory_ctr
typedef void (*GiveItemStr_t)(void*, void*, int, void*, bool, void*);
// LootControl
typedef void* (*GenerateLootChest_t)(void*, void*, void*);
typedef void  (*AddManyLoots_t)(void*, int, int, void*, int, int, void*, void*, void*);
// RewardsControl
typedef int  (*TryRollCreature_t)(void*, void*);
// CompanionController
typedef void (*IncreaseCompanionExp_t)(void*, int, void*, void*);
// CreativeModeControl (god mode)
typedef void (*CycleInvincibility_t)(void*, void*);
typedef void (*CycleInstaKill_t)(void*, void*);
typedef void (*CycleEnemiesCanSeeYou_t)(void*, void*);
// ConsoleControl
typedef void (*SetLevel_t)(void*, int, void*);

// ===== Instance pointers =====
static void** GameController_Inst      = nullptr;
static void** inventory_ctr_Inst       = nullptr;
static void** LootControl_Inst         = nullptr;
static void** RewardsControl_Inst      = nullptr;
static void** CompanionController_Inst = nullptr;
static void** CreativeMode_Inst        = nullptr;
static void** ConsoleControl_Inst      = nullptr;

// ===== Function pointers =====
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

// ===== Saved slider values =====
static int val_exp          = 0;
static int val_level        = 1;
static int val_skillpts     = 0;
static int val_coin         = 0;
static int val_gem          = 0;
static int val_wood         = 0;
static int val_stone        = 0;
static int val_comp_exp     = 0;

// ===== Instance getters =====
#define INST(ptr) ((ptr) ? *(ptr) : nullptr)

// ===== Actions =====
static void doSetExp(int v)       { void* i=INST(GameController_Inst); if(fn_OverwriteExp&&i)      fn_OverwriteExp(i,v,nullptr,nullptr); }
static void doSetLevel(int v)     { void* i=INST(GameController_Inst); if(fn_OverwriteLevel&&i)    fn_OverwriteLevel(i,v,nullptr,nullptr); }
static void doSetSkillPts(int v)  { void* i=INST(GameController_Inst); if(fn_OverwriteSkillPts&&i) fn_OverwriteSkillPts(i,v,nullptr,nullptr); }

static void doGiveItem(const char* name, int count) {
    void* i=INST(inventory_ctr_Inst);
    if(fn_GiveItem&&i&&fn_il2cpp_string_new) fn_GiveItem(i,mkstr(name),count,nullptr,true,nullptr);
}

static void doGenLootChest()    { void* i=INST(LootControl_Inst);         if(fn_GenLootChest&&i)    fn_GenLootChest(i,nullptr,nullptr); }
static void doAddManyLoots()    { void* i=INST(LootControl_Inst);         if(fn_AddManyLoots&&i)    fn_AddManyLoots(i,5,10,nullptr,3,6,nullptr,nullptr,nullptr); }
static void doTryRollCreature() { void* i=INST(RewardsControl_Inst);      if(fn_TryRollCreature&&i) fn_TryRollCreature(i,nullptr); }

static void doIncCompanionExp(int v) {
    void* ctrl=INST(CompanionController_Inst);
    if(!fn_IncCompanionExp||!ctrl) return;
    // Ambil companion pertama dari active_companions list (offset 0x40)
    // active_companions = List<ActiveCompanion>, items di offset 0x10 dari List object
    void** list_ptr = (void**)((uintptr_t)ctrl + 0x40);
    if(!*list_ptr) return;
    void** items_ptr = (void**)((uintptr_t)*list_ptr + 0x10);
    if(!*items_ptr) return;
    void* first_companion = *(void**)((uintptr_t)*items_ptr + 0x0);
    if(!first_companion) return;
    fn_IncCompanionExp(ctrl, v, first_companion, nullptr);
}

static void doSetCompanionLevel(int v) {
    void* ctrl=INST(ConsoleControl_Inst);
    if(fn_SetLevel&&ctrl) fn_SetLevel(ctrl,v,nullptr);
}

static void doCycleInvincible()  { void* i=INST(CreativeMode_Inst); if(fn_CycleInvincible&&i) fn_CycleInvincible(i,nullptr); }
static void doCycleInstaKill()   { void* i=INST(CreativeMode_Inst); if(fn_CycleInstaKill&&i)  fn_CycleInstaKill(i,nullptr); }
static void doCycleEnemySee()    { void* i=INST(CreativeMode_Inst); if(fn_CycleEnemySee&&i)   fn_CycleEnemySee(i,nullptr); }

// ===== Backup/Restore =====
static std::string getExternalFilesDir() {
    if(!gJvm||!gContext) return "";
    JNIEnv* env=nullptr; bool att=false;
    if(gJvm->GetEnv((void**)&env,JNI_VERSION_1_6)==JNI_EDETACHED){gJvm->AttachCurrentThread(&env,nullptr);att=true;}
    if(!env) return "";
    jclass cls=env->GetObjectClass(gContext);
    jmethodID m=env->GetMethodID(cls,"getExternalFilesDir","(Ljava/lang/String;)Ljava/io/File;");
    jobject f=env->CallObjectMethod(gContext,m,nullptr);
    if(!f){m=env->GetMethodID(cls,"getFilesDir","()Ljava/io/File;");f=env->CallObjectMethod(gContext,m);}
    jclass fc=env->FindClass("java/io/File");
    jstring ps=(jstring)env->CallObjectMethod(f,env->GetMethodID(fc,"getAbsolutePath","()Ljava/lang/String;"));
    const char* p=env->GetStringUTFChars(ps,nullptr);
    std::string result(p); env->ReleaseStringUTFChars(ps,p);
    if(att) gJvm->DetachCurrentThread();
    return result;
}
static bool fileExists(const char* p){struct stat b;return stat(p,&b)==0;}
static void copyFile(const char* s,const char* d){std::ifstream in(s,std::ios::binary);std::ofstream out(d,std::ios::binary);out<<in.rdbuf();}
static void backupInventory(){
    std::string base=getExternalFilesDir(); if(base.empty()) return;
    std::string bdir=base+"/backup/"; mkdir(bdir.c_str(),0777);
    const char* slots[]={"Slot_0","Slot_1","Slot_2"};
    for(auto s:slots){std::string src=base+"/"+s+"/the_inventory",dst=bdir+s+"_the_inventory";if(fileExists(src.c_str()))copyFile(src.c_str(),dst.c_str());}
}
static void loadBackup(){
    std::string base=getExternalFilesDir(); if(base.empty()) return;
    std::string bdir=base+"/backup/";
    const char* slots[]={"Slot_0","Slot_1","Slot_2"};
    for(auto s:slots){std::string src=bdir+s+"_the_inventory",dst=base+"/"+s+"/the_inventory";if(fileExists(src.c_str()))copyFile(src.c_str(),dst.c_str());}
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
        // ── Player Stats ──          idx
        "Category_⚡ Player Stats",   // 0
        "SeekBar_Set EXP_0_999999",   // 1
        "SeekBar_Set Level_1_100",    // 2
        "SeekBar_Set Skill Points_0_500", // 3

        // ── Give Item ──
        "Category_🎁 Give Item",      // 4
        "SeekBar_Coins_1_9999",       // 5
        "Button_Give Coins",          // 6
        "SeekBar_Gems_1_999",         // 7
        "Button_Give Gems",           // 8
        "SeekBar_Wood_1_999",         // 9
        "Button_Give Wood",           // 10
        "SeekBar_Stone_1_999",        // 11
        "Button_Give Stone",          // 12

        // ── Loot & Random ──
        "Category_🎲 Loot & Random",  // 13
        "Button_Generate Chest Loot", // 14
        "Button_Roll Random Creature",// 15
        "Button_Add Many Loots",      // 16

        // ── Companion / Pet ──
        "Category_🐾 Companion",      // 17
        "SeekBar_Add Pet EXP_1_99999",// 18
        "Button_Give EXP to Pet",     // 19
        "SeekBar_Set Pet Level_1_100",// 20
        "Button_Apply Pet Level",     // 21

        // ── God Mode ──
        "Category_⚔️ God Mode",       // 22
        "ButtonOnOff_Toggle Invincible", // 23
        "ButtonOnOff_Toggle Insta Kill", // 24
        "ButtonOnOff_Mobs Ignore Player",// 25

        // ── Backup & Restore ──
        "Category_💾 Backup & Restore", // 26
        "Button_Backup Inventory",    // 27
        "Button_Load Backup",         // 28

        // ── Misc ──
        "WhatsApp_6281241462583_Chat Owner",
        "Hide_Hide Icon",
        "Close_Close Menu",
    };
    int total = sizeof(features)/sizeof(features[0]);
    jobjectArray ret = env->NewObjectArray(total, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for(int i=0;i<total;i++) env->SetObjectArrayElement(ret,i,env->NewStringUTF(features[i]));
    return ret;
}

static int val_pet_level = 1;

JNIEXPORT void JNICALL
Java_com_android_support_Loader_Changes(JNIEnv* env, jobject obj, jint feature, jint value) {
    switch(feature) {
        // Player Stats
        case 1:  val_exp=value;      doSetExp(value);      break;
        case 2:  val_level=value;    doSetLevel(value);    break;
        case 3:  val_skillpts=value; doSetSkillPts(value); break;

        // Give Item
        case 5:  val_coin=value;   break;
        case 6:  doGiveItem("coin",val_coin);   break;
        case 7:  val_gem=value;    break;
        case 8:  doGiveItem("gem",val_gem);     break;
        case 9:  val_wood=value;   break;
        case 10: doGiveItem("wood",val_wood);   break;
        case 11: val_stone=value;  break;
        case 12: doGiveItem("stone",val_stone); break;

        // Loot & Random
        case 14: doGenLootChest();    break;
        case 15: doTryRollCreature(); break;
        case 16: doAddManyLoots();    break;

        // Companion
        case 18: val_comp_exp=value; break;
        case 19: doIncCompanionExp(val_comp_exp); break;
        case 20: val_pet_level=value; break;
        case 21: doSetCompanionLevel(val_pet_level); break;

        // God Mode (toggle — value 0=off, setiap tekan cycle)
        case 23: doCycleInvincible(); break;
        case 24: doCycleInstaKill();  break;
        case 25: doCycleEnemySee();   break;

        // Backup & Restore
        case 27: backupInventory(); break;
        case 28: loadBackup();      break;

        default: break;
    }
}

} // extern "C"

// ===== Hack Thread =====
void* hack_thread(void*) {
    do { sleep(1); } while(!isLibraryLoaded(libName));

    uintptr_t base = getLibraryBase(libName);
    void* handle = dlopen(libName, RTLD_NOLOAD);
    if(!handle) return nullptr;

    fn_il2cpp_string_new = (il2cpp_string_new_t)dlsym(handle, "il2cpp_string_new");

    // Assign all function pointers
    fn_OverwriteExp      = (OverwriteCurrentExp_t) (base + 0xCF4250);
    fn_OverwriteLevel    = (OverwritePlayerLevel_t)(base + 0xCE8D2C);
    fn_OverwriteSkillPts = (OverwriteSkillPoints_t)(base + 0xCF4198);
    fn_GiveItem          = (GiveItemStr_t)          (base + 0xB56710);
    fn_GenLootChest      = (GenerateLootChest_t)    (base + 0xB9367C);
    fn_AddManyLoots      = (AddManyLoots_t)         (base + 0xB929CC);
    fn_TryRollCreature   = (TryRollCreature_t)      (base + 0xC1B3B0);
    fn_IncCompanionExp   = (IncreaseCompanionExp_t) (base + 0xC797E4);
    fn_SetLevel          = (SetLevel_t)             (base + 0xCA6758);
    fn_CycleInvincible   = (CycleInvincibility_t)  (base + 0xB08BA4);
    fn_CycleInstaKill    = (CycleInstaKill_t)       (base + 0xB08C0C);
    fn_CycleEnemySee     = (CycleEnemiesCanSeeYou_t)(base + 0xB085F0);

    // Resolve static instances via IL2CPP API
    typedef void* (*dom_get_t)();
    typedef void* (*dom_asm_open_t)(void*, const char*);
    typedef void* (*asm_get_image_t)(void*);
    typedef void* (*class_from_name_t)(void*, const char*, const char*);
    typedef void* (*get_field_t)(void*, const char*);
    typedef void  (*field_static_get_t)(void*, void*);

    auto dom_get       = (dom_get_t)      dlsym(handle,"il2cpp_domain_get");
    auto dom_asm_open  = (dom_asm_open_t) dlsym(handle,"il2cpp_domain_assembly_open");
    auto asm_get_image = (asm_get_image_t)dlsym(handle,"il2cpp_assembly_get_image");
    auto class_from    = (class_from_name_t)dlsym(handle,"il2cpp_class_from_name");
    auto get_field     = (get_field_t)    dlsym(handle,"il2cpp_class_get_field_from_name");
    auto field_get     = (field_static_get_t)dlsym(handle,"il2cpp_field_static_get_value");

    if(!dom_get||!dom_asm_open||!asm_get_image||!class_from||!get_field||!field_get) return nullptr;

    void* domain = dom_get();
    void* asm_   = dom_asm_open(domain,"Assembly-CSharp");
    void* image  = asm_get_image(asm_);

    auto resolveInst = [&](const char* className, void**& out_ptr) {
        static void* cache[16] = {};
        static int n = 0;
        void* cls = class_from(image,"",className);
        if(!cls) return;
        void* f = get_field(cls,"Instance");
        if(!f) return;
        void** slot = &cache[n++];
        field_get(f, slot);
        out_ptr = slot;
    };

    resolveInst("GameController",      GameController_Inst);
    resolveInst("inventory_ctr",       inventory_ctr_Inst);
    resolveInst("LootControl",         LootControl_Inst);
    resolveInst("RewardsControl",      RewardsControl_Inst);
    resolveInst("CompanionController", CompanionController_Inst);
    resolveInst("ConsoleControl",      ConsoleControl_Inst);

    // CreativeModeControl — cari nama class yang punya CycleInvinsibility
    // Dari dump: class di sekitar line 54920 punya god_mode_page_button, invinsible_button
    // Nama classnya perlu dicek — coba "CreativeModeControl" dulu
    resolveInst("CreativeModeControl", CreativeMode_Inst);

    return nullptr;
}

__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);
}
