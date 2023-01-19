/*
 * SipHash reference C implementation
 *
 * Copyright (c) 2012-2021 Jean-Philippe Aumasson
 * <jeanphilippe.aumasson@gmail.com>
 * Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
 * 
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 * 
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include "common.h"
#include "siphash.h"

#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x, b) (u64)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)                                              \
    (p)[0] = (u8)((v));                                              \
    (p)[1] = (u8)((v) >> 8);                                         \
    (p)[2] = (u8)((v) >> 16);                                        \
    (p)[3] = (u8)((v) >> 24);

#define U64TO8_LE(p, v)                                              \
    U32TO8_LE((p), (u32)((v)));                                      \
    U32TO8_LE((p) + 4, (u32)((v) >> 32));

#define U8TO64_LE(p)                                                 \
    (((u64)((p)[0])) | ((u64)((p)[1]) << 8) |                        \
     ((u64)((p)[2]) << 16) | ((u64)((p)[3]) << 24) |                 \
     ((u64)((p)[4]) << 32) | ((u64)((p)[5]) << 40) |                 \
     ((u64)((p)[6]) << 48) | ((u64)((p)[7]) << 56))

#define SIPROUND                                                     \
    do {                                                             \
        v0 += v1;                                                    \
        v1 = ROTL(v1, 13);                                           \
        v1 ^= v0;                                                    \
        v0 = ROTL(v0, 32);                                           \
        v2 += v3;                                                    \
        v3 = ROTL(v3, 16);                                           \
        v3 ^= v2;                                                    \
        v0 += v3;                                                    \
        v3 = ROTL(v3, 21);                                           \
        v3 ^= v0;                                                    \
        v2 += v1;                                                    \
        v1 = ROTL(v1, 17);                                           \
        v1 ^= v2;                                                    \
        v2 = ROTL(v2, 32);                                           \
    } while (0)

static u64 _siphash(u8 const *in, size_t const in_len, u8 const *key) {
    // initialization, 16-byte key k (k0, k1) and 32-byte state v (v0 to v3)
    u64 k0 = U8TO64_LE(key);
    u64 k1 = U8TO64_LE(key + 8);

    u64 v0 = UINT64_C(0x736f6d6570736575) ^ k0;
    u64 v1 = UINT64_C(0x646f72616e646f6d) ^ k1;
    u64 v2 = UINT64_C(0x6c7967656e657261) ^ k0;
    u64 v3 = UINT64_C(0x7465646279746573) ^ k1;

    u8 const *end = in + in_len - (in_len % sizeof(u64));

    for (; in != end; in += 8) {
        u64 msg = U8TO64_LE(in);
        v3 ^= msg;

        for (u32 i=1; i <= cROUNDS; ++i) {
            SIPROUND;
        }

        v0 ^= msg;
    }

    i32 const left = in_len & 7;
    u64 b = ((u64)in_len) << 56;

    switch (left) {
    case 7:
        b |= ((u64)in[6]) << 48;
    case 6:
        b |= ((u64)in[5]) << 40;
    case 5:
        b |= ((u64)in[4]) << 32;
    case 4:
        b |= ((u64)in[3]) << 24;
    case 3:
        b |= ((u64)in[2]) << 16;
    case 2:
        b |= ((u64)in[1]) << 8;
    case 1:
        b |= ((u64)in[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;

    for (u32 i=1; i <= cROUNDS; ++i) {
        SIPROUND;
    }

    v0 ^= b;
    v2 ^= 0xff;

    for (u32 j=1; j <= dROUNDS; ++j) {
        SIPROUND;
    }

    b = v0 ^ v1 ^ v2 ^ v3;

    u64 out = 0;
    // 64-bit hash value `b` to 8-byte little-endian representation `out`
    U64TO8_LE((u8 *)&out, b);

    return out;
}

u64 siphash(void const *data, size_t data_len, u8 const key[16]) {
    return _siphash((u8 *)data, data_len, key);
}
