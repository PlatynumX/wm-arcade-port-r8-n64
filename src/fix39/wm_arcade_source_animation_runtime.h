#ifndef WM_ARCADE_SOURCE_ANIMATION_RUNTIME_H
#define WM_ARCADE_SOURCE_ANIMATION_RUNTIME_H
#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#include "wm_arcade_source_animation_program.h"
#include "wm_arcade_react.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_source_anim_runtime wm_source_anim_runtime_t;

/* R37N8: Midway frame application copies geometry fields into live object
   state when a frame changes. Keep that copied state distinct from the
   display-frame token so collision never has to reinterpret a later name. */
typedef struct wm_source_anim_frame_geometry {
    const char *source_frame;
    uint16_t width;
    uint16_t height;
    int16_t xani;
    int16_t yani;
    int16_t iani3x;
    int16_t iani3y;
    int16_t iani3z;
    int16_t iani3id;
    uint8_t valid;
} wm_source_anim_frame_geometry_t;

typedef struct wm_source_anim_services {
    uint16_t (*round_tick)(void *user);
    uint32_t (*pcnt)(void *user);
    uint32_t (*rndrng0)(uint32_t max_inclusive, void *user);
    int (*rndper_hi)(uint16_t probability, void *user);
    void (*sound)(wm_arcade_actor_t *a, const char *token, int32_t raw, void *user);
    void (*raw_sound)(wm_arcade_actor_t *a, const char *token, int32_t raw, void *user);
    void (*code)(wm_arcade_actor_t *a, const char *label, void *user);
    void (*shake)(wm_arcade_actor_t *a, int kind, int value, void *user);
    void (*rope)(wm_arcade_actor_t *a, int bank, int action, int value, void *user);
    void (*set_rope_z)(wm_arcade_actor_t *a, int rope_index, int action, void *user);
    void (*debris)(wm_arcade_actor_t *origin, const wm_source_anim_ins_t *ins, void *user);
    void (*create_proc)(wm_arcade_actor_t *a, const wm_source_anim_ins_t *ins, void *user);
    void (*shadowtrail)(wm_arcade_actor_t *a, const wm_source_anim_ins_t *ins, void *user);
    void (*draw_name)(wm_arcade_actor_t *a, int id, void *user);
    void (*scroll_ctrl)(wm_arcade_actor_t *a, int value, void *user);
    void (*attach_image)(wm_arcade_actor_t *a, const char *image, int x, int y, int z, void *user);
    void (*add_move)(wm_arcade_actor_t *a, int move, int p1, int p2, void *user);
    void (*set_allow_offscreen)(int ticks, void *user);
    bool (*change_other_anim)(wm_arcade_actor_t *a, wm_arcade_actor_t *other, const char *label, void *user);
    bool (*force_other_frame)(wm_arcade_actor_t *a, wm_arcade_actor_t *other, const char *frame, void *user);
    /* R37N7: exact logical geometry represented by the pre-LOAD2 WIMP frame.
       These are the outputs reconstructed by Midway get_mpart_offsets +
       get_mpart_xsize after the logical image is split into DMA pieces. */
    bool (*multipart_geometry)(wm_arcade_actor_t *a, const char *frame,
                               int16_t *xani, int16_t *yani, uint16_t *xsize,
                               void *user);
    /* R37N8: full converted WIMP metadata copied at frame-application time. */
    bool (*frame_geometry)(wm_arcade_actor_t *a, const char *frame,
                           wm_source_anim_frame_geometry_t *out, void *user);
    int (*do_roll)(wm_arcade_actor_t *a, void *user);
    int (*buttons_down)(wm_arcade_actor_t *a, void *user);
    wm_arcade_combat_runtime_t *combat_runtime;
    const wm_arcade_react_callbacks_t *react;
    void *user;
} wm_source_anim_services_t;

struct wm_source_anim_runtime {
    const wm_source_anim_program_t *program;
    uint16_t pc;
    const char *current_frame;
    wm_source_anim_frame_geometry_t frame_geometry;
    const wm_source_anim_services_t *services;
    uint32_t instructions_executed;
    int fault;
    /* Combat2CZ: ANIM.ASM has independent primary/secondary state. */
    uint16_t mode_shadow;
    int32_t count_shadow;
    uint8_t secondary;
};

void wm_source_anim_runtime_init(wm_source_anim_runtime_t *s);
void wm_source_anim_runtime_bind(wm_source_anim_runtime_t *s,const wm_source_anim_services_t *services);
void wm_source_anim_runtime_set_secondary(wm_source_anim_runtime_t *s,bool secondary);
bool wm_source_anim_runtime_change(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a,
                                   uint8_t roster_id, const char *label);
void wm_source_anim_runtime_tick(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a);
void wm_source_anim_runtime_force_frame(wm_source_anim_runtime_t *s,const char *frame);
bool wm_source_anim_runtime_force_frame_actor(wm_source_anim_runtime_t *s,
                                               wm_arcade_actor_t *a,
                                               const char *frame);
bool wm_source_anim_runtime_copy_geometry_frame(wm_source_anim_runtime_t *s,
                                                 wm_arcade_actor_t *a,
                                                 const char *geometry_frame);
const char *wm_source_anim_runtime_frame(const wm_source_anim_runtime_t *s);
const wm_source_anim_frame_geometry_t *wm_source_anim_runtime_geometry(const wm_source_anim_runtime_t *s);
const char *wm_source_anim_runtime_label(const wm_source_anim_runtime_t *s);
#ifdef __cplusplus
}
#endif
#endif
