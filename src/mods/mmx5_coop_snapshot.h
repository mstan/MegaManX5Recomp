#ifndef MMX5_COOP_SNAPSHOT_H
#define MMX5_COOP_SNAPSHOT_H
#include <stdint.h>
#include <string.h>

/* Explicit little-endian wire fields; host layout/pointers never enter a state. */
enum { MMX5_SNAPSHOT_COUNT, MMX5_SNAPSHOT_WRITE, MMX5_SNAPSHOT_VALIDATE, MMX5_SNAPSHOT_READ };
typedef struct {
    uint8_t *out;
    const uint8_t *in;
    uint32_t len,pos;
    unsigned mode;
    int ok;
} Mmx5SnapshotIO;
static inline uint32_t mmx5_snapshot_integer(Mmx5SnapshotIO *s,uint32_t value,unsigned bytes,uint32_t maximum) {
    if(!s->ok || !bytes || bytes>4 || bytes>UINT32_MAX-s->pos ||
       (s->mode!=MMX5_SNAPSHOT_COUNT && (s->pos>s->len || bytes>s->len-s->pos))) {
        s->ok=0;return 0;
    }
    if(s->mode==MMX5_SNAPSHOT_WRITE) {
        for(unsigned i=0;i<bytes;++i)s->out[s->pos+i]=(uint8_t)(value>>(i*8));
    } else if(s->mode==MMX5_SNAPSHOT_VALIDATE || s->mode==MMX5_SNAPSHOT_READ) {
        value=0;for(unsigned i=0;i<bytes;++i)value|=(uint32_t)s->in[s->pos+i]<<(i*8);
    }
    s->pos+=bytes;
    if(value>maximum)s->ok=0;
    return value;
}
static inline void mmx5_snapshot_bytes(Mmx5SnapshotIO *s,void *bytes,uint32_t count) {
    if(!s->ok || count>UINT32_MAX-s->pos ||
       (s->mode!=MMX5_SNAPSHOT_COUNT && (s->pos>s->len || count>s->len-s->pos))) {s->ok=0;return;}
    if(s->mode==MMX5_SNAPSHOT_WRITE)memcpy(s->out+s->pos,bytes,count);
    else if(s->mode==MMX5_SNAPSHOT_READ)memcpy(bytes,s->in+s->pos,count);
    s->pos+=count;
}
static inline void mmx5_snapshot_fixed(Mmx5SnapshotIO *s,uint32_t expected) {
    if(mmx5_snapshot_integer(s,expected,4,UINT32_MAX)!=expected)s->ok=0;
}
static inline void mmx5_snapshot_preference(Mmx5SnapshotIO *s,uint32_t current) {
    (void)mmx5_snapshot_integer(s,current,1,1);
}
#define MMX5_SNAPSHOT_FIELD(s,field,bytes,max) do { \
    uint32_t snapshot_value=mmx5_snapshot_integer((s),(uint32_t)(field),(bytes),(max)); \
    if((s)->mode==MMX5_SNAPSHOT_READ)(field)=snapshot_value; \
} while(0)
#define MMX5_SNAPSHOT_U32(s,field) MMX5_SNAPSHOT_FIELD(s,field,4,UINT32_MAX)
#define MMX5_SNAPSHOT_U16(s,field) do { \
    uint32_t snapshot_value=mmx5_snapshot_integer((s),(uint16_t)(field),2,UINT16_MAX); \
    if((s)->mode==MMX5_SNAPSHOT_READ)(field)=snapshot_value; \
} while(0)
#define MMX5_SNAPSHOT_U8(s,field) MMX5_SNAPSHOT_FIELD(s,field,1,UINT8_MAX)
#define MMX5_SNAPSHOT_BOOL(s,field) MMX5_SNAPSHOT_FIELD(s,field,1,1)
#endif
