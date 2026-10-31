#include "mmx5_coop_assets.h"
#include "mmx5_coop_sprite_packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", what); exit(1); }
}
static void put32(uint8_t *p, unsigned v) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i));
}
int main(void) {
    /* Native unflipped SPRT_16 and flipped FT4 must both survive projection.
     * Exercise signed XY, palette/blend flags and the last tile of a page. */
    uint32_t sprite[10]={0x03123456,0x7F808080,0x0064FFFD,0x7800F0F0};
    uint16_t tile;
    check(mmx5_coop_sprite_quad(sprite,&tile),"unflipped sprite accepted");
    check(tile==0xF0F0 && sprite[0]==0x03123456 && sprite[1]==0x2F808080,"sprite identity");
    check(sprite[2]==0x0064FFFD && sprite[4]==0x0064000D &&
          sprite[6]==0x0074FFFD && sprite[8]==0x0074000D,"full sixteen-pixel coverage");
    check(sprite[3]==0x78000000 && sprite[5]==0x10 &&
          sprite[7]==0x1000 && sprite[9]==0x1010,"tile-local UVs do not wrap");
    uint32_t flipped[10]={0x09123456,0x2D808080,0x00640010,0x7800000F,
                         0x00640020,0x00200000,0x00740010,0x0000100F,0x00740020,0x1000};
    uint32_t original[10]; memcpy(original,flipped,sizeof original);
    check(mmx5_coop_sprite_quad(flipped,&tile) && tile==UINT16_MAX &&
          !memcmp(original,flipped,sizeof original),"native flipped geometry preserved");
    /* Original Zero animation12/frame2E uses tile0 at (-32,-10).
     * With facing40 and a private origin, native24FC8 clips its right edge
     * to15px. Restore16px with reflected pixels, preserving CLUT/page/blend.
     * This is a source-derived native packet, not a runtime capture. */
    uint32_t clipped[10]={0x09123456,0x2F808080,0x005A00D8,0x780D000F,
                         0x005A00E7,0x00250000,0x006A00D8,0x0000100F,
                         0x006A00E7,0x00001000};
    check(mmx5_coop_sprite_quad_tile(clipped)==1,"first-column X reflection key");
    check(clipped[0]==0x09123456 && clipped[1]==0x2F808080 &&
          clipped[3]==0x780D0000 && clipped[5]==0x00250010,
          "reflected packet keeps link blend palette and native page");
    check(clipped[4]==0x005A00E8 && clipped[8]==0x006A00E8 &&
          clipped[7]==0x1000 && clipped[9]==0x1010,
          "clipped native edge becomes full sixteen-pixel square");
    uint16_t atlas[256*256],part[256];
    for(unsigned i=0;i<256*256;++i)atlas[i]=(uint16_t)i;
    mmx5_coop_sprite_tile_pixels(atlas,1,part);
    check(part[0]==15 && part[15]==0 && part[240]==3855 && part[255]==3840,
          "X reflection samples same tile without adjoining pixels");
    mmx5_coop_sprite_tile_pixels(atlas,0xF0F3,part);
    check(part[0]==65535 && part[15]==65520 && part[240]==61695 && part[255]==61680,
          "last-page XY reflection stays within its authored tile");
    mmx5_coop_sprite_tile_pixels(atlas,0x1010,part);
    check(part[0]==4112 && part[255]==7967,"ordinary sprite origin remains unchanged");
    /* Class15's lingering plasma uses native image FT4s. A Y-reflected
     * endpoint is one row before the authored tile; sampling the minimum
     * would import that adjoining row into the orb. Preserve its actual
     * row16 origin, blend/page/CLUT and complete sixteen-pixel coverage. */
    uint32_t plasma[10]={0x09123456,0x2F808080,0x007300BA,0x78001F20,
                        0x007300CA,0x00251F30,0x008300BA,0x00000F20,
                        0x008300CA,0x00000F30};
    uint16_t plasma_tile=mmx5_coop_sprite_quad_tile(plasma);
    check(plasma_tile==0x1022 && plasma[3]==0x78000000 &&
          plasma[5]==0x00250010 && plasma[7]==0x1000 && plasma[9]==0x1010,
          "static plasma keeps authored image origin with Y reflection");
    mmx5_coop_sprite_tile_pixels(atlas,plasma_tile,part);
    check(part[0]==7968 && part[15]==7983 && part[240]==4128 && part[255]==4143,
          "reflected plasma tile excludes preceding texture row");
    sprite[1]=0x60000000;
    check(!mmx5_coop_sprite_quad(sprite,&tile),"unsupported primitive rejected");
    uint8_t out[32], archive[8192] = {0};
    size_t written;
    Mmx5AssetView view;
    /* Literal 1234, overlapping distance-one repeat, zero fill, terminator. */
    const uint8_t stream[] = {0x00,0x70, 0x34,0x12, 0x01,0x18,
                             0x00,0x10, 0x00,0x00, 0x00,0x00};
    check(mmx5_coop_decode_sprite(stream,sizeof stream,out,sizeof out,&written),"decode");
    check(written == 12,"decoded length");
    for (unsigned i=0;i<8;i+=2) check(out[i]==0x34 && out[i+1]==0x12,"overlapping copy");
    for (unsigned i=8;i<12;++i) check(!out[i],"zero fill");
    for (size_t n=0;n<sizeof stream;++n)
        check(!mmx5_coop_decode_sprite(stream,n,out,sizeof out,&written),"truncation rejected");
    check(!mmx5_coop_decode_sprite(stream,sizeof stream,out,10,&written),"output bound");
    const uint8_t bad[] = {0,0x80,1,8};
    check(!mmx5_coop_decode_sprite(bad,sizeof bad,out,sizeof out,&written),"backreference bound");
    const uint8_t extended[] = {0,0xc0,0,0,3,0,0,0,0,0};
    check(mmx5_coop_decode_sprite(extended,sizeof extended,out,sizeof out,&written) && written==6,"extended zero run");
    put32(archive+85*8,2); put32(archive+85*8+4,4096);
    put32(archive+4096,1); put32(archive+4100,4096);
    put32(archive+4104,3); put32(archive+4108,4);
    archive[6144]=0xAB;
    check(mmx5_coop_dat_asset(archive,sizeof archive,85,0,&view) &&
          view.data==archive+6144 && view.size==4 && view.type==3,"DAT ownership");
    check(!mmx5_coop_dat_asset(archive,6145,85,0,&view),"DAT truncation");
    check(!mmx5_coop_dat_asset(archive,sizeof archive,85,1,&view),"DAT asset index");
    put32(archive+166*8,3); put32(archive+166*8+4,512);
    check(mmx5_coop_dat_record(archive,sizeof archive,166,&view) &&
          view.data==archive+6144 && view.size==512,"raw portrait palette record");
    check(!mmx5_coop_dat_record(archive,6655,166,&view),"raw record truncation");
    check(!mmx5_coop_dat_record(archive,sizeof archive,256,&view),"raw record index bound");
    put32(archive,1); put32(archive+4,4);
    check(mmx5_coop_overlay(archive,sizeof archive,0,&view) && view.data==archive+2048,"overlay sector origin");
    put32(archive,0xffffffff);
    check(!mmx5_coop_overlay(archive,sizeof archive,0,&view),"overlay extent bound");
    puts("MMX5 co-op asset checks passed");
    return 0;
}
