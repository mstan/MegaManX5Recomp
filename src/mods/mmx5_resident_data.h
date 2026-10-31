#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the active loader's verified effective DAT. Emulation thread only;
 * copy needed assets before returning to the game. NULL means use disc I/O. */
const uint8_t *mmx5_resident_dat(uint32_t *size);
#ifdef __cplusplus
}
#endif
