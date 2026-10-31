/* Verify replay boundaries, including a P2 input changing mid-frame. Native
 * gameplay replay is a separate original-disc check. No window is created. */
extern "C" {
#include "mmx5_coop_internal.h"
}
#include "host_osd.h"
#include "memcard.h"
#include "mod_netplay.h"
#include "mod_snapshot.h"
#include "mod_runtime.h"
#include "savestate.h"
#include "sio.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <cstdlib>
#include <memory>
static std::array<uint8_t,0x200000> memory;
static std::array<uint8_t,0x158> partner;
static std::array<uint16_t,2> pads{0xFFFF,0xFFFF};
static std::map<std::string,unsigned> counters;
static std::string save_root,shown[2];
static std::string application_root;
static bool online;
static bool diagnostics_active=true;
static bool coop_active=true,split_active=true,partner_initialized=true;
static bool projected_context;
static std::array<uint8_t,0x158> canonical_first;
static std::array<PSXModControllerSource,2> sources{};
uint8_t psx_mod_read_byte(uint32_t a) {return memory[a&0x1FFFFF];}
void psx_mod_write_byte(uint32_t a,uint8_t v) {memory[a&0x1FFFFF]=v;}
uint16_t psx_mod_read_half(uint32_t a) {return psx_mod_read_byte(a)|(psx_mod_read_byte(a+1)<<8);}
void psx_mod_write_half(uint32_t a,uint16_t v) {psx_mod_write_byte(a,(uint8_t)v);psx_mod_write_byte(a+1,(uint8_t)(v>>8));}
uint32_t psx_mod_read_word(uint32_t a) {return psx_mod_read_half(a)|(uint32_t(psx_mod_read_half(a+2))<<16);}
void psx_mod_write_word(uint32_t a,uint32_t v) {psx_mod_write_half(a,(uint16_t)v);psx_mod_write_half(a+2,(uint16_t)(v>>16));}
int psx_mod_register_activation_plugin(const char*,PSXModActivationCallback) {return 1;}
int psx_mod_register_function_filter_plugin(const char*,uint32_t,PSXModFunctionFilterCallback) {return 1;}
int psx_mod_register_snapshot_loaded_observer(const char*,PSXModSnapshotLoadedObserver) {return 1;}
int psx_mod_register_snapshot_provider(const char*,const PSXModSnapshotProvider*) {return 1;}
int psx_mod_local_view_scope(void) {return 0;}
int psx_mod_netplay_is_active(void) {return online;}
int mod_runtime_snapshot_package_active(const char *id) {
    return (!strcmp(id,"mmx5.diagnostics") && diagnostics_active) ||
           (!strcmp(id,"mmx5.coop") && coop_active);
}
int psx_mod_option_value(const char *package,const char *feature,const char *key,char *value,uint32_t size) {
    if(strcmp(package,"mmx5.coop") || strcmp(feature,"coop"))return 0;
    const char *text=!strcmp(key,"keep_zero")?"false":!strcmp(key,"local_cameras")?"split":
                     !strcmp(key,"cameras")?"unified":!strcmp(key,"hud_layout")?"stacked":
                     !strcmp(key,"door_players")?"both":!strcmp(key,"separate_etanks")?"true":
                     !strcmp(key,"ultimate_armor_x")?(online?"true":"false"):
                     !strcmp(key,"black_zero")?(online?"false":"true"):
                     !strcmp(key,"boss_scaling")?(online?"false":"true"):
                     !strcmp(key,"boss_cooldowns")?(online?"per_player":"shared"):nullptr;
    if(!text)return 0;
    return snprintf(value,size,"%s",text)>0;
}
int psx_mod_set_controller_source(uint32_t seat,PSXModControllerSource source) {sources.at(seat)=source;return 1;}
void psx_mod_counter_add(const char* name,uint32_t n) {counters[name]+=n;}
uint16_t sio_get_pad_buttons_slot(int slot) {return pads.at(slot);}
int memcard_is_present(int slot) {return slot==0;}
int memcard_export_raw(int,uint8_t *out) {memset(out,0xAB,MEMCARD_SIZE);return 0;}
const char *savestate_root_dir(void) {return save_root.c_str();}
void host_osd_set_diagnostics(const char *a,const char *b) {shown[0]=a?a:"";shown[1]=b?b:"";}
namespace PSXRecompV4 {
std::string mod_runtime_plan_fingerprint_portable() {return {};}
const std::string& mod_runtime_fingerprint() {static const std::string fp="fixture-plan";return fp;}
const std::filesystem::path& mod_runtime_root() {static const auto root=std::filesystem::path(save_root)/"mods";return root;}
}
int mmx5_coop_ready(void) {return partner_initialized;}
int mmx5_coop_split_views(void) {return split_active;}
int mmx5_coop_projected(void) {return projected_context;}
const uint8_t *mmx5_coop_first_body(void) {return canonical_first.data();}
void mmx5_coop_development_mask_pad(void) {}
const uint8_t *mmx5_coop_second_body(void) {return partner.data();}
int mmx5_coop_finish(CPUState *cpu,uint32_t v) {cpu->gpr[2]=v;return 1;}
uint32_t mmx5_coop_call(CPUState*,uint32_t,uint32_t,uint32_t) {
    uint16_t held=(uint16_t)~pads[0];held=(uint16_t)((held<<8)|(held>>8));
    uint16_t old=psx_mod_read_half(0x800C931Cu);
    psx_mod_write_half(0x800C931Cu,held);psx_mod_write_half(0x800C931Eu,old);
    psx_mod_write_half(0x800C9320u,(uint16_t)(held&~old));return 77;
}
uint16_t mmx5_coop_input(unsigned seat);
namespace {std::filesystem::path application_directory() {return std::filesystem::u8path(application_root);}}
#define MMX5_DIAGNOSTIC_TEST
#define MMX5_DIAGNOSTIC_RELEASE_VERSION "fixture-release-42"
#include "../src/mods/mmx5_diagnostics.cpp"
uint16_t mmx5_coop_input(unsigned seat) {
    uint16_t buttons=pads.at(seat);mmx5_diagnostics_resolve_input(seat,&buttons);
    uint16_t native=(uint16_t)~buttons;return (uint16_t)((native<<8)|(native>>8));
}
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}} while(0)
static void env(const char *v) {
#ifdef _WIN32
    _putenv_s("MMX5_DIAGNOSTIC_REPLAY",v);
#else
    setenv("MMX5_DIAGNOSTIC_REPLAY",v,1);
#endif
}
static void baseline_env(const char *v) {
#ifdef _WIN32
    _putenv_s("MMX5_DIAGNOSTIC_BASELINE",v);
#else
    setenv("MMX5_DIAGNOSTIC_BASELINE",v,1);
#endif
}
int main(int argc,char **argv) {
    CHECK(mmx5_diagnostic_application_directory()==
          std::filesystem::canonical(std::filesystem::u8path(argv[0])).parent_path());
#ifndef _WIN32
    setenv("RECOMP_APPIMAGE_PATH","/tmp/mmx5-path-test/MegaManX5Recomp.AppImage",1);
    CHECK(mmx5_diagnostic_application_directory()==std::filesystem::path("/tmp/mmx5-path-test"));
    unsetenv("RECOMP_APPIMAGE_PATH");
    setenv("APPIMAGE","/tmp/mmx5-fallback/Game.AppImage",1);
    CHECK(mmx5_diagnostic_application_directory()==std::filesystem::path("/tmp/mmx5-fallback"));
    unsetenv("APPIMAGE");
#endif
    save_root=argc>1?argv[1]:"diagnostic-test-output";
    save_root+="/"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    application_root=save_root;
    // Effective labels follow the live mode, independently of saved choices.
    coop_active=false;CHECK(!strcmp(effective_mode().camera_name(),"single_player"));
    coop_active=true;split_active=false;CHECK(!strcmp(effective_mode().camera_name(),"local_unified"));
    split_active=true;CHECK(!strcmp(effective_mode().camera_name(),"local_split_8_3"));
    online=true;CHECK(!strcmp(effective_mode().camera_name(),"netplay_separate"));
    split_active=false;CHECK(!strcmp(effective_mode().camera_name(),"netplay_unified"));
    online=false;split_active=true;partner_initialized=false;CHECK(!effective_mode().ready);
    partner_initialized=true;
    std::filesystem::create_directories(save_root);CPUState cpu{};env("");activate();
    CHECK(!sources[0] && !sources[1]);
    std::filesystem::create_directories(PSXRecompV4::mod_runtime_root());
    {std::ofstream state(PSXRecompV4::mod_runtime_root()/"state.toml");state<<"format_version = 2\n";}
    auto initial=std::make_unique<decltype(memory)>(memory);
    pads={0xFFEF,0xBFFF};CHECK(inputs(&cpu,0x8001255Cu));
    CHECK(shown[0].find("U")!=std::string::npos && shown[1].find("X")!=std::string::npos);
    uint16_t raw=0xFFEF;mmx5_diagnostics_resolve_input(0,&raw);
    raw=0xFF7F;mmx5_diagnostics_resolve_input(1,&raw);
    psx_mod_write_word(FRAME,1);pads={0xFFFF,0xFFF7};CHECK(inputs(&cpu,0x8001255Cu));
    raw=0xFFF7;mmx5_diagnostics_resolve_input(1,&raw);
    recording.flush();auto final=std::make_unique<decltype(memory)>(memory);recording.close();
    std::filesystem::path file;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics"))
        if(std::filesystem::exists(entry.path()/"inputs.csv"))file=entry.path()/"inputs.csv";
    CHECK(!file.empty() && std::filesystem::file_size(file)>100);
    CHECK(std::filesystem::file_size(file.parent_path()/"card1.mcd")==MEMCARD_SIZE);
    CHECK(std::filesystem::exists(file.parent_path()/"card2.absent"));
    CHECK(std::filesystem::exists(file.parent_path()/"mods-state.toml"));
    CHECK(std::filesystem::exists(file.parent_path()/"session.json"));
    {
        std::ifstream metadata(file.parent_path()/"session.json");
        std::string text((std::istreambuf_iterator<char>(metadata)),{});
        CHECK(text.find("\"game_version\": \"fixture-release-42\"")!=std::string::npos);
        CHECK(text.find("\"camera_mode\": \"local_split_8_3\"")!=std::string::npos);
        CHECK(text.find("\"cameras\": \"unified\"")!=std::string::npos);
        CHECK(text.find("\"keep_zero\": false")!=std::string::npos);
        CHECK(text.find("\"door_players\": \"both\"")!=std::string::npos);
        CHECK(text.find("\"separate_etanks\": true")!=std::string::npos);
        CHECK(text.find("\"ultimate_armor_x\": false")!=std::string::npos);
        CHECK(text.find("\"black_zero\": true")!=std::string::npos);
        CHECK(text.find("\"boss_scaling\": true")!=std::string::npos);
        CHECK(text.find("\"boss_cooldowns\": \"shared\"")!=std::string::npos);
        std::ifstream changes(file.parent_path()/"mode-changes.csv");
        std::string history((std::istreambuf_iterator<char>(changes)),{});
        CHECK(history.find("0,0,local,local_split_8_3,2,1,x\n")!=std::string::npos);
    }
    {std::ifstream location(std::filesystem::path(save_root)/"diagnostics-location.txt");
     std::string text((std::istreambuf_iterator<char>(location)),{});
     CHECK(text.find(std::filesystem::absolute(file.parent_path()).make_preferred().u8string())!=std::string::npos);}
    memory=*initial;env(file.string().c_str());activate();pads={0xFFFF,0xFFFF};
    CHECK(sources[0] && sources[1]);
    CHECK(inputs(&cpu,0x8001255Cu) && replaying && !replay_failed);
    raw=0xFFFF;CHECK(mmx5_diagnostics_resolve_input(0,&raw) && raw==0xFFEF);
    raw=0xFFFF;CHECK(mmx5_diagnostics_resolve_input(1,&raw) && raw==0xFF7F);
    psx_mod_write_word(FRAME,1);CHECK(inputs(&cpu,0x8001255Cu));
    raw=0xFFFF;CHECK(mmx5_diagnostics_resolve_input(1,&raw) && raw==0xFFF7);
    CHECK(memory==*final && counters["mmx5.diagnostics.replayed-inputs"]==2);
    psx_mod_write_word(FRAME,2);CHECK(inputs(&cpu,0x8001255Cu) && !replaying && !replay_failed);
    CHECK(counters["mmx5.diagnostics.replay-complete"]==1);
    // Divergence must be surfaced rather than silently accepting the trace.
    memory=*initial;activate();psx_mod_write_byte(MMX5_PLAY+0x79,1);
    inputs(&cpu,0x8001255Cu);CHECK(replay_failed && !replaying);
    memory=*initial;activate();inputs(&cpu,0x8001255Cu);
    raw=0xFFFF;mmx5_diagnostics_resolve_input(1,&raw);CHECK(replay_failed);
    memory=*initial;activate();online=true;inputs(&cpu,0x8001255Cu);CHECK(replay_failed && !replaying);
    env("");activate();CHECK(!sources[0] && !sources[1]);
    // First-time installs have no mod state file. Their defaults still need
    // a portable starting-mod capture for the replay launcher.
    std::filesystem::remove(PSXRecompV4::mod_runtime_root()/"state.toml");
    begin();recording.close();
    unsigned captures=0;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        if(std::filesystem::exists(entry.path()/"inputs.csv")) {
            CHECK(std::filesystem::exists(entry.path()/"mods-state.toml"));
            ++captures;
        }
    }
    CHECK(captures==2);
    // A P2 callback temporarily occupies PLAYER/PLAY; labels must still name
    // the actual P1 rather than claiming the campaign switched characters.
    projected_context=true;canonical_first[2]=0;canonical_first[0x5C]=17;
    canonical_first[10]=123;canonical_first[14]=77;
    psx_mod_write_byte(MMX5_PLAY+0x44,1);psx_mod_write_byte(MMX5_PLAYER+0x5C,3);
    psx_mod_write_half(MMX5_PLAYER+10,400);psx_mod_write_half(MMX5_PLAYER+14,200);
    auto projected_row=sample();
    CHECK(projected_row[9]==0 && projected_row[11]==17 && projected_row[13]==123 && projected_row[14]==77);
    CHECK(effective_mode().campaign==0);
    projected_context=false;
    // Netplay traces must identify their mode, rather than masquerading as
    // independently playable offline recordings.
    online=true;activate();inputs(&cpu,0x8001255Cu);recording.flush();
    bool network_metadata=false;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        std::ifstream meta(entry.path()/"session.json");
        std::string text((std::istreambuf_iterator<char>(meta)),{});
        if(text.find("\"netplay\": true")!=std::string::npos) {
            CHECK(text.find("\"ultimate_armor_x\": true")!=std::string::npos);
            CHECK(text.find("\"black_zero\": false")!=std::string::npos);
            CHECK(text.find("\"boss_scaling\": false")!=std::string::npos);
            CHECK(text.find("\"boss_cooldowns\": \"per_player\"")!=std::string::npos);
            network_metadata=true;
        }
    }
    CHECK(network_metadata);
    // Loading or rewinding starts a new replayable segment with its exact
    // restored machine image. It must not append a time jump to old inputs.
    online=false;activate();inputs(&cpu,0x8001255Cu);
    const uint8_t restored_image[]={0x50,0x53,0x54,0x0A,0xFE,0x01};
    diagnostics_active=false;activate();
    auto folder_count=std::distance(std::filesystem::directory_iterator(
        std::filesystem::path(application_root)/"saves/diagnostics"),
        std::filesystem::directory_iterator{});
    state_loaded(restored_image,sizeof restored_image);
    uint16_t disabled_buttons=0xFF7F;mmx5_diagnostics_resolve_input(1,&disabled_buttons);
    CHECK(!started && !recording.is_open() && pending_baseline.empty() && sequence==0);
    CHECK(std::distance(std::filesystem::directory_iterator(
        std::filesystem::path(application_root)/"saves/diagnostics"),
        std::filesystem::directory_iterator{})==folder_count);
    diagnostics_active=true;
    state_loaded(restored_image,sizeof restored_image);
    CHECK(sequence==0 && started && recording.is_open() && pending_baseline.empty());
    CHECK(counters["mmx5.diagnostics.state-segments"]==1);
    auto segment_initial=std::make_unique<decltype(memory)>(memory);
    uint16_t prefix_buttons=0xFF7F;mmx5_diagnostics_resolve_input(1,&prefix_buttons);
    inputs(&cpu,0x8001255Cu);
    auto segment_frame=psx_mod_read_word(FRAME);
    psx_mod_write_word(FRAME,segment_frame+1);inputs(&cpu,0x8001255Cu);recording.flush();
    bool found_baseline=false;
    std::filesystem::path segment_inputs;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        auto baseline=entry.path()/"baseline.pst";
        if(!std::filesystem::exists(baseline))continue;
        std::ifstream in(baseline,std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(in)),{});
        CHECK(bytes.size()==sizeof restored_image && !memcmp(bytes.data(),restored_image,bytes.size()));
        std::ifstream meta(entry.path()/"session.json");
        std::string text((std::istreambuf_iterator<char>(meta)),{});
        CHECK(text.find("\"baseline\": \"baseline.pst\"")!=std::string::npos);
        segment_inputs=entry.path()/"inputs.csv";
        found_baseline=true;
    }
    CHECK(found_baseline);
    // A loaded PC can read P2 before the first pad decoder invocation. Start
    // playback on restore so leading C rows are consumed at that boundary.
    recording.close();memory=*segment_initial;env(segment_inputs.string().c_str());
    baseline_env((segment_inputs.parent_path()/"baseline.pst").string().c_str());activate();
    // A deferred or failed auto-load must never consume ordinary-boot rows.
    CHECK(inputs(&cpu,0x8001255Cu) && !started && !replaying && !replay_failed);
    CHECK(!baseline_loaded && sequence==0);
    memory=*segment_initial;
    state_loaded(restored_image,sizeof restored_image);
    CHECK(started && replaying && !replay_failed && baseline_loaded);
    prefix_buttons=0xFFFF;CHECK(mmx5_diagnostics_resolve_input(1,&prefix_buttons) && prefix_buttons==0xFF7F);
    CHECK(inputs(&cpu,0x8001255Cu) && !replay_failed);
    psx_mod_write_word(FRAME,segment_frame+1);CHECK(inputs(&cpu,0x8001255Cu) && !replay_failed);
    // A different restored image is a refusal, even if its input trace could
    // accidentally match the short segment.
    memory=*segment_initial;activate();
    uint8_t wrong_image[sizeof restored_image];memcpy(wrong_image,restored_image,sizeof wrong_image);
    wrong_image[sizeof wrong_image-1]^=1;
    state_loaded(wrong_image,sizeof wrong_image);
    CHECK(replay_failed && !replaying && !baseline_loaded);
    baseline_env("");
    env("");activate();
    // Direct QA setup remains useful evidence, but must be explicitly marked
    // so an input-only replay cannot misreport its later divergence.
    inputs(&cpu,0x8001255Cu);mmx5_diagnostics_development_fixture(9,0,1,4);
    recording.close();
    bool fixture_provenance=false;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        std::ifstream trace(entry.path()/"inputs.csv");
        std::string text((std::istreambuf_iterator<char>(trace)),{});
        if(text.find("D 1 9 0 1 4\n")!=std::string::npos)fixture_provenance=true;
    }
    CHECK(fixture_provenance);
    // A directory/write failure is visible both on the input display and in
    // the location pointer. It must never show REC with an unusable recording.
    online=false;activate();
    application_root+="/blocked-app";std::filesystem::create_directories(application_root);
    {std::ofstream blocked(std::filesystem::path(application_root)/"saves");blocked<<"file, not a directory";}
    inputs(&cpu,0x8001255Cu);
    CHECK(recording_failed && !recording.is_open());
    CHECK(shown[0].find("ERROR")!=std::string::npos && shown[0].find("REC")==std::string::npos);
    {std::ifstream location(std::filesystem::path(application_root)/"diagnostics-location.txt");
     std::string text((std::istreambuf_iterator<char>(location)),{});CHECK(text.find("FAILED")!=std::string::npos);}
    printf("diagnostic recording/replay boundary checks passed\n");return 0;
}
