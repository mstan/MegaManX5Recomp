#include "mmx5_coop_armor.h"

/* Native13458 selects these actual DAT records. Keep source1 for Zero and
 * source2 for Ultimate, including retained UI banks and duel appearance. */
unsigned mmx5_coop_armor_source(unsigned roster) {
    static const uint8_t sources[5] = {0,3,4,5,2};
    return roster < 5 ? sources[roster] : 0;
}
unsigned mmx5_coop_armor_record(unsigned source) {
    static const uint8_t records[MMX5_COOP_ARMOR_SOURCES] = {88,90,93,92,94,95};
    return source < MMX5_COOP_ARMOR_SOURCES ? records[source] : 0;
}
unsigned mmx5_coop_armor_list(uint8_t list[6], unsigned count) {
    /* Native order: base, Fourth, Gaea, Falcon, Ultimate. Fourth is always
     * available in co-op; other forms still require native availability.
     * Do not change the campaign/unlock bytes that native card saves own. */
    unsigned available = 3u;
    if(count > 6)count = 6;
    for(unsigned i=0;i<count;++i)if(list[i]<5)available |= 1u << list[i];
    static const uint8_t order[5] = {0,1,3,2,4};
    unsigned used = 0;
    for(unsigned i=0;i<5;++i)if(available & (1u << order[i]))list[used++] = order[i];
    for(unsigned i=used;i<6;++i)list[i] = 0;
    return used;
}
uint16_t mmx5_coop_armor_input(uint16_t primary_edges, uint16_t x_edges) {
    /* Byte-swapped native PAD+4: P1 keeps Cancel, X owns L/R and Confirm. */
    return (uint16_t)((primary_edges & 0x10u) | (x_edges & 0xA840u));
}
unsigned mmx5_coop_armor_layers(unsigned roster,unsigned visual,unsigned action,unsigned frame,unsigned groups[4]) {
    /* Original27370: Fourth draws four pieces in this order. Group0 is a
     * real armor group; it must never be confused with the base-body pass. */
    if(roster==1) {
        groups[0]=visual+3;groups[1]=visual;groups[2]=visual+2;groups[3]=visual+1;
        return 4;
    }
    if(!roster || roster>4 || (roster!=4 && action==1 && frame==0x7E))return 0;
    groups[0]=visual;return 1;
}
