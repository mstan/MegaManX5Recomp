#include "mod_capcom_bg2d.h"
#include "cpu_state.h"
#include "mod_netplay.h"
#include "mmx5_widescreen_internal.h"
#include "mmx5_coop_internal.h"
#include "mmx5_coop_assets.h"
#include "gpu_render.h"
#include <string.h>

/* SLUS-01334 modern renderer. The retired fixed-16:9 ring-widening path has
 * no generated hooks. Retail producers remain 21x16, within the 32-column
 * ring and 1000-packet budget. Shared code builds the actual drawable view. */
#define PKG "mmx5.enhancement.widescreen"
#define FEATURE "widescreen"
#define LAYERS 0x8009a1f8u
static uint32_t arena;
/* Immutable initial scenery for the intro and Dinorex's first section.
 * Native area controllers replace the shared atlas at their next region.
 * A retained old-region camera still requires the original sheet. Keep only
 * ROM-derived host words here; there is no new mutable simulation state. */
static uint16_t initial_region_atlases[2][512u * 256u];
static unsigned initial_region_atlas_ready[2];

static int prepare_region_atlas(const uint8_t *dat,uint32_t bytes,
                                unsigned slot,unsigned record,unsigned asset) {
    if (initial_region_atlas_ready[slot]) return 1;
    Mmx5AssetView image;
    if (!mmx5_coop_dat_asset(dat, bytes, record, asset, &image) ||
        image.type != 0x10000u || image.size != 0x40000u) return 0;
    /* Original type10000 uploads64-word by16-row blocks down each column,
     * beginning at VRAM320,256. Type16 is already linear and is different.
     * Decode words explicitly so host byte order cannot change the image. */
    for (unsigned block = 0; block < 128u; ++block) {
        unsigned x = (block / 16u) * 64u, y = (block % 16u) * 16u;
        for (unsigned row = 0; row < 16u; ++row)
            for (unsigned col = 0; col < 64u; ++col) {
                unsigned source = block * 2048u + (row * 64u + col) * 2u;
                initial_region_atlases[slot][(y + row) * 512u + x + col] =
                    (uint16_t)(image.data[source] | (uint16_t)image.data[source + 1u] << 8);
            }
    }
    initial_region_atlas_ready[slot] = 1;
    return 1;
}
int mmx5_widescreen_prepare_region_atlases(const uint8_t *dat,uint32_t bytes) {
    int intro=prepare_region_atlas(dat,bytes,0,100,5);
    int dinorex=prepare_region_atlas(dat,bytes,1,105,3);
    return intro && dinorex;
}

int mmx5_widescreen_project_region_atlas(void) {
    /* Only an existing private-view RAM/VRAM transaction may substitute this
     * old-region sheet. The original world request, progression, map,
     * descriptors, palette, camera and the other view remain untouched. */
    if (!psx_mod_local_view_scope() ||
        !mmx5_coop_split_views() || psx_mod_read_byte(0x800d1c00u) != 10u ||
        psx_mod_read_byte(0x800d1c0du) ||
        psx_mod_read_word(0x800e92f8u) != 2u) return 1;
    unsigned slot;
    if (!psx_mod_read_byte(0x800d1c0cu)) {
        if (psx_mod_read_word(0x800ee970u)!=4u ||
            psx_mod_read_word(0x80056070u)!=0xac6292f8u ||
            psx_mod_read_word(0x8007c758u)!=0x0abf0aa8u ||
            psx_mod_read_word(0x8007c760u)!=0xffff0a08u ||
            (int16_t)psx_mod_read_half(LAYERS+10u)>=2568) return 1;
        slot=0;
    } else if (psx_mod_read_byte(0x800d1c0cu)==4u) {
        if (psx_mod_read_word(0x800ee970u)!=9u ||
            psx_mod_read_word(0x800eeb8cu)!=0xac8392f8u ||
            psx_mod_read_word(0x800f4828u)!=0x08800020u ||
            psx_mod_read_half(0x800f4834u)!=2192u ||
            (int16_t)psx_mod_read_half(LAYERS+14u)>=2192) return 1;
        slot=1;
    } else return 1;
    if (!initial_region_atlas_ready[slot]) {
        psx_mod_counter_add(slot?"mmx5.renderer.dinorex-atlas-unavailable":
                                 "mmx5.renderer.intro-atlas-unavailable",1);
        return 0;
    }
    /* Same native rectangle800718D8. The renderer transfer journals the old
     * words BEFORE touching the CPU mirror and preserves queued ordering;
     * a GP0 A0 payload would mutate that mirror before journal protection. */
    gr_vram_transfer_in(320,256,512,256,initial_region_atlases[slot]);
    if(slot)psx_mod_counter_add("mmx5.renderer.dinorex-old-region-atlas",1);
    return 1;
}
static const PSXCapcomBackground background = {0x80093f78u, 0x80091d60u, 3u, 17u, 0x7900u};

static void mmx5_bg_begin(CPUState *cpu, uint32_t address) {
    (void)address;
    gpu_ws_bg2d_begin_view_layer(cpu->gpr[4], psx_mod_read_word(0x1f800108u), 6u);
}

typedef struct GrizzlyTilePatch {
    int left,top,right,bottom,source_x,source_y;
} GrizzlyTilePatch;
/* Draw-call scratch derived from live native area actors, never a second
 * animation clock or persistent world state. Native F79C0 advances +1B and
 * copies a source rectangle into the retail tile ring. The full-map renderer
 * must resolve that same rectangle instead of its static authored tiles. */
static GrizzlyTilePatch grizzly_tile_patches[96];
static unsigned grizzly_tile_patch_count;
static void mmx5_grizzly_tile_patches(unsigned layer) {
    grizzly_tile_patch_count=0;
    if(layer)return;
    for(uint32_t actor=0x8009E7A0u;actor<0x8009F9A0u;actor+=0x30u) {
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=9u ||
           psx_mod_read_byte(actor+2)>=4u || psx_mod_read_byte(actor+4)!=1u)continue;
        uint32_t parent=psx_mod_read_word(actor+0x20);
        unsigned frame=psx_mod_read_byte(actor+0x1B);
        if(parent<0x80092090u || parent>=0x80093DD0u ||
           (parent-0x80092090u)%0x9Cu || !psx_mod_read_byte(parent) ||
           psx_mod_read_byte(parent+1)!=0x10u || psx_mod_read_byte(parent+0x87)>=2u ||
           frame>=18u)continue;
        uint32_t shape=0x800FD408u+psx_mod_read_byte(actor+2)*6u;
        if(psx_mod_read_byte(shape)!=layer)continue;
        uint32_t source=0x800FD30Cu+frame*8u;
        GrizzlyTilePatch *p=&grizzly_tile_patches[grizzly_tile_patch_count++];
        p->left=(int16_t)psx_mod_read_half(actor+10)-(int16_t)psx_mod_read_half(shape+2);
        p->top=(int16_t)psx_mod_read_half(actor+14)-(int16_t)psx_mod_read_half(shape+4);
        p->right=p->left+psx_mod_read_half(source+4)*16;
        p->bottom=p->top+psx_mod_read_half(source+6)*16;
        p->source_x=(int16_t)psx_mod_read_half(source);
        p->source_y=(int16_t)psx_mod_read_half(source+2);
    }
}
static void mmx5_grizzly_panorama_tile(unsigned layer, int *x, int *y,
                                       unsigned *mirror_x, uint16_t *bank) {
    (void)mirror_x; (void)bank;
    if(!layer) {
        for(unsigned i=0;i<grizzly_tile_patch_count;++i) {
            const GrizzlyTilePatch *p=&grizzly_tile_patches[i];
            if(*x>=p->left && *x<p->right && *y>=p->top && *y<p->bottom) {
                *x=p->source_x+*x-p->left;*y=p->source_y+*y-p->top;
                psx_mod_counter_add("mmx5.renderer.grizzly-animated-tiles",1);
                return;
            }
        }
        return;
    }
    /* Original28328 wraps a newly exposed column through camera4D..4E.
     * The authored map renderer must sample the same column rather than
     * discard it at the interval boundary while a full view straddles it.
     * Negative sourceX retains28340's original no-column behavior. */
    if (!layer || layer >= 3u || *x < 0) return;
    uint32_t camera = LAYERS + layer * 0x54u;
    int first = (int)psx_mod_read_byte(camera + 0x4du) * 256;
    int end = ((int)psx_mod_read_byte(camera + 0x4eu) + 1) * 256;
    int span = end - first;
    if (span > 0 && (*x < first || *x >= end)) {
        int relative = (*x - first) % span;
        if (relative < 0) relative += span;
        *x = first + relative;
    }
}

static void mmx5_bg_end(CPUState *cpu, uint32_t address) {
    (void)address;
    unsigned layer = cpu->gpr[4];
    uint32_t native_packet = psx_mod_read_word(0x1f800108u);
    gpu_ws_bg2d_end_view_layer(layer, native_packet);
    WsViewAnchor view;
    if (layer >= 3) return;
    if (!gpu_ws_bg2d_get_view(layer, &view)) {
        /* At native4:3 the wide anchor is absent. Independent cameras and
         * their shared scenes still need authored tiles; the retail ring
         * belongs to the last canonical camera and can be a distant room. */
        if(!mmx5_coop_split_views())return;
        view=(WsViewAnchor){0,16,0,0,0};
    }
    if(mmx5_coop_split_views() && view.left<=0 && view.right<=0)view.right=16;
    uint32_t b = LAYERS + layer * 0x54u;
    int sx = (int16_t)psx_mod_read_half(b + 10u);
    int sy = (int16_t)psx_mod_read_half(b + 14u);
    int parent = (int8_t)psx_mod_read_byte(b + 0x52u);
    if (parent >= 0 && parent < 3) {
        uint32_t p = LAYERS + (unsigned)parent * 0x54u;
        sx += (int16_t)psx_mod_read_half(p + 10u);
        sy += (int16_t)psx_mod_read_half(p + 14u);
    }
    /* Layer2 mode1 calls 8002AAB4 -> 8002AB64: floor(foreground_x/2)
     * plus its authored offset. Mode4 uses the same X mapping at 8002AAE0.
     * Keep its skyline on the native half-speed mapping across the drawable,
     * rather than copying and independently clamping the foreground view. */
    unsigned mode = psx_mod_read_byte(b + 4u);
    if (layer == 2u && parent < 0 && (mode == 1u || mode == 4u)) {
        WsViewAnchor foreground;
        if (gpu_ws_bg2d_get_view(0, &foreground)) {
            int camera = (int16_t)psx_mod_read_half(LAYERS + 10u);
            view = psx_capcom_parallax_view(foreground, 2, camera);
            psx_mod_counter_add("mmx5.renderer.half-speed-view", 1);
        }
    }
    PSXCapcomTileMap map = {
        psx_mod_read_word(0x1f800004u), psx_mod_read_word(0x1f800008u),
        psx_mod_read_word(0x1f80000cu), psx_mod_read_byte(0x800d1dbcu),
        psx_mod_read_byte(0x800d1dbdu), psx_mod_read_half(0x80091d58u),
        layer, psx_mod_read_byte(b + 0x4du), psx_mod_read_byte(b + 0x4eu)
    };
    PSXCapcomTilePolicy policy = NULL;
    grizzly_tile_patch_count=0;
    if (map.left_screen <= map.right_screen && map.right_screen < map.width &&
        psx_mod_read_byte(0x800d1c0cu) == 1u && !psx_mod_read_byte(0x800d1c0du) &&
        psx_mod_read_word(0x800ee970u) == 5u &&
        psx_mod_read_word(0x800283e8u) == 0x9107004du &&
        psx_mod_read_word(0x80028448u) == 0x9087004eu &&
        psx_mod_read_word(0x800F7A7Cu) == 0x0C00A288u &&
        psx_mod_read_word(0x800F7C88u) == 0x0C03DE70u &&
        (!layer || mode == 5u)) {
        mmx5_grizzly_tile_patches(layer);
        policy = mmx5_grizzly_panorama_tile;
    }
    int packets = psx_capcom_background_render(&background, arena,
        psx_mod_read_word(0x1f800000u), &map, sx, sy, view, native_packet, policy);
    if (packets >= 0) psx_mod_counter_add("mmx5.renderer.tiles", (uint32_t)packets);
    else psx_mod_counter_add("mmx5.renderer.invalid-layer", 1);
}

/* UI shares the object packet arena with scenery. Use the retail camera
 * selector rather than an address heuristic, preserving composite dialogue. */
static uint32_t ui_begin;
static int ui_pending, ui_screen, ui_x_delta;
static int mmx5_barrier(uint32_t object) {
    /* Retail category5/type36 scenery uses a negative camera selector and
     * screen-relative coordinates. It is not UI: 800F11BC scrolls its stored
     * placement X at trunc(5*foreground_camera/4), then calls 8002D9EC. */
    return (psx_mod_read_byte(object) & 0x60u) == 0x40u &&
        psx_mod_read_byte(object + 1u) == 0x36u &&
        (int8_t)psx_mod_read_byte(object + 0x14u) < 0;
}
static int barrier_scroll(int camera) { return (5 * camera) / 4; }
static int mmx5_foreground_view(WsViewAnchor *view) {
    if (!gpu_ws_bg2d_get_view(0, view)) return 0;
    /* Objects precede this frame's BG setup. Use the current retail camera,
     * not the previous BG sample, when crossing a room edge. */
    int extra = (view->left + view->right + view->pad_left + view->pad_right) / 2;
    *view = ws_view_anchor(extra, (int16_t)psx_mod_read_half(LAYERS + 10u),
        (int16_t)psx_mod_read_half(LAYERS + 0x1eu),
        (int16_t)psx_mod_read_half(LAYERS + 0x1cu));
    return 1;
}
static void finish_ui(uint32_t end) {
    if (ui_pending && end >= ui_begin && end - ui_begin <= 1000u * 0x28u) {
        for (uint32_t p = ui_begin; p < end; p += 0x28u) {
            psx_mod_tag_world_primitive(p, !ui_screen);
            if (ui_screen) gpu_ws_tag_hud_prim(p, 0);
            if (ui_x_delta) {
                unsigned op = psx_mod_read_byte(p + 7u) & 0xfdu;
                unsigned vertices = op == 0x2du ? 4u : op == 0x7du ? 1u : 0u;
                for (unsigned i = 0; i < vertices; ++i) {
                    uint32_t xy = p + 8u + i * 8u;
                    int x = (int16_t)psx_mod_read_half(xy);
                    psx_mod_write_half(xy, (uint16_t)(x + ui_x_delta));
                }
            }
        }
    }
    ui_pending = 0;
}
static void mmx5_object_begin(CPUState *cpu, uint32_t address) {
    uint32_t packet = psx_mod_read_word(0x1f800100u);
    finish_ui(packet);
    unsigned selector = address == 0x80024c60u ? 0x14u : 0x37u;
    ui_screen = (int8_t)psx_mod_read_byte(cpu->gpr[4] + selector) < 0;
    ui_x_delta = 0;
    if (address == 0x80024c60u && mmx5_barrier(cpu->gpr[4])) {
        WsViewAnchor view;
        ui_screen = 0;
        if (mmx5_foreground_view(&view)) {
            int camera = (int16_t)psx_mod_read_half(LAYERS + 10u);
            ui_x_delta = barrier_scroll(camera) - barrier_scroll(camera - view.left) - view.left;
            psx_mod_counter_add("mmx5.renderer.foreground-barrier", 1);
        }
    }
    ui_begin = packet;
    ui_pending = 1;
}
static void mmx5_object_end(CPUState *cpu, uint32_t address) {
    (void)cpu; (void)address;
    finish_ui(psx_mod_read_word(0x1f800100u));
}

static void mmx5_actor_bounds(CPUState *cpu, uint32_t address) {
    (void)address;
    if (mmx5_barrier(cpu->gpr[4])) {
        /* Co-op owns each private view's native scenery origin, independently
         * of this optional renderer. Only extend its visible margin here. */
        WsViewAnchor view;
        if (mmx5_foreground_view(&view)) {
            int reveal = view.left > view.right ? view.left : view.right;
            cpu->gpr[5] += (uint32_t)((5 * reveal + 3) / 4 + 1);
        }
    } else if ((int8_t)psx_mod_read_byte(cpu->gpr[4] + 0x14u) >= 0)
        cpu->gpr[5] += (uint32_t)psx_mod_widescreen_x_margin();
}
static int mmx5_fixed_draw(CPUState *cpu, uint32_t address) {
    if (psx_mod_widescreen_x_margin() <= 0 ||
        (int8_t)psx_mod_read_byte(cpu->gpr[4] + 0x14u) < 0) return 0;
    cpu->gpr[5] = address == 0x8002d89cu ? 32u : 96u;
    cpu->gpr[6] = address == 0x8002d89cu ? 32u : 80u;
    uint32_t caller = cpu->gpr[31];
    psx_dispatch_call(cpu, 0x8002d9ecu, caller);
    return 1;
}
static int mmx5_fixed_lifetime(CPUState *cpu, uint32_t address) {
    (void)address;
    if (psx_mod_widescreen_x_margin() <= 0 ||
        (int8_t)psx_mod_read_byte(cpu->gpr[4] + 0x14u) < 0) return 0;
    cpu->gpr[5] = 64u; cpu->gpr[6] = 64u;
    uint32_t caller = cpu->gpr[31];
    psx_dispatch_call(cpu, 0x8002d938u, caller);
    return 1;
}

static int supplemental_scan;
void mmx5_widescreen_view_save(Mmx5WidescreenViewContext *context) {
    *context=(Mmx5WidescreenViewContext){ui_begin,ui_pending,ui_screen,ui_x_delta,supplemental_scan};
}
void mmx5_widescreen_view_restore(const Mmx5WidescreenViewContext *context) {
    ui_begin=context->ui_begin;ui_pending=context->ui_pending;
    ui_screen=context->ui_screen;ui_x_delta=context->ui_x_delta;
    supplemental_scan=context->supplemental_scan;
}
static int mmx5_extra_placement(CPUState *cpu, uint32_t address) {
    (void)address;
    if (!supplemental_scan) return 0;
    uint32_t record = cpu->gpr[4];
    unsigned category = psx_mod_read_byte(record + 3u) & 15u;
    unsigned type = psx_mod_read_byte(record + 1u);
    /* Original intro list: ordinary enemies are types0C/0D/0F. Type31 is
     * the boss and type2F a special encounter; other categories own scripts,
     * NPCs and effects. Retain their native activation, difficulty and latches. */
    if ((category < 3u && type < 0x20u) || (category == 5u && type == 0x36u)) {
        psx_mod_counter_add(category == 5u ? "mmx5.renderer.extra-scenery" : "mmx5.renderer.extra-enemy", 1);
        return 0;
    }
    cpu->gpr[2] = 1;
    return 1;
}
static int signed_bound(int x) { return x < -32768 ? -32768 : x > 32767 ? 32767 : x; }
static void mmx5_scan_view(CPUState *cpu, uint32_t address) {
    (void)address;
    int margin = psx_mod_widescreen_x_margin();
    unsigned state = psx_mod_read_word(0x800d1c00u);
    /* Attract demo and player-controlled stages have distinct retail states. */
    if (margin <= 0 || (state != 9u && state != 10u)) return;
    int x = (int16_t)psx_mod_read_half(LAYERS + 10u);
    int y = (int16_t)psx_mod_read_half(LAYERS + 14u);
    CPUState saved = *cpu;
    for (unsigned side = 0; side < 2; ++side) {
        *cpu = saved;
        cpu->gpr[29] -= 32u;
        uint32_t arg = cpu->gpr[29] + 16u;
        uint32_t original_arg = psx_mod_read_word(arg);
        cpu->gpr[4] = (uint32_t)signed_bound(side ? x - 48 - margin : x + 368);
        cpu->gpr[5] = (uint32_t)signed_bound(side ? x - 48 : x + 368 + margin);
        cpu->gpr[6] = (uint32_t)signed_bound(y - 48);
        cpu->gpr[7] = (uint32_t)signed_bound(y + 288);
        psx_mod_write_word(arg, side ? 2u : 1u);
        cpu->gpr[31] = side ? 0x8002af04u : 0x8002aebcu;
        supplemental_scan = 1;
        psx_dispatch_call(cpu, 0x8002b070u, cpu->gpr[31]);
        supplemental_scan = 0;
        psx_mod_write_word(arg, original_arg);
        if (cpu->muldiv_ts_done > saved.muldiv_ts_done) saved.muldiv_ts_done = cpu->muldiv_ts_done;
        if (cpu->gte_ts_done > saved.gte_ts_done) saved.gte_ts_done = cpu->gte_ts_done;
    }
    *cpu = saved;
}

static void mmx5_widescreen_activate(void) {
    arena = psx_capcom_background_activate(3u);
    if (!arena) return;
    gpu_ws_bg2d_configure(LAYERS, 0x800a51a8u, 0x800d1dbcu, 0x80091d58u, 32u, 3u, 0x54u, 1000u);
    gpu_ws_set_view_anchor(LAYERS + 10u, LAYERS + 0x1eu, LAYERS + 0x1cu, LAYERS);
    (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", 0x80027f88u, mmx5_bg_begin);
    (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", 0x80027d84u, mmx5_bg_end);
    static const uint32_t renderers[] = {0x80024c60u, 0x80025240u, 0x8002553cu};
    for (unsigned i = 0; i < sizeof renderers / sizeof renderers[0]; ++i)
        (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", renderers[i], mmx5_object_begin);
    (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", 0x80024b88u, mmx5_object_end);
    (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", 0x8002d938u, mmx5_actor_bounds);
    (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", 0x8002d9ecu, mmx5_actor_bounds);
    (void)psx_mod_register_function_filter_plugin("mmx5.widescreen", 0x8002d89cu, mmx5_fixed_draw);
    (void)psx_mod_register_function_filter_plugin("mmx5.widescreen", 0x8002daa0u, mmx5_fixed_draw);
    (void)psx_mod_register_function_filter_plugin("mmx5.widescreen", 0x8002d800u, mmx5_fixed_lifetime);
    (void)psx_mod_register_function_entry_plugin("mmx5.widescreen", 0x8002ae50u, mmx5_scan_view);
    (void)psx_mod_register_function_filter_plugin("mmx5.widescreen", 0x8002afb4u, mmx5_extra_placement);
    if(psx_mod_netplay_is_active()) {
        int aspect=psx_mod_netplay_aspect();
        (void)psx_mod_set_fixed_display_aspect(aspect==2?21u:aspect==1?16u:4u,aspect?9u:3u);
        return;
    }
    char aspect[16];
    if (!psx_mod_option_value(PKG, FEATURE, "aspect", aspect, sizeof aspect)) strcpy(aspect, "adaptive");
    if (!strcmp(aspect,"16:9")) (void)psx_mod_set_fixed_display_aspect(16u,9u);
    else if (!strcmp(aspect,"21:9")) (void)psx_mod_set_fixed_display_aspect(21u,9u);
    else if (!strcmp(aspect,"32:9")) (void)psx_mod_set_fixed_display_aspect(32u,9u);
    else {
        /* Seed the first window at16:9, then fit the live drawable. Both
         * requests use this modern renderer; there is no legacy path. */
        (void)psx_mod_set_fixed_display_aspect(16u,9u);
        (void)psx_mod_set_adaptive_display_aspect(0u,0u);
    }
}

PSX_MOD_CONSTRUCTOR(mmx5_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx5.widescreen", mmx5_widescreen_activate);
}
