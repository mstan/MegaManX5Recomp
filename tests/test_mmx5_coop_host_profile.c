#define PSX_EXECUTION_PROFILE_CONFIG "mmx5_host_profile_test.h"
#include "../src/mods/mmx5_coop_host_profile.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t digest(void){return 1;}
static PSXModNetplayProfile title={.plugin_id="mmx5.coop",.compatibility_id="mmx5-coop-delay-v1:test",
    .skip_bios_intro=1,.state_digest=digest,.requires_retained_texture_banks=1,
    .settings_package_id="mmx5.coop",.settings_feature_id="coop",
    .renderer_option_id="cameras",.renderer_option_value="split"};
const PSXModNetplayProfile *psx_mod_netplay_profile(void){return &title;}
static uint8_t artifact[32]={1};
static PSXModNetplayHostStartup valid(void){
    PSXModNetplayHostStartup c={0};
    c.cpu=(PSXHostCPU){PSX_HOST_ARCH_X64,1,0x7f};
    c.compiled_backends=c.available_backends=PSX_HOST_BACKEND_ENHANCED;
    c.fp_contract=2;c.interpreted_overlays_sealed=c.builtin_linked_bios=1;
    c.actual_executable_sha256=artifact;c.declared_build_manifest_schema=1;
    c.declared_build_manifest_sha256="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    c.builtin_bios_image_id="OPENBIOS";c.builtin_bios_image_wordsum=0xc4daeb88u;
    c.slot_count=c.player_count=2;c.occupied_mask=3;c.renderer=1;c.retained_texture_banks=1;
    return c;
}
static void refused(PSXModNetplayHostStartup c){
    PSXHostProfileCatalog cat={(const PSXHostProfileRule *)(uintptr_t)1,9};
    CHECK(!mmx5_coop_host_profiles(&c,&cat)&&!cat.count&&!cat.rules);
}
#define REJECT(field,value) do{PSXModNetplayHostStartup c=valid();c.field=(value);refused(c);}while(0)
int main(void){
    PSXModNetplayHostStartup c=valid();PSXHostProfileCatalog cat={0};
    CHECK(mmx5_coop_host_profiles(&c,&cat)==MMX5_HOST_TEST_EXPECT_OFFER);
    if(MMX5_HOST_TEST_EXPECT_OFFER){
        CHECK(cat.count==1&&!strcmp(cat.rules->profile,MMX5_RELEASE_HOST_PROFILE));
        CHECK(cat.rules->flags==PSX_HOST_PROFILE_PORTABLE);
        uint8_t hash[32];psx_sha256_compute((const uint8_t*)MMX5_RELEASE_HOST_POLICY,strlen(MMX5_RELEASE_HOST_POLICY),hash);
        CHECK(!memcmp(hash,cat.rules->qualification,32));
        PSXHostCapabilities peers[2];
        for(unsigned i=0;i<2;++i){PSXHostCPU cpu={PSX_HOST_ARCH_X64,1,i?0x3fu:0x7fu};
            CHECK(psx_host_caps_init(peers+i,&cpu,2,2,artifact));
            CHECK(psx_host_caps_offer(peers+i,cat.rules,2)&&psx_host_caps_validate(peers+i,&cat));}
        PSXHostMatch match;uint8_t session[32]={2};psx_host_match_init(&match);
        CHECK(psx_host_match_select(&match,peers,2,&cat,session)==PSX_HOST_OK);
        peers[1].build[0]^=2;psx_host_match_init(&match);
        CHECK(psx_host_match_select(&match,peers,2,&cat,session)==PSX_HOST_BUILD_MISMATCH);
        c.cpu.features=0;c.player_count=0;c.occupied_mask=0;c.direct_loopback_scope=1;
        CHECK(mmx5_coop_host_profiles(&c,&cat));
    }
    REJECT(cpu.architecture,PSX_HOST_ARCH_ARM64);REJECT(cpu.features_known,0);REJECT(cpu.features,0x80);
    REJECT(compiled_backends,1);REJECT(available_backends,3);REJECT(fp_contract,1);
    REJECT(interpreted_overlays_sealed,0);REJECT(builtin_linked_bios,0);
    REJECT(builtin_bios_image_id,"SCPH-1001");REJECT(builtin_bios_image_wordsum,0);
    REJECT(actual_executable_sha256,NULL);uint8_t zero[32]={0};REJECT(actual_executable_sha256,zero);
    REJECT(declared_build_manifest_schema,0);REJECT(declared_build_manifest_schema,2);REJECT(declared_build_manifest_sha256,"bad");
    REJECT(slot_count,3);REJECT(player_count,1);REJECT(occupied_mask,1);REJECT(rollback,1);
    REJECT(renderer,0);REJECT(retained_texture_banks,0);REJECT(aspect_index,1);REJECT(direct_loopback_scope,2);
    c=valid();title.plugin_id="other";refused(c);
    puts("native host-policy admission and exact-artifact rejection checked; LAN is a separate release gate");
    return 0;
}
