#include "wmania_rope_spawn.h"

#define E(img,flip,horz,side,x,y,z) \
    { (img), true, (flip), (horz), (side), (x), (y), (z) }
#define N { 0, false, false, false, false, 0, 0, 0 }

/*
 * Direct translation of #front_ptable, #back_ptable, #left_ptable and
 * #right_ptable.  Channel order mirrors rope PDATA:
 * red, white, blue, shadow.
 */
const WmRopeBankSeed wm_rope_bank_seeds[4] = {
    {
        WM_ROPE_FRONT, true, false,
        {
            {
                E("ROPE_F_R",false,true,false, 674,400,0x15aa),
                E("ROPE_F_R",true, true,false,1268,400,0x15aa)
            },
            {
                E("ROPE_F_W",false,true,false, 672,424,0x15aa),
                E("ROPE_F_W",true, true,false,1268,424,0x15aa)
            },
            {
                E("ROPE_F_B",false,true,false, 673,447,0x15aa),
                E("ROPE_F_B",true, true,false,1271,447,0x15aa)
            },
            { N, N }
        }
    },
    {
        WM_ROPE_BACK, true, false,
        {
            {
                E("ROPE_B_R",false,true,false, 750,316,0x13ca),
                E("ROPE_B_R",true, true,false,1191,316,0x13ca)
            },
            {
                E("ROPE_B_W",false,true,false, 750,339,0x13ca),
                E("ROPE_B_W",true, true,false,1191,339,0x13ca)
            },
            {
                E("ROPE_B_B",false,true,false, 750,362,0x13ca),
                E("ROPE_B_B",true, true,false,1191,362,0x13ca)
            },
            { N, N }
        }
    },
    {
        WM_ROPE_LEFT, false, true,
        {
            {
                E("ROPE_S_Ra",false,false,true,677,321,0x13cc),
                E("ROPE_S_Rb",false,false,true,677,321,0x13cc)
            },
            {
                E("ROPE_S_Wa",false,false,true,677,344,0x13cb),
                E("ROPE_S_Wb",false,false,true,677,344,0x13cb)
            },
            {
                E("ROPE_S_Ba",false,false,true,677,367,0x13ca),
                E("ROPE_S_Bb",false,false,true,677,367,0x13ca)
            },
            {
                E("ROPSHADA",false,false,true,0x2c7,0x189,0x13c8),
                E("ROPSHADB",false,false,true,0x2c7,0x189,0x13c8)
            }
        }
    },
    {
        WM_ROPE_RIGHT, false, true,
        {
            {
                E("ROPE_S_Ra",true,false,true,1265,320,0x13cc),
                E("ROPE_S_Rb",true,false,true,1265,320,0x13cc)
            },
            {
                E("ROPE_S_Wa",true,false,true,1265,343,0x13cb),
                E("ROPE_S_Wb",true,false,true,1265,343,0x13cb)
            },
            {
                E("ROPE_S_Ba",true,false,true,1265,366,0x13ca),
                E("ROPE_S_Bb",true,false,true,1265,366,0x13ca)
            },
            {
                E("ROPSHADA",true,false,true,0x469 + 100,0x189,0x13c8),
                E("ROPSHADB",true,false,true,0x469 + 100,0x189,0x13c8)
            }
        }
    }
};

#undef E
#undef N

const WmRopeBankSeed *wm_rope_bank_seed(WmRopeBank bank)
{
    if ((unsigned)bank > WM_ROPE_RIGHT) {
        return 0;
    }
    return &wm_rope_bank_seeds[(unsigned)bank];
}

const WmRopeObjectSeed *wm_rope_object_seed(
    WmRopeBank bank,
    WmRopeChannel channel,
    WmRopeHalf half)
{
    const WmRopeBankSeed *b = wm_rope_bank_seed(bank);

    if (b == 0 ||
        (unsigned)channel >= WM_ROPE_CHANNEL_COUNT ||
        (unsigned)half > WM_ROPE_HALF_SECOND) {
        return 0;
    }

    return &b->object[(unsigned)channel][(unsigned)half];
}

int32_t wm_rope_spawn_x_fp16(const WmRopeObjectSeed *seed)
{
    if (seed == 0 || !seed->exists) {
        return 0;
    }
    return ((int32_t)seed->raw_x + 104) << 16;
}

int32_t wm_rope_spawn_y_fp16(const WmRopeObjectSeed *seed)
{
    if (seed == 0 || !seed->exists) {
        return 0;
    }
    return ((int32_t)seed->raw_y - 258) << 16;
}

bool wm_rope_process_survives_reduce_bog(
    WmRopeBank bank,
    bool reduce_bog)
{
    if (!reduce_bog) {
        return true;
    }

    return bank == WM_ROPE_LEFT || bank == WM_ROPE_RIGHT;
}
