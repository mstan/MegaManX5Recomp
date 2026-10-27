#include "mmx5_coop_armor.h"
#include <stdio.h>
#include <string.h>
#define REQUIRE(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while(0)
int main(void) {
    uint8_t zero_start[6]={0,5};
    REQUIRE(mmx5_coop_armor_list(zero_start,2)==2);
    REQUIRE(!memcmp(zero_start,(uint8_t[]){0,1,0,0,0,0},6));
    uint8_t all[6]={0,1,3,2,5,4};
    REQUIRE(mmx5_coop_armor_list(all,6)==5);
    REQUIRE(!memcmp(all,(uint8_t[]){0,1,3,2,4,0},6));
    uint8_t partial[6]={0,2,5};
    REQUIRE(mmx5_coop_armor_list(partial,3)==3);
    REQUIRE(!memcmp(partial,(uint8_t[]){0,1,2,0,0,0},6));
    /* Native13458 is the record oracle; source1 remains Zero. */
    static const unsigned records[5]={88,92,94,95,93};
    for(unsigned form=0;form<5;++form)
        REQUIRE(mmx5_coop_armor_record(mmx5_coop_armor_source(form))==records[form]);
    REQUIRE(mmx5_coop_armor_record(1)==90);
    /* P1 keeps Cancel. Only the X seat supplies movement/confirmation. */
    REQUIRE(mmx5_coop_armor_input(0xA850,0)==0x10);
    REQUIRE(mmx5_coop_armor_input(0,0xA850)==0xA840);
    REQUIRE(mmx5_coop_armor_input(0x8000,0x2000)==0x2000);
    unsigned groups[4];
    REQUIRE(mmx5_coop_armor_layers(1,0,0,0,groups)==4);
    REQUIRE(!memcmp(groups,(unsigned[]){3,0,2,1},sizeof groups));
    REQUIRE(mmx5_coop_armor_layers(4,5,1,0x7E,groups)==1 && groups[0]==5);
    REQUIRE(!mmx5_coop_armor_layers(2,0,1,0x7E,groups));
    REQUIRE(!mmx5_coop_armor_layers(0,0,2,0,groups));
    return 0;
}
