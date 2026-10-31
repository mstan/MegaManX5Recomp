#include "mod_plugins.h"
#include "mmx5_resident_data.h"
const uint8_t *mmx5_resident_dat(uint32_t *size) {
    if(size)*size=0;
    return 0;
}
// Fixed REFERENCE builds register the matching feature without HLE hooks.
static void activate(void) {}
PSX_MOD_CONSTRUCTOR(mmx5_register_resident_reference) {
    psx_mod_register_activation_plugin("mmx5.resident-loading",activate);
}
