#include "mmx5_coop_pixel_codec.h"

static uint16_t read16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8);
}
static void write16(uint8_t *p,uint16_t value) {
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
}
static uint32_t repeated(const uint16_t *p,uint32_t at,uint32_t count) {
    uint32_t n=1;
    while(n<32768u && n<count-at && p[at+n]==p[at])++n;
    return n;
}
static uint32_t literal_end(const uint16_t *p,uint32_t at,uint32_t count) {
    uint32_t start=at++;
    while(at<count && at-start<32768u) {
        if(count-at>=3u && p[at]==p[at+1u] && p[at]==p[at+2u])break;
        ++at;
    }
    return at;
}
static uint32_t packet_bytes(const uint16_t *p,uint32_t count) {
    uint32_t bytes=0;
    for(uint32_t at=0;at<count;) {
        uint32_t run=repeated(p,at,count);
        if(run>=3u) {bytes+=4u;at+=run;}
        else {uint32_t end=literal_end(p,at,count);bytes+=2u+2u*(end-at);at=end;}
    }
    return bytes;
}
int mmx5_coop_pixels_measure(const uint16_t *pixels,uint32_t count,
                             uint8_t *encoding,uint32_t *bytes) {
    if(!encoding || !bytes || count>MMX5_COOP_PIXELS_MAX_COUNT || (count && !pixels))return 0;
    uint32_t raw=count*2u,runs=packet_bytes(pixels,count);
    *encoding=(uint8_t)(runs<raw?MMX5_COOP_PIXELS_RUNS:MMX5_COOP_PIXELS_RAW);
    *bytes=runs<raw?runs:raw;return 1;
}
int mmx5_coop_pixels_encode(uint8_t *out,uint32_t bytes,
                            const uint16_t *pixels,uint32_t count,uint8_t encoding) {
    if(count>MMX5_COOP_PIXELS_MAX_COUNT || (count && !pixels) || (bytes && !out))return 0;
    if(encoding==MMX5_COOP_PIXELS_RAW) {
        if(bytes!=count*2u)return 0;
        for(uint32_t i=0;i<count;++i)write16(out+2u*i,pixels[i]);
        return 1;
    }
    if(encoding!=MMX5_COOP_PIXELS_RUNS || !count || bytes>=count*2u ||
       bytes!=packet_bytes(pixels,count))return 0;
    uint32_t pos=0;
    for(uint32_t at=0;at<count;) {
        uint32_t run=repeated(pixels,at,count);
        if(run>=3u) {
            write16(out+pos,(uint16_t)(0x8000u|(run-1u)));
            write16(out+pos+2u,pixels[at]);pos+=4u;at+=run;
        } else {
            uint32_t end=literal_end(pixels,at,count);
            write16(out+pos,(uint16_t)(end-at-1u));pos+=2u;
            while(at<end) {write16(out+pos,pixels[at++]);pos+=2u;}
        }
    }
    return pos==bytes;
}
static int packets_valid(const uint8_t *in,uint32_t bytes,uint32_t count) {
    uint32_t pos=0,decoded=0;
    while(pos<bytes) {
        if(bytes-pos<2u)return 0;
        uint16_t control=read16(in+pos);pos+=2u;
        uint32_t n=(control&0x7FFFu)+1u;
        if(n>count-decoded)return 0;
        uint32_t payload=(control&0x8000u)?2u:n*2u;
        if(payload>bytes-pos)return 0;
        pos+=payload;decoded+=n;
    }
    return decoded==count;
}
int mmx5_coop_pixels_decode(const uint8_t *in,uint32_t bytes,uint32_t count,
                            uint8_t encoding,uint16_t *out) {
    if(count>MMX5_COOP_PIXELS_MAX_COUNT || (bytes && !in) || bytes>count*2u)return 0;
    if(encoding==MMX5_COOP_PIXELS_RAW) {
        if(bytes!=count*2u)return 0;
        if(out)for(uint32_t i=0;i<count;++i)out[i]=read16(in+2u*i);
        return 1;
    }
    if(encoding!=MMX5_COOP_PIXELS_RUNS || !count || !bytes || bytes>=count*2u ||
       !packets_valid(in,bytes,count))return 0;
    if(!out)return 1;
    uint32_t pos=0,decoded=0;
    while(pos<bytes) {
        uint16_t control=read16(in+pos);pos+=2u;
        uint32_t n=(control&0x7FFFu)+1u;
        if(control&0x8000u) {
            uint16_t value=read16(in+pos);pos+=2u;
            for(uint32_t i=0;i<n;++i)out[decoded++]=value;
        } else for(uint32_t i=0;i<n;++i) {out[decoded++]=read16(in+pos);pos+=2u;}
    }
    return 1;
}
