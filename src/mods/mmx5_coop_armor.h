#ifndef MMX5_COOP_ARMOR_H
#define MMX5_COOP_ARMOR_H
#include <stdint.h>

enum { MMX5_COOP_ARMOR_SOURCES = 6 };
unsigned mmx5_coop_armor_source(unsigned roster);
unsigned mmx5_coop_armor_record(unsigned source);
unsigned mmx5_coop_armor_list(uint8_t list[6], unsigned count);
uint16_t mmx5_coop_armor_input(uint16_t primary_edges, uint16_t x_edges);
unsigned mmx5_coop_armor_layers(unsigned roster,unsigned visual,unsigned action,unsigned frame,unsigned groups[4]);
#endif
