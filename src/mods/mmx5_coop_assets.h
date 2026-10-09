#ifndef MMX5_COOP_ASSETS_H
#define MMX5_COOP_ASSETS_H

#include <stddef.h>
#include <stdint.h>

/* Views into a caller-owned, mounted-disc file. No copyrighted assets are
 * included with the mod. This implementation targets SLUS-01334. */
typedef struct {
    const uint8_t *data;
    size_t size;
    uint32_t type;
} Mmx5AssetView;

/* Raw DAT records include portrait pixels/palettes without a subasset header. */
int mmx5_coop_dat_record(const uint8_t *dat, size_t size, unsigned record,
                        Mmx5AssetView *out);
int mmx5_coop_dat_asset(const uint8_t *dat, size_t size, unsigned record,
                        unsigned asset, Mmx5AssetView *out);
int mmx5_coop_overlay(const uint8_t *bin, size_t size, unsigned index,
                      Mmx5AssetView *out);
/* Native word LZ format. Returns success only at the explicit
 * terminator; malformed input never reads/writes outside its supplied spans. */
int mmx5_coop_decode_sprite(const uint8_t *src, size_t size,
                            uint8_t *dst, size_t capacity, size_t *written);

#endif
