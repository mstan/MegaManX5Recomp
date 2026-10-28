#pragma once
#include <stdint.h>
/* Host draw bookkeeping must restore alongside the renderer's RAM/VRAM
 * transaction. The authored renderer remains the sole terrain producer. */
typedef struct {
    uint32_t ui_begin;
    int ui_pending,ui_screen,ui_x_delta,supplemental_scan;
} Mmx5WidescreenViewContext;
void mmx5_widescreen_view_save(Mmx5WidescreenViewContext *context);
void mmx5_widescreen_view_restore(const Mmx5WidescreenViewContext *context);

/* Immutable pre-transition sheets; project only inside an existing private draw. */
int mmx5_widescreen_prepare_region_atlases(const uint8_t *dat,uint32_t bytes);
/* Success also includes scopes that do not need a substitution. */
int mmx5_widescreen_project_region_atlas(void);
