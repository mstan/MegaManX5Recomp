#ifndef MMX5_COOP_PIXEL_CODEC_H
#define MMX5_COOP_PIXEL_CODEC_H
#include <stdint.h>

/* Snapshot wire only; decoded cache pixels and their identity stay unchanged.
 * LE u16 packets: bit15 repeats one following pixel; otherwise literal pixels.
 * Low15+1 is the pixel count (1..32768). Runs of at least3 are emitted greedily.
 * Raw LE u16 is used whenever packets would not be smaller. No allocation. */
enum { MMX5_COOP_PIXELS_RAW, MMX5_COOP_PIXELS_RUNS };
#define MMX5_COOP_PIXELS_MAX_COUNT (1024u * 512u)

int mmx5_coop_pixels_measure(const uint16_t *pixels,uint32_t count,
                             uint8_t *encoding,uint32_t *bytes);
int mmx5_coop_pixels_encode(uint8_t *out,uint32_t bytes,
                            const uint16_t *pixels,uint32_t count,uint8_t encoding);
/* NULL out validates only. All packet/length bounds are checked BEFORE any
 * output write, so rejection leaves even the offside destination unchanged. */
int mmx5_coop_pixels_decode(const uint8_t *in,uint32_t bytes,uint32_t count,
                            uint8_t encoding,uint16_t *out);
#endif
