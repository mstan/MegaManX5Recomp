#pragma once
#include <stdint.h>
#include "cpu_state.h"
#include "mmx5_coop_snapshot.h"

#define MMX5_PLAYER 0x8009A0A0u
#define MMX5_PLAY 0x800D1C00u

int mmx5_coop_audio_activate(void);
int mmx5_coop_audio_prepare(unsigned character,const uint8_t *dat,uint32_t size,int initialize_guest);
int mmx5_coop_audio_load(unsigned character,const uint8_t *dat,uint32_t size);
uint32_t mmx5_coop_audio_digest(uint32_t hash);
int mmx5_coop_audio_snapshot_ready(void);
void mmx5_coop_audio_snapshot(Mmx5SnapshotIO *s);
void mmx5_coop_audio_restored(void);

/* Borrowed read-only contexts. Native PLAYER holds P2 during projection. */
int mmx5_coop_ready(void);
/* A guarded silent-hint filter cancels only its just-admitted gather. */
void mmx5_coop_silent_hint(void);
/* Only suppress the shared stop/music call in the second native menu step. */
int mmx5_coop_menu_secondary(void);
int mmx5_coop_projected(void);
int mmx5_coop_split_views(void);
/* Native Zero/Black Zero choice, including the current unit preview. */
int mmx5_coop_zero_form(void);
const uint8_t *mmx5_coop_first_body(void);
const uint8_t *mmx5_coop_second_body(void);
uint16_t mmx5_coop_input(unsigned seat);
uint32_t mmx5_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1);
int mmx5_coop_finish(CPUState *cpu,uint32_t result);
