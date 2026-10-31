/* SLUS-01334 private character audio. The original VAB parser and voice
 * driver retain their metadata and channel selection, with independent P2
 * residency and voices. See docs/COOP_AUDIO_PARITY_EVIDENCE.md. */
#include "mmx5_coop_internal.h"
#include "mmx5_coop_assets.h"
#include "mod_plugins.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ID "mmx5.coop"
#define SOUND 0x80016490u
#define VOICE_KEY_ON 0x8005D458u
#define VAB_OPEN 0x80062688u
#define VAB_STATUS 0x800EA7A0u
#define SOUND_END 0x8009A730u
#define SOUND_RECORDS 0x8009A770u
#define PRIORITY 0x800E8028u
#define VOICE_REGS 0x800EA4B0u
#define VOICE_META 0x800E9F50u
#define VOICE_DIRTY 0x800EA490u
#define QUEUE 0x800EA760u
#define KEY_MASKS 0x800E9F40u
typedef struct { Mmx5AssetView header,samples; uint32_t guest,offset,id; unsigned loaded,initialized; } AudioBank;
static AudioBank banks[2][2];
static unsigned sound_call,voice_call,stop_call;
static uint32_t audio_state;
/* Derived residency belongs to one SPU boot, not to the saved simulation. */
static uint32_t bound_epoch;
static void read_bytes(uint32_t address,uint8_t *out,unsigned n) {
    for(unsigned i=0;i<n;++i)out[i]=psx_mod_read_byte(address+i);
}
static void write_bytes(uint32_t address,const uint8_t *in,unsigned n) {
    for(unsigned i=0;i<n;++i)psx_mod_write_byte(address+i,in[i]);
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void copy_header(AudioBank *bank) {
    /* Mod allocations use the private guest aperture, outside hardware RAM. */
    uint32_t at=0;
    for(;at+4<=bank->header.size;at+=4)
        psx_mod_write_word(bank->guest+at,le32(bank->header.data+at));
    for(;at<bank->header.size;++at)
        psx_mod_write_byte(bank->guest+at,bank->header.data[at]);
    /* The record's upper two bits select direct/sequence sound routing.
     * Preserve them; the lower six bits select the original VAB handle. */
    for(at=8;at<bank->offset;at+=4)
        psx_mod_write_byte(bank->guest+at,
            (uint8_t)((bank->header.data[at]&0xC0u)|bank->id));
}
void mmx5_coop_audio_reset(void) {
    for(unsigned c=0;c<2;++c)for(unsigned g=0;g<2;++g) {
        free((void*)banks[c][g].header.data);free((void*)banks[c][g].samples.data);
    }
    memset(banks,0,sizeof banks);sound_call=voice_call=stop_call=0;audio_state=bound_epoch=0;
}
static int bind_sample_banks(void) {
    uint32_t epoch=psx_mod_spu_sample_bank_epoch();
    if(!epoch)return 0;
    if(epoch==bound_epoch)return 1;
    for(unsigned character=0;character<2;++character)for(unsigned group=0;group<2;++group)
        if(!banks[character][group].loaded)return 0;
    for(unsigned character=0;character<2;++character)for(unsigned group=0;group<2;++group) {
        const AudioBank *bank=&banks[character][group];
        if(!psx_mod_spu_sample_bank(bank->id,bank->samples.data,(uint32_t)bank->samples.size))return 0;
    }
    bound_epoch=epoch;return 1;
}
int mmx5_coop_audio_activate(void) {
    mmx5_coop_audio_reset();
    for(unsigned character=0;character<2;++character)for(unsigned group=0;group<2;++group) {
        AudioBank *bank=&banks[character][group];
        bank->id=12u+character*2u+group;
        bank->guest=psx_mod_alloc_guest_memory(0x2000,16);
        if(!bank->guest)return 0;
    }
    audio_state=psx_mod_alloc_guest_memory(32,16);
    if(!audio_state)return 0;
    for(unsigned i=0;i<32;++i)psx_mod_write_byte(audio_state+i,0);
    return 1;
}
int mmx5_coop_audio_prepare(unsigned character,const uint8_t *file,uint32_t size,int initialize_guest) {
    if(character>1)return 0;
    static const unsigned members[2]={10,12};
    static const uint32_t types[2]={0x20101u,0x20303u};
    for(unsigned group=0;group<2;++group) {
        AudioBank *bank=&banks[character][group];
        if(bank->loaded)continue;
        Mmx5AssetView head,body;
        if(!mmx5_coop_dat_asset(file,size,character?90:88,members[group],&head) || head.type!=6 ||
           !mmx5_coop_dat_asset(file,size,character?90:88,members[group]+1,&body) || body.type!=types[group] ||
           head.size<40 || head.size>0x2000 || !body.size || body.size%16)return 0;
        uint32_t offset=le32(head.data)&0xFFFFFFu;
        if(offset<8 || offset%4 || offset>head.size-32 ||
           le32(head.data+4)!=body.size || memcmp(head.data+offset,"pBAV",4))return 0;
        if(!bank->guest || bank->id!=12u+character*2u+group)return 0;
        uint8_t *head_copy=malloc(head.size),*body_copy=malloc(body.size);
        if(!head_copy || !body_copy) {free(head_copy);free(body_copy);return 0;}
        memcpy(head_copy,head.data,head.size);memcpy(body_copy,body.data,body.size);
        bank->header=head;bank->header.data=head_copy;
        bank->samples=body;bank->samples.data=body_copy;
        bank->offset=offset;bank->loaded=1;
    }
    /* Activation precedes spu_init(), which discards immutable sample banks.
     * Cache first, then bind once after that boot boundary has happened. */
    if(initialize_guest) {
        if(!bind_sample_banks())return 0;
        for(unsigned group=0;group<2;++group) {
            AudioBank *bank=&banks[character][group];
            if(!bank->initialized) {copy_header(bank);bank->initialized=1;}
        }
    }
    return audio_state!=0;
}
int mmx5_coop_audio_load(unsigned character,const uint8_t *file,uint32_t size) {
    return mmx5_coop_audio_prepare(character,file,size,1);
}
static int open_bank(CPUState *cpu,AudioBank *bank) {
    if(!bind_sample_banks())return 0;
    if(psx_mod_read_byte(VAB_STATUS+bank->id)==1)return 1;
    copy_header(bank);
    /* Sticky open parses the original header at a logical sample base of 0.
     * It does not allocate or upload SPU RAM. Private KEYON bindings resolve
     * those addresses against the immutable host bank instead. */
    uint32_t a2=cpu->gpr[6];cpu->gpr[6]=0;
    uint32_t result=mmx5_coop_call(cpu,VAB_OPEN,bank->guest+bank->offset,bank->id);
    cpu->gpr[6]=a2;
    if(result!=bank->id)return 0;
    /* Native open leaves status 2 (awaiting body) and the shared transfer
     * gate set. Our body already resides in the immutable bank: publish the
     * same ready status 1 and release that gate without issuing a DMA. */
    psx_mod_write_byte(VAB_STATUS+bank->id,1);
    mmx5_coop_call(cpu,0x8005FCC8u,0,0);
    fprintf(stderr,"mmx5.coop: private-audio bank=%u header=%08X ready=1\n",
        bank->id,bank->guest);
    return 1;
}
static int character_sound(CPUState *cpu,uint32_t address) {
    unsigned group=cpu->gpr[4]&0x7Fu;
    if(sound_call || !mmx5_coop_ready() || !mmx5_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX5_PLAY)!=10 ||
       (group!=1 && group!=3))return 0;
    unsigned character=psx_mod_read_byte(MMX5_PLAYER+2);
    if(character>1)return 0;
    AudioBank *bank=&banks[character][group==3];if(!bank->loaded)return 0;
    sound_call=1;
    if(!open_bank(cpu,bank)) {
        sound_call=0;psx_mod_counter_add("mmx5.coop.audio-bank-open-failed",1);
        return mmx5_coop_finish(cpu,(uint32_t)-1);
    }
    uint32_t end=SOUND_END+group*4,records=SOUND_RECORDS+group*4;
    uint32_t saved_end=psx_mod_read_word(end),saved_records=psx_mod_read_word(records);
    psx_mod_write_word(end,bank->guest+bank->offset);
    psx_mod_write_word(records,bank->guest+8);
    uint8_t priority[24];read_bytes(PRIORITY,priority,sizeof priority);
    for(unsigned voice=0;voice<24;++voice)
        psx_mod_write_byte(PRIORITY+voice,psx_mod_read_byte(audio_state+voice));
    uint32_t result=mmx5_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    for(unsigned voice=0;voice<24;++voice)
        psx_mod_write_byte(audio_state+voice,psx_mod_read_byte(PRIORITY+voice));
    write_bytes(PRIORITY,priority,sizeof priority);
    psx_mod_write_word(end,saved_end);psx_mod_write_word(records,saved_records);
    sound_call=0;return mmx5_coop_finish(cpu,result);
}
static int voice_key_on(CPUState *cpu,uint32_t address) {
    if(voice_call || !mmx5_coop_ready() || psx_mod_local_view_scope())return 0;
    uint32_t id=cpu->gpr[5];uint32_t bank=0;
    if(id>=12 && id<16 && banks[(id-12)/2][(id-12)%2].loaded)bank=id;
    unsigned voice=cpu->gpr[4];
    if(bank && voice<24u) {
        /* Let the original VAB/tone driver compute pitch, stereo gain and
         * ADSR, then start P2's private voice. Restore its queued hardware
         * record before the next native flush, including P1's pending KEYON.
         * The finite calculation is uncharged so no interrupt can flush this
         * temporary record between calculation and restoration. */
        uint8_t regs[16],meta[0x38],queue[0x40],history[0x44],masks[12];
        uint8_t dirty=psx_mod_read_byte(VOICE_DIRTY+voice);
        read_bytes(VOICE_REGS+voice*16u,regs,sizeof regs);
        read_bytes(VOICE_META+voice*0x38u,meta,sizeof meta);
        read_bytes(QUEUE,queue,sizeof queue);read_bytes(KEY_MASKS,masks,sizeof masks);
        read_bytes(0x800EA638u,history,sizeof history);
        voice_call=1;
        uint32_t result=psx_mod_call_guest_uncharged(cpu,address,cpu->gpr[31],
            voice,id,cpu->gpr[6],cpu->gpr[7],0,NULL);
        if(result!=(uint32_t)-1) {
            uint16_t calculated[8];
            for(unsigned r=0;r<8;++r)calculated[r]=psx_mod_read_half(VOICE_REGS+voice*16u+r*2u);
            calculated[7]=calculated[3];
            unsigned mode=(psx_mod_read_half(0x800EA796u)==0xFFu?1u:0u)|
                (psx_mod_read_byte(0x800EA792u)&4u?2u:0u);
            if(!psx_mod_spu_private_voice_play(voice,bank,calculated,mode)) {
                result=(uint32_t)-1;psx_mod_counter_add("mmx5.coop.private-voice-failed",1);
            }
        }
        write_bytes(VOICE_REGS+voice*16u,regs,sizeof regs);
        write_bytes(VOICE_META+voice*0x38u,meta,sizeof meta);
        write_bytes(QUEUE,queue,sizeof queue);write_bytes(KEY_MASKS,masks,sizeof masks);
        write_bytes(0x800EA638u,history,sizeof history);
        psx_mod_write_byte(VOICE_DIRTY+voice,dirty);
        psx_mod_counter_add(result==(uint32_t)-1?"mmx5.coop.audio-key-failed":"mmx5.coop.private-voice-plays",1);
        voice_call=0;return mmx5_coop_finish(cpu,result);
    }
    /* The original driver queues KEYON for a later SPU update. Bind its
     * explicit voice slot now; the binding survives that delay and rollback. */
    psx_mod_spu_bind_voice_bank(cpu->gpr[4],bank);
    voice_call=1;
    uint32_t result=mmx5_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(result==(uint32_t)-1)psx_mod_spu_bind_voice_bank(cpu->gpr[4],0);
    voice_call=0;return mmx5_coop_finish(cpu,result);
}
static int voice_status(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!sound_call || !mmx5_coop_projected())return 0;
    return mmx5_coop_finish(cpu,(psx_mod_spu_private_voice_active()&cpu->gpr[4])?1u:0u);
}
static int private_stop(CPUState *cpu,uint32_t address) {
    if(stop_call!=1 || cpu->gpr[4]>=24u)return 0;
    unsigned voice=cpu->gpr[4];
    if(address==0x8005D7C4u)psx_mod_spu_private_voice_stop(1u<<voice);
    else psx_mod_spu_private_voice_volume(voice,
        (uint16_t)((int16_t)cpu->gpr[5]*129),(uint16_t)((int16_t)cpu->gpr[6]*129));
    return mmx5_coop_finish(cpu,0);
}
static int character_stop(CPUState *cpu,uint32_t address) {
    unsigned group=cpu->gpr[4]&255u;
    if(group==255u && mmx5_coop_menu_secondary())return mmx5_coop_finish(cpu,0);
    if(stop_call || !mmx5_coop_ready() || psx_mod_local_view_scope())return 0;
    if(group==255u) {
        stop_call=2;
        uint32_t result=mmx5_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        for(unsigned voice=0;voice<24;++voice)psx_mod_spu_private_voice_volume(voice,0,0);
        psx_mod_spu_private_voice_stop(0xFFFFFFu);stop_call=0;
        return mmx5_coop_finish(cpu,result);
    }
    if(!mmx5_coop_projected() || (group!=1 && group!=3))return 0;
    unsigned character=psx_mod_read_byte(MMX5_PLAYER+2);
    if(character>1 || !banks[character][group==3].loaded)return 0;
    uint32_t pointer=SOUND_RECORDS+group*4u,saved=psx_mod_read_word(pointer);
    psx_mod_write_word(pointer,banks[character][group==3].guest+8u);stop_call=1;
    uint32_t result=mmx5_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    stop_call=0;psx_mod_write_word(pointer,saved);
    return mmx5_coop_finish(cpu,result);
}
PSX_MOD_CONSTRUCTOR(mmx5_register_coop_audio) {
    psx_mod_register_function_filter_plugin(ID,SOUND,character_sound);
    psx_mod_register_function_filter_plugin(ID,VOICE_KEY_ON,voice_key_on);
    psx_mod_register_function_filter_plugin(ID,0x80016D54u,character_stop);
    psx_mod_register_function_filter_plugin(ID,0x80059988u,voice_status);
    psx_mod_register_function_filter_plugin(ID,0x8005D7C4u,private_stop);
    psx_mod_register_function_filter_plugin(ID,0x8005E448u,private_stop);
}

uint32_t mmx5_coop_audio_digest(uint32_t hash) {
    const unsigned flags[]={sound_call,voice_call,stop_call};
    for(unsigned i=0;i<3;++i)hash=(hash^flags[i])*16777619u;
    for(unsigned c=0;c<2;++c)for(unsigned g=0;g<2;++g) {
        hash=(hash^banks[c][g].loaded)*16777619u;
        hash=(hash^banks[c][g].initialized)*16777619u;
    }
    if(audio_state)for(unsigned i=0;i<32;++i)hash=(hash^psx_mod_read_byte(audio_state+i))*16777619u;
    return hash;
}

int mmx5_coop_audio_snapshot_ready(void) {
    if(sound_call || voice_call || stop_call || !audio_state)return 0;
    for(unsigned c=0;c<2;++c)for(unsigned g=0;g<2;++g)if(!banks[c][g].loaded)return 0;
    return 1;
}
void mmx5_coop_audio_snapshot(Mmx5SnapshotIO *s) {
    mmx5_snapshot_fixed(s,audio_state);
    for(unsigned c=0;c<2;++c)for(unsigned g=0;g<2;++g) {
        AudioBank *b=&banks[c][g];
        mmx5_snapshot_fixed(s,b->guest);mmx5_snapshot_fixed(s,b->offset);
        mmx5_snapshot_fixed(s,b->id);mmx5_snapshot_fixed(s,(uint32_t)b->header.size);
        mmx5_snapshot_fixed(s,(uint32_t)b->samples.size);
        MMX5_SNAPSHOT_BOOL(s,b->initialized);
    }
}
void mmx5_coop_audio_restored(void) {
    /* Device restoration preserves private voices and guest VAB metadata.
     * Rebind immutable host samples only; never reopen a native VAB here. */
    bound_epoch=0;(void)bind_sample_banks();
}
