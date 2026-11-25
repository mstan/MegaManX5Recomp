#include "build_manifest.h"
#include "mod_plugins.h"
#include <stdio.h>

/* Keep the actual target's compiled manifest present even before a transport
 * envelope consumes it. No guest state, settings or network behavior changes. */
static const char *volatile mmx5_build_manifest_anchor;
PSX_MOD_CONSTRUCTOR(mmx5_embed_build_manifest) {
    mmx5_build_manifest_anchor = psx_compiled_build_manifest_sha256();
    fprintf(stderr,"mmx5.build: declared manifest schema=%u sha256=%s\n",
        psx_compiled_build_manifest_schema(),psx_compiled_build_manifest_sha256());
}

const char *mmx5_declared_build_manifest_sha256(void) {
    return psx_compiled_build_manifest_sha256();
}

unsigned mmx5_declared_build_manifest_schema(void) {
    return psx_compiled_build_manifest_schema();
}
