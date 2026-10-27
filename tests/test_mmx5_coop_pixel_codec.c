#include "mmx5_coop_pixel_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)

static uint32_t roundtrip(const uint16_t *pixels,uint32_t count,int expected) {
    uint8_t encoding;uint32_t bytes;
    CHECK(mmx5_coop_pixels_measure(pixels,count,&encoding,&bytes));
    if(expected>=0)CHECK(encoding==(uint8_t)expected);
    CHECK(bytes<=count*2u);
    uint8_t *wire=malloc(bytes?bytes:1u);
    uint16_t *decoded=malloc(count?count*2u:2u);
    CHECK(wire && decoded);
    CHECK(mmx5_coop_pixels_encode(wire,bytes,pixels,count,encoding));
    CHECK(mmx5_coop_pixels_decode(wire,bytes,count,encoding,NULL));
    CHECK(mmx5_coop_pixels_decode(wire,bytes,count,encoding,decoded));
    CHECK(!count || !memcmp(pixels,decoded,count*2u));
    uint8_t repeat_encoding;uint32_t repeat_bytes;
    CHECK(mmx5_coop_pixels_measure(decoded,count,&repeat_encoding,&repeat_bytes));
    CHECK(repeat_encoding==encoding && repeat_bytes==bytes);
    uint8_t *repeat=malloc(bytes?bytes:1u);CHECK(repeat);
    CHECK(mmx5_coop_pixels_encode(repeat,bytes,decoded,count,repeat_encoding));
    CHECK(!bytes || !memcmp(wire,repeat,bytes));
    free(repeat);free(decoded);free(wire);return bytes+5u;
}
static void rejected(const uint8_t *in,uint32_t bytes,uint32_t count,uint8_t encoding) {
    uint16_t output[16],before[16];
    for(unsigned i=0;i<16;++i)output[i]=before[i]=0xD00D;
    CHECK(!mmx5_coop_pixels_decode(in,bytes,count,encoding,output));
    CHECK(!memcmp(output,before,sizeof output));
}
static uint32_t read32(FILE *f) {
    uint8_t b[4];CHECK(fread(b,1,4,f)==4);
    return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;
}
/* Optional owner-local corpus: X5PX, LE u32 case count, then each LE u32
 * pixel count followed by raw LE u16 pixels. Original art is never tracked. */
static void actual_corpus(const char *path) {
    FILE *f=fopen(path,"rb");CHECK(f);
    uint8_t magic[4];CHECK(fread(magic,1,4,f)==4 && !memcmp(magic,"X5PX",4));
    uint32_t cases=read32(f);CHECK(cases && cases<=8192u);
    uint64_t raw=0,encoded=0;
    for(uint32_t i=0;i<cases;++i) {
        uint32_t count=read32(f);CHECK(count<=MMX5_COOP_PIXELS_MAX_COUNT);
        uint8_t *wire=malloc(count?count*2u:1u);
        uint16_t *pixels=malloc(count?count*2u:2u);CHECK(wire && pixels);
        CHECK(fread(wire,2,count,f)==count);
        CHECK(mmx5_coop_pixels_decode(wire,count*2u,count,MMX5_COOP_PIXELS_RAW,pixels));
        raw+=(uint64_t)count*2u;encoded+=roundtrip(pixels,count,-1);
        free(pixels);free(wire);
    }
    CHECK(fgetc(f)==EOF);fclose(f);
    printf("Actual retained-art corpus: %u buffers, %llu raw bytes -> %llu pixel-wire bytes\n",
           cases,(unsigned long long)raw,(unsigned long long)encoded);
}
int main(int argc,char **argv) {
    uint8_t encoding;uint32_t bytes;
    CHECK(mmx5_coop_pixels_measure(NULL,0,&encoding,&bytes) &&
          encoding==MMX5_COOP_PIXELS_RAW && !bytes);
    CHECK(mmx5_coop_pixels_encode(NULL,0,NULL,0,MMX5_COOP_PIXELS_RAW));
    CHECK(mmx5_coop_pixels_decode(NULL,0,0,MMX5_COOP_PIXELS_RAW,NULL));
    CHECK(!mmx5_coop_pixels_measure(NULL,1,&encoding,&bytes));
    CHECK(!mmx5_coop_pixels_measure(NULL,MMX5_COOP_PIXELS_MAX_COUNT+1u,&encoding,&bytes));
    const uint16_t endian_pixels[]={0x1234,0xABCD};
    uint8_t endian_wire[4];
    CHECK(mmx5_coop_pixels_encode(endian_wire,4,endian_pixels,2,MMX5_COOP_PIXELS_RAW));
    CHECK(!memcmp(endian_wire,"\x34\x12\xCD\xAB",4));
    CHECK(roundtrip(endian_pixels,2,MMX5_COOP_PIXELS_RAW)==9u);

    uint16_t *pixels=malloc(MMX5_COOP_PIXELS_MAX_COUNT*2u);CHECK(pixels);
    for(uint32_t i=0;i<MMX5_COOP_PIXELS_MAX_COUNT;++i)pixels[i]=0x801F;
    CHECK(roundtrip(pixels,65536,MMX5_COOP_PIXELS_RUNS)==13u); /* two32768 packets */
    CHECK(roundtrip(pixels,32769,MMX5_COOP_PIXELS_RUNS)==13u); /* final literal1 */
    CHECK(roundtrip(pixels,MMX5_COOP_PIXELS_MAX_COUNT,MMX5_COOP_PIXELS_RUNS)==69u);
    for(uint32_t i=0;i<65536;++i)pixels[i]=(uint16_t)i;
    roundtrip(pixels,65536,MMX5_COOP_PIXELS_RAW);
    /* Full native-sized atlas with transparent gaps and distinct color/STP
     * values; runs/literals span row boundaries without changing pixels. */
    memset(pixels,0,65536u*2u);
    for(uint32_t y=7;y<39;++y)for(uint32_t x=11;x<91;++x)
        pixels[y*256u+x]=(uint16_t)(0x8000u|((x+y*3u)&0x7FFFu));
    roundtrip(pixels,65536,MMX5_COOP_PIXELS_RUNS);
    free(pixels);

    /* A zero pixel is real data. Control low bits encode length minus1,
     * so control0 is a one-pixel literal, never a zero-length packet. */
    const uint8_t zero_pixels[]={0,0,0,0,0x0E,0x80,0,0};
    uint16_t zeros[16];
    memset(zeros,0xFF,sizeof zeros);
    CHECK(mmx5_coop_pixels_decode(zero_pixels,sizeof zero_pixels,16,MMX5_COOP_PIXELS_RUNS,zeros));
    for(unsigned i=0;i<16;++i)CHECK(!zeros[i]);
    roundtrip(zeros,16,MMX5_COOP_PIXELS_RUNS);
    const uint8_t overflow[]={0xFF,0xFF,0x34,0x12};
    const uint8_t truncated_literal[]={0x06,0x00,0x34,0x12};
    const uint8_t underfilled[]={0x82,0x80,0x34,0x12}; /* 131 exceeds16 */
    const uint8_t short_run[]={0x02,0x80,0x34,0x12}; /* only3 of16 */
    const uint8_t trailing[]={0x0F,0x80,0x34,0x12,0x00};
    rejected(overflow,sizeof overflow,16,MMX5_COOP_PIXELS_RUNS);
    rejected(truncated_literal,sizeof truncated_literal,16,MMX5_COOP_PIXELS_RUNS);
    rejected(underfilled,sizeof underfilled,16,MMX5_COOP_PIXELS_RUNS);
    rejected(short_run,sizeof short_run,16,MMX5_COOP_PIXELS_RUNS);
    rejected(trailing,sizeof trailing,16,MMX5_COOP_PIXELS_RUNS);
    rejected(short_run,3,16,MMX5_COOP_PIXELS_RUNS);
    rejected(short_run,1,16,MMX5_COOP_PIXELS_RUNS);
    rejected(NULL,0,16,MMX5_COOP_PIXELS_RUNS);
    rejected(NULL,4,16,MMX5_COOP_PIXELS_RUNS);
    rejected(short_run,4,16,2);
    rejected(short_run,4,16,MMX5_COOP_PIXELS_RAW);
    rejected(NULL,0,1,MMX5_COOP_PIXELS_RAW);
    rejected(short_run,4,MMX5_COOP_PIXELS_MAX_COUNT+1u,MMX5_COOP_PIXELS_RUNS);
    uint8_t untouched[4]={1,2,3,4};
    CHECK(!mmx5_coop_pixels_encode(untouched,3,endian_pixels,2,MMX5_COOP_PIXELS_RAW));
    CHECK(!memcmp(untouched,"\x01\x02\x03\x04",4));
    if(argc==2)actual_corpus(argv[1]);else CHECK(argc==1);
    puts("X5 pixel codec lossless/endian/raw/run limits/corruption checks passed");return 0;
}
