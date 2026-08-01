#include "mod_plugins.h"

/*
 * Mega Man X5's full-2D classification, widened background tile window, HUD
 * bounds, and object/primitive cull hooks live in generated/runtime code and
 * remain identity transforms at 4:3. This trusted activation plugin moves the
 * player-facing switch out of generic Display settings and into the mod catalog.
 */
#define PKG "mmx5.enhancement.widescreen"
#define FEATURE "widescreen"

static void mmx5_widescreen_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
}

PSX_MOD_CONSTRUCTOR(mmx5_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx5.widescreen", mmx5_widescreen_activate);
}
