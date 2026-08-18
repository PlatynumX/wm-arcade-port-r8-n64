#ifndef WM_INPUT_H
#define WM_INPUT_H
#include <stdbool.h>
#include <stdint.h>

/* Portable gameplay input. The N64 frontend maps the user's requested layout:
   A=run, C-left=light punch, C-up=power punch, C-right=light kick,
   C-down=power kick, R=block. Menu code reuses A/Start as confirm. */
typedef struct {
    int8_t stick_x;
    int8_t stick_y;

    bool run;          /* held */
    bool light_punch;  /* edge */
    bool power_punch;  /* edge */
    bool light_kick;   /* edge */
    bool power_kick;   /* edge */
    bool block;        /* held */

    bool start;        /* edge: pause/menu */
    bool l;            /* edge: AI/debug utility */
    bool z;            /* edge: debug HUD utility */
    bool b;            /* reserved; not required for gameplay */
} wm_input_state;

#endif
