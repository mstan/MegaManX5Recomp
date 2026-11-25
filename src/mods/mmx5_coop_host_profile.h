#ifndef MMX5_COOP_HOST_PROFILE_H
#define MMX5_COOP_HOST_PROFILE_H
#include "mod_netplay.h"
#if defined(_WIN32)
#define MMX5_RELEASE_HOST_PROFILE "mmx5.win64.enhanced.release"
#define MMX5_RELEASE_HOST_ABI "win64-mingw-gcc15.2.0"
#else
#define MMX5_RELEASE_HOST_PROFILE "mmx5.linux.enhanced.release"
#define MMX5_RELEASE_HOST_ABI "linux-x64-gcc13.3.0"
#endif
#define MMX5_RELEASE_HOST_EXECUTION "740637ffbe6d670c854742a60d47e532182397eace53a4a5dcc3a262dc59e046"
#define MMX5_RELEASE_HOST_POLICY MMX5_RELEASE_HOST_PROFILE "/v1;kernel=host-native-v1;abi=" MMX5_RELEASE_HOST_ABI ";execution=" MMX5_RELEASE_HOST_EXECUTION ";cpu=x64-portable-known;fp=2;bios=OPENBIOS:c4daeb88;overlays=interpreted-sealed;manifest=sha256-v1;artifact=exact;seats=2;rollback=off;renderer=retained-opengl;aspect=4:3;debug=off;route=normal"
int mmx5_coop_host_profiles(const PSXModNetplayHostStartup *,PSXHostProfileCatalog *);
#endif
