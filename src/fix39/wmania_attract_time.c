#include "wmania_attract_time.h"

#include <stdio.h>
#include <string.h>

static const char *const weekdays[7] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
    "THURSDAY", "FRIDAY", "SATURDAY"
};

static const char *const months[12] = {
    "JANUARY", "FEBRUARY", "MARCH", "APRIL",
    "MAY", "JUNE", "JULY", "AUGUST",
    "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

bool wm_attract_format_clock(
    const WmAttractClock *clock,
    WmAttractClockText *out)
{
    int dow, month, day, year, hour, minute, hour12;
    int n1, n2;

    if (clock == 0 || out == 0) {
        return false;
    }

    dow = clock->day_of_week;
    if (dow < 1 || dow >= 8) {
        dow = 1;
    }

    month = clock->month;
    if (month < 1 || month >= 13) {
        month = 1;
    }

    day = clock->day;
    if (day < 1 || day >= 32) {
        day = 1;
    }

    year = clock->year_2digit;
    if (year < 0 || year >= 99) {
        /*
         * Exact assembly uses CMPI 99/JRLT; 99 itself falls into bad_year.
         */
        year = 0;
    }

    hour = clock->hour_24;
    if (hour < 0 || hour >= 24) {
        hour = 0;
    }

    minute = clock->minute;
    if (minute < 0 || minute >= 60) {
        minute = 0;
    }

    hour12 = hour % 12;
    if (hour12 == 0) {
        hour12 = 12;
    }

    n1 = snprintf(out->weekday, sizeof(out->weekday), "%s",
                  weekdays[dow - 1]);
    n2 = snprintf(out->date, sizeof(out->date), "%s %d, 19%02d",
                  months[month - 1], day, year);
    if (n1 < 0 || n2 < 0 ||
        (size_t)n1 >= sizeof(out->weekday) ||
        (size_t)n2 >= sizeof(out->date)) {
        return false;
    }

    n1 = snprintf(out->time, sizeof(out->time), "%d:%02d",
                  hour12, minute);
    return n1 >= 0 && (size_t)n1 < sizeof(out->time);
}
