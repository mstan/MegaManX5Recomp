#include "mmx5_coop_bindings.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void) {
    uint8_t card[32]={0},out[7],p2[7]={0x10,0x80,2,0x40,0x20,8,4};
    for(unsigned i=0;i<11;++i){card[i*2]=(uint8_t)i;card[i*2+1]=0x80;}
    uint8_t original[22];memcpy(original,card,22);
    CHECK(!mmx5_bindings_decode(card,out));
    mmx5_bindings_encode(card,p2);
    CHECK(!memcmp(card,original,22));
    CHECK(mmx5_bindings_decode(card,out) && !memcmp(p2,out,7));
    /* Native card serialization copies the complete mapping bytes. */
    uint8_t saved[32];memcpy(saved,card,32);memset(card,0,32);memcpy(card,saved,32);
    CHECK(mmx5_bindings_decode(card,out) && !memcmp(p2,out,7));
    for(unsigned i=22;i<32;++i) {
        card[i]^=1;CHECK(!mmx5_bindings_decode(card,out));card[i]^=1;
    }
    p2[1]=p2[0];mmx5_bindings_encode(card,p2);CHECK(!mmx5_bindings_decode(card,out));
    p2[1]=3;mmx5_bindings_encode(card,p2);CHECK(!mmx5_bindings_decode(card,out));
    p2[1]=0;mmx5_bindings_encode(card,p2);CHECK(!mmx5_bindings_decode(card,out));
    puts("Independent binding persistence preserves native P1 data and rejects invalid metadata");
    return 0;
}
