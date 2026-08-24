#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1]); p=repo/'src/fix39/wm_fix39_runtime.c'; t=p.read_text()
start=t.find('static void drone_seek_dir_dist(')
if start<0: raise SystemExit('Combat2BL: drone_seek_dir_dist not found')
nextfn=t.find('\nstatic int drone_script_seek(',start)
if nextfn<0: raise SystemExit('Combat2BL: drone_script_seek boundary not found')
new=r'''/* Combat2BL: direct translation of DRONE.ASM::drone_seekdirdist,
   #drn_getxz and drone_seekxz. DRN_SEEKDIR is a 16-way angular offset
   around the opponent; DRN_SEEKDIST selects the source 50-pixel radius
   bands. It is NOT a "walk toward opponent / reverse when hanging back"
   boolean. */
static const int16_t drone_seek_sine_t[6][20] = {
    {-50,-46,-35,-19,0,19,35,46,50,46,35,19,0,-19,-35,-46,-50,-46,-35,-19},
    {-100,-92,-71,-38,0,38,71,92,100,92,71,38,0,-38,-71,-92,-100,-92,-71,-38},
    {-150,-139,-106,-57,0,57,106,139,150,139,106,57,0,-57,-106,-139,-150,-139,-106,-57},
    {-200,-185,-141,-76,0,76,141,185,200,185,141,76,0,-76,-141,-185,-200,-185,-141,-76},
    {-250,-231,-177,-95,0,95,177,231,250,231,177,95,0,-95,-177,-231,-250,-231,-177,-95},
    {-300,-277,-212,-115,0,114,212,277,300,277,212,114,0,-114,-212,-277,-300,-277,-212,-115}
};

static bool drone_seek_source_target(const wm_arcade_actor_t *opp,
                                     int seek_dir, int seek_dist,
                                     int32_t *tx, int32_t *tz)
{
    int d = seek_dist;
    int a = seek_dir & 15;
    int32_t x, z;
    if (!opp || !tx || !tz) return false;
    if (d < 0) d = 0;
    if (d > 5) d = 5;
    /* DRONE.ASM #drn_getxz: Z uses sine_t[a], X uses sine_t[a+4]. */
    z = opp->z_int + drone_seek_sine_t[d][a];
    x = opp->x_int + drone_seek_sine_t[d][a + 4];
    /* Exact #drn_getxz ring guards: RING_X_CENTER +/- 220, RING_TOP/BOT. */
    if (x < (1074 - 220) || x > (1074 + 220) || z < 1023 || z > 1345)
        return false;
    *tx = x; *tz = z; return true;
}

static uint16_t drone_seek_source_joy(const wm_arcade_actor_t *actor,
                                      int32_t tx, int32_t tz, int32_t range)
{
    int32_t dx, dz;
    uint16_t joy = WM_MOVE_ZIP;
    if (!actor) return joy;
    dx = actor->x_int - tx;
    dz = actor->z_int - tz;
    if (dx < -range) joy |= WM_MOVE_RIGHT;
    else if (dx > range) joy |= WM_MOVE_LEFT;
    if (dz < -range) joy |= WM_MOVE_DOWN;
    else if (dz > range) joy |= WM_MOVE_UP;
    return joy;
}

static void drone_seek_dir_dist(wm_arcade_actor_t *actor, wm_arcade_drone_state_t *d, void *user)
{
    wm_arcade_actor_t *opp;
    int dir, dist;
    int32_t tx=0, tz=0;
    bool valid=false;
    uint16_t oldjoy;
    if (!actor || !d) return;
    opp = actor->smart_target;
    if (!opp) { d->joy = WM_MOVE_ZIP; return; }
    oldjoy = d->joy;
    dir = d->seek_dir & 15;
    dist = d->seek_dist;

    /* DRONE.ASM tries the requested direction first, then walks outward
       +1/-1 for seven pairs until #drn_getxz returns an in-ring target. */
    valid = drone_seek_source_target(opp, dir, dist, &tx, &tz);
    if (!valid) {
        int plus=dir, minus=dir;
        for (int n=0; n<7 && !valid; ++n) {
            plus=(plus+1)&15;
            if (drone_seek_source_target(opp, plus, dist, &tx, &tz)) { dir=plus; valid=true; break; }
            minus=(minus-1)&15;
            if (drone_seek_source_target(opp, minus, dist, &tx, &tz)) { dir=minus; valid=true; break; }
        }
    }
    if (!valid) { d->joy = WM_MOVE_ZIP; return; }
    d->seek_dir = dir;
    d->joy = drone_seek_source_joy(actor, tx, tz, 30);

    /* At the seek point, modes -2/-3 choose a new +/-2/3 angular offset.
       Source restores old joy for this tick to reduce the direction glitch. */
    if (d->joy == WM_MOVE_ZIP && d->mode < -1) {
        uint32_t r = drone_rnd_upto(3u, user);
        int delta = (r==0u) ? -2 : (r==1u) ? -3 : (r==2u) ? 2 : 3;
        d->joy = oldjoy;
        d->seek_dir = (d->seek_dir + delta) & 15;
    }
}
'''
t=t[:start]+new+t[nextfn:]
p.write_text(t)
print('Combat2BL exact DRONE.ASM drone_seekdirdist translation applied')
