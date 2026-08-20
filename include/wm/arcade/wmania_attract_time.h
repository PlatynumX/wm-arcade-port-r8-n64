#ifndef WMANIA_ATTRACT_TIME_H
#define WMANIA_ATTRACT_TIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int day_of_week; /* source: 1..7; invalid -> Sunday */
    int month;       /* source: 1..12; invalid -> January */
    int day;         /* source: 1..31; invalid -> 1 */
    int year_2digit; /* source: 0..99; invalid -> 0 */
    int hour_24;     /* source: 0..23; invalid -> 0 */
    int minute;      /* source: 0..59; invalid -> 0 */
} WmAttractClock;

typedef struct {
    char weekday[16];
    char date[32];
    char time[16];
} WmAttractClockText;

/* Source show_time_date validation and 12-hour conversion. */
bool wm_attract_format_clock(
    const WmAttractClock *clock,
    WmAttractClockText *out);

/* Exact source layout/timing. */
#define WM_ATTRACT_TIME_DATE_MIN_PRE_TICKS 30
#define WM_ATTRACT_TIME_DATE_PROMPT_Y 40
#define WM_ATTRACT_TIME_DATE_WEEKDAY_Y 72
#define WM_ATTRACT_TIME_DATE_DATE_Y 92
#define WM_ATTRACT_TIME_DATE_TIME_Y 112
#define WM_ATTRACT_TIME_DATE_PLAY_PROMPT_Y 150
#define WM_ATTRACT_TIME_DATE_WRESTLEMANIA_Y 180
#define WM_ATTRACT_TIME_DATE_MIN_DISPLAY_TICKS 25
#define WM_ATTRACT_TIME_DATE_WAIT_TSEC 5

#ifdef __cplusplus
}
#endif

#endif
