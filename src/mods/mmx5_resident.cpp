// SLUS-01334: resident archive bytes consumed by the original callbacks and
// GPU/SPU upload queue. Blocking requests only; XA/STR and actor streams stay LLE.
#include "mod_plugins.h"
#include "mod_resident.h"
#include "mod_resident_sector.hpp"
#include "mod_runtime.h"
#include "mmx5_resident_data.h"
#include "cpu_state.h"
#include "cdrom.h"
#include "crash_trace.h"
#include "mmx5_resident_catalog.h"

namespace {
constexpr const char* Plugin="mmx5.resident-loading";
constexpr uint32_t Status=0x8009811C, Pending=0x80091D50;
constexpr uint32_t Remain=0x800E5A18, Dest=0x800E5A1C, Lba=0x800E5A24;
constexpr uint32_t Packed=0x800E5A30, Count=0x800E5A40, Index=0x800E5BE9;
const PSXResidentPack* pack=nullptr;
PSXResidentMode2Sector sector;
bool pumping=false;
uint32_t sector_lba=0;
uint32_t word(uint32_t a) {return psx_mod_read_word(a);}
void call(CPUState* cpu,uint32_t pc,uint32_t a0=0,uint32_t a1=0) {
    psx_mod_call_guest(cpu,pc,0x8000FD00,a0,a1,0,0);
}
bool ram(uint32_t a,uint32_t bytes) {
    return !(a&3) && a>=0x80010000 && a<0x80200000 && bytes<=0x80200000-a;
}
bool callback_caller(CPUState* cpu) {
    return pumping && cpu->gpr[31]>=0x80014464 && cpu->gpr[31]<0x80015DDC;
}
int ready(CPUState* cpu,uint32_t) {
    if (!callback_caller(cpu)) return 0;
    if (cpu->gpr[4]!=1 || cpu->gpr[5])
        psx_fatal_halt("X5 resident callback changed its CdReady ABI");
    cpu->gpr[2]=1;return 1;
}
int get_sector(CPUState* cpu,uint32_t) {
    if (!callback_caller(cpu)) return 0;
    const uint32_t words=cpu->gpr[5];
    if (words>585 || !ram(cpu->gpr[4],words*4))
        psx_fatal_halt("X5 resident sector destination is out of bounds");
    const auto* bytes=sector.take_words(words);
    if (!bytes || !psx_mod_dma_write_ram(cpu->gpr[4],bytes,words*4,int(sector_lba)))
        psx_fatal_halt("X5 resident sector read exceeded its verified view");
    cpu->gpr[2]=1;return 1;
}
template<unsigned N> bool caller(const MMX5Caller (&list)[N],uint32_t ra) {
    for (const auto& c:list) if (ra==c.ra && word(ra-8)==c.jal) return true;
    return false;
}
bool blocking(CPUState* cpu,bool packed) {
    const uint32_t ra=cpu->gpr[31],sp=cpu->gpr[29];
    if (!ram(sp,48)) return false;
    if (packed) return ra==0x80014920 && caller(MMX5PackedCallers,word(sp+40));
    if (ra==0x80015F94) return caller(MMX5BinCallers,word(sp+28));
    if (ra==0x80015B60) return caller(MMX5DatCallers,word(sp+36));
    // Original restart sites inside the three blocking loading-screen waits.
    return ra==0x800141A8 || ra==0x800156CC || ra==0x80015910;
}
bool member(unsigned archive,unsigned index,uint32_t lba,uint32_t bytes,bool packed) {
    const auto& a=MMX5Archives[archive];
    if (index>=a.count) return false;
    const auto& m=archive?MMX5Bin[index]:MMX5Dat[index];
    if (m.packed!=packed || lba!=a.lba+m.offset || bytes!=m.size ||
        lba!=psx_resident_file_lba(pack,archive)+m.offset) return false;
    // The original initializer resolves every descriptor. Validate the whole
    // archive table so requests cannot be served from a partially old map.
    for (unsigned i=0;i<a.count;++i) {
        const auto& d=archive?MMX5Bin[i]:MMX5Dat[i];
        if (word(a.table+i*8)!=a.lba+d.offset || word(a.table+i*8+4)!=d.size) return false;
    }
    return true;
}
int start(CPUState* cpu,uint32_t pc) {
    if (!pack || pumping || !psx_mod_game_started()) return 0;
    const bool packed=pc==0x80014944;
    if (!blocking(cpu,packed)) {
        psx_mod_counter_add("mmx5.resident.background_original",1);return 0;
    }
    const uint32_t lba=word(Lba),bytes=word(Remain);
    const unsigned index=psx_mod_read_byte(Index);
    uint32_t archive=UINT32_MAX;
    const uint32_t rounded=(bytes+2047)&~2047u;
    const auto* data=psx_resident_find_lba(pack,lba,rounded,&archive);
    if (!bytes || bytes>UINT32_MAX-2047 || !data || archive>=2 ||
        !member(archive,index,lba,bytes,packed) || word(Packed)!=unsigned(packed) ||
        (!packed && (word(Dest)<0x80092000 || !ram(word(Dest),(bytes+3)&~3u))) ||
        cdrom_data_read_active() || cdrom_fmv_stream_pending() ||
        !psx_resident_guest_ranges_match(MMX5LoaderRanges,5,MMX5LoaderSha)) {
        psx_resident_record("mmx5.archive-request",archive,lba,bytes,0);return 0;
    }
    // The original start routine's public state; Setmode/Setloc/ReadN and the
    // three-frame drive wait are replaced by scoped resident sector delivery.
    psx_mod_write_word(Count,0);call(cpu,0x80066BC8,lba,0x800E5A48);
    if (!packed) psx_mod_write_word(Lba,lba-1);
    psx_mod_write_byte(Status,1);pumping=true;
    for (uint32_t offset=0;psx_mod_read_byte(Status)==1;offset+=2048) {
        if (offset>=rounded || !sector.assign(lba+offset/2048,data+offset,offset+2048==rounded))
            psx_fatal_halt("X5 resident callback left its verified member");
        sector_lba=lba+offset/2048;
        call(cpu,packed?0x80014A30:0x80015C00);
        if (sector.consumed()!=2060)
            psx_fatal_halt("X5 resident callback did not consume one data sector");
        if (psx_mod_read_byte(Pending)) {
            call(cpu,0x800152B8); // original deferred GPU/SPU upload queue
            call(cpu,0x80068634); // original DrawSync(0), before ring reuse
        }
        psx_mod_counter_add("mmx5.resident.sectors",1);
    }
    pumping=false;
    if (psx_mod_read_byte(Status)!=2 || psx_mod_read_byte(Pending))
        psx_fatal_halt("X5 resident loader violated its completion contract");
    psx_resident_record("mmx5.archive-request",archive,lba,bytes,1);
    // Packed start's last lui leaves v0=800A0000; raw start returns1.
    // Preserve those original return values even where current callers ignore v0.
    psx_mod_counter_add("mmx5.resident.requests",1);
    cpu->gpr[2]=packed?0x800A0000u:1u;return 1;
}
void activate() {
    PSXResidentSpec spec{};spec.struct_size=sizeof spec;
    spec.title="MegaManX5Recomp";spec.format="slus01334-archive-sectors-v1";
    spec.files=MMX5Files;spec.file_count=2;spec.policy=PSX_RESIDENT_REQUIRE_STOCK;
    pack=psx_resident_prepare(&spec);pumping=false;
}
}
extern "C" const uint8_t* mmx5_resident_dat(uint32_t* size) {
    if(size)*size=0;
    if(!pack || !mod_runtime_snapshot_package_active("mmx5.enhancement.resident-loading"))
        return nullptr;
    return psx_resident_file(pack,0,size,nullptr);
}
PSX_MOD_CONSTRUCTOR(mmx5_register_resident) {
    psx_mod_register_activation_plugin(Plugin,activate);
    psx_mod_register_function_filter_plugin(Plugin,0x80014574,start);
    psx_mod_register_function_filter_plugin(Plugin,0x80014944,start);
    psx_mod_register_function_filter_plugin(Plugin,0x80066D78,ready);
    psx_mod_register_function_filter_plugin(Plugin,0x80067198,get_sector);
}
