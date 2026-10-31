#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A replay overrides accepted seat input, never physical device bindings. */
int mmx5_diagnostics_resolve_input(unsigned seat,uint16_t *buttons);
void mmx5_diagnostics_reset(void);
/* Direct debug fixtures are inspection evidence, not input-replayable runs. */
void mmx5_diagnostics_development_fixture(unsigned command,unsigned seat,unsigned hp,unsigned ammo);
/* A native input callback has a host stack until its guest call returns. */
int mmx5_diagnostics_snapshot_ready(void);
#ifdef __cplusplus
}
#endif
