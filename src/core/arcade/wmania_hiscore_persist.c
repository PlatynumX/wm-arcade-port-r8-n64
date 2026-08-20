#include "wm/arcade/wmania_hiscore_persist.h"

#include <stdbool.h>
#include <string.h>

#define MAGIC0 'W'
#define MAGIC1 'H'
#define MAGIC2 'S'
#define MAGIC3 '1'

static void put16(uint8_t **p, uint16_t v)
{
    *(*p)++ = (uint8_t)(v >> 8);
    *(*p)++ = (uint8_t)v;
}

static uint16_t get16(const uint8_t **p)
{
    uint16_t v = (uint16_t)((uint16_t)(*p)[0] << 8) | (*p)[1];
    *p += 2;
    return v;
}

static void put32(uint8_t **p, uint32_t v)
{
    *(*p)++ = (uint8_t)(v >> 24);
    *(*p)++ = (uint8_t)(v >> 16);
    *(*p)++ = (uint8_t)(v >> 8);
    *(*p)++ = (uint8_t)v;
}

static uint32_t get32(const uint8_t **p)
{
    uint32_t v =
        ((uint32_t)(*p)[0] << 24) |
        ((uint32_t)(*p)[1] << 16) |
        ((uint32_t)(*p)[2] << 8) |
        (uint32_t)(*p)[3];
    *p += 4;
    return v;
}

static uint32_t crc32_calc(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xffffffffu;
    size_t i;
    unsigned bit;

    for (i = 0; i < size; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8u; ++bit) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

static size_t table_bytes(void)
{
    return
        (WM_HS_STREAK_LAST_ENTRY + 1u) * sizeof(WmHsEntry) +
        (WM_HS_PIN_SPEED_LAST_ENTRY + 1u) * sizeof(WmHsEntry) +
        (WM_HS_BEATEN_LAST_ENTRY + 1u) * sizeof(WmHsEntry) +
        (WM_HS_INTER_LAST_ENTRY + 1u) * sizeof(WmHsEntry) +
        (WM_HS_TAG_LAST_ENTRY + 1u) * sizeof(WmHsEntry);
}

size_t wm_hs_save_encoded_size(void)
{
    /*
     * magic 4 + version 2 + payload_size 2 +
     * counter/value+verifier 8 + recent 10 + tables 1100 + crc32 4
     */
    return 4u + 2u + 2u + 8u +
           (WM_HS_TABLE_COUNT * 2u) + table_bytes() + 4u;
}

static void encode_entries(
    uint8_t **p, const WmHsEntry *entries, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        memcpy(*p, &entries[i], sizeof(WmHsEntry));
        *p += sizeof(WmHsEntry);
    }
}

static void decode_entries(
    const uint8_t **p, WmHsEntry *entries, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        memcpy(&entries[i], *p, sizeof(WmHsEntry));
        *p += sizeof(WmHsEntry);
    }
}

bool wm_hs_save_encode(
    const WmHsSystem *system,
    uint8_t *buffer,
    size_t capacity,
    size_t *written)
{
    uint8_t *p = buffer;
    size_t need = wm_hs_save_encoded_size();
    uint32_t crc;
    unsigned i;

    if (system == 0 || buffer == 0 || capacity < need) {
        return false;
    }

    *p++ = MAGIC0; *p++ = MAGIC1; *p++ = MAGIC2; *p++ = MAGIC3;
    put16(&p, WM_HS_SAVE_VERSION);
    put16(&p, (uint16_t)(need - 12u)); /* payload excluding 8-byte hdr+crc */
    put32(&p, system->reset_counter.value);
    put32(&p, system->reset_counter.verifier);

    for (i = 0; i < WM_HS_TABLE_COUNT; ++i) {
        put16(&p, system->recent_index[i]);
    }

    encode_entries(&p, system->streak, WM_HS_STREAK_LAST_ENTRY + 1u);
    encode_entries(&p, system->pin_speed, WM_HS_PIN_SPEED_LAST_ENTRY + 1u);
    encode_entries(&p, system->beaten, WM_HS_BEATEN_LAST_ENTRY + 1u);
    encode_entries(&p, system->inter, WM_HS_INTER_LAST_ENTRY + 1u);
    encode_entries(&p, system->tag, WM_HS_TAG_LAST_ENTRY + 1u);

    crc = crc32_calc(buffer, (size_t)(p - buffer));
    put32(&p, crc);

    if (written != 0) {
        *written = (size_t)(p - buffer);
    }

    return (size_t)(p - buffer) == need;
}

WmHsLoadResult wm_hs_save_decode(
    WmHsSystem *system,
    const uint8_t *buffer,
    size_t size,
    uint32_t adjusted_reset_value)
{
    const uint8_t *p = buffer;
    const uint8_t *crc_pos;
    uint32_t stored_crc;
    uint32_t actual_crc;
    uint16_t version;
    uint16_t payload;
    unsigned i;
    bool repaired = false;

    if (system == 0 || buffer == 0 ||
        size != wm_hs_save_encoded_size()) {
        wm_hs_system_init(system, adjusted_reset_value);
        return WM_HS_LOAD_RESET;
    }

    if (p[0] != MAGIC0 || p[1] != MAGIC1 ||
        p[2] != MAGIC2 || p[3] != MAGIC3) {
        wm_hs_system_init(system, adjusted_reset_value);
        return WM_HS_LOAD_RESET;
    }
    p += 4;

    version = get16(&p);
    payload = get16(&p);
    if (version != WM_HS_SAVE_VERSION ||
        payload != (uint16_t)(size - 12u)) {
        wm_hs_system_init(system, adjusted_reset_value);
        return WM_HS_LOAD_RESET;
    }

    crc_pos = buffer + size - 4u;
    {
        const uint8_t *q = crc_pos;
        stored_crc = get32(&q);
    }
    actual_crc = crc32_calc(buffer, size - 4u);
    if (stored_crc != actual_crc) {
        wm_hs_system_init(system, adjusted_reset_value);
        return WM_HS_LOAD_RESET;
    }

    memset(system, 0, sizeof(*system));
    system->adjusted_reset_value = adjusted_reset_value;
    system->reset_counter.value = get32(&p);
    system->reset_counter.verifier = get32(&p);

    for (i = 0; i < WM_HS_TABLE_COUNT; ++i) {
        system->recent_index[i] = get16(&p);
    }

    decode_entries(&p, system->streak, WM_HS_STREAK_LAST_ENTRY + 1u);
    decode_entries(&p, system->pin_speed, WM_HS_PIN_SPEED_LAST_ENTRY + 1u);
    decode_entries(&p, system->beaten, WM_HS_BEATEN_LAST_ENTRY + 1u);
    decode_entries(&p, system->inter, WM_HS_INTER_LAST_ENTRY + 1u);
    decode_entries(&p, system->tag, WM_HS_TAG_LAST_ENTRY + 1u);

    wm_hs_system_rebind(system);

    {
        bool counter_repaired = false;
        (void)wm_hs_counter_get(&system->reset_counter,
                                adjusted_reset_value,
                                &counter_repaired);
        repaired |= counter_repaired;
    }

    {
        WmHsEntry before_streak = system->streak[1];
        WmHsEntry before_pin = system->pin_speed[1];
        WmHsEntry before_beaten = system->beaten[1];
        WmHsEntry before_inter = system->inter[1];
        WmHsEntry before_tag = system->tag[1];

        if (!wm_hs_system_table_cmos_check(system)) {
            repaired = true;
        }

        /*
         * Detect a source-style table reinitialization for load status.
         * The wrapper itself already applies INIT_HSTRING audit clearing.
         */
        if (memcmp(&before_streak, &system->streak[1], sizeof(WmHsEntry)) != 0 ||
            memcmp(&before_pin, &system->pin_speed[1], sizeof(WmHsEntry)) != 0 ||
            memcmp(&before_beaten, &system->beaten[1], sizeof(WmHsEntry)) != 0 ||
            memcmp(&before_inter, &system->inter[1], sizeof(WmHsEntry)) != 0 ||
            memcmp(&before_tag, &system->tag[1], sizeof(WmHsEntry)) != 0) {
            repaired = true;
        }
    }

    for (i = 0; i < WM_HS_TABLE_COUNT; ++i) {
        if (system->recent_index[i] >
            system->tables[i].template_def->last_entry) {
            system->recent_index[i] = 0u;
            repaired = true;
        }
    }

    return repaired ? WM_HS_LOAD_REPAIRED : WM_HS_LOAD_OK;
}

bool wm_hs_save_write(
    const WmHsSystem *system,
    const WmHsSaveBackend *backend)
{
    uint8_t buffer[WM_HS_SAVE_MAX_BYTES];
    size_t size;

    if (backend == 0 || backend->write == 0 ||
        !wm_hs_save_encode(system, buffer, sizeof(buffer), &size)) {
        return false;
    }

    return backend->write(backend->user, buffer, size) == 0;
}

WmHsLoadResult wm_hs_save_read(
    WmHsSystem *system,
    const WmHsSaveBackend *backend,
    uint32_t adjusted_reset_value)
{
    uint8_t buffer[WM_HS_SAVE_MAX_BYTES];
    size_t size = wm_hs_save_encoded_size();

    if (backend == 0 || backend->read == 0 ||
        backend->read(backend->user, buffer, size) != 0) {
        wm_hs_system_init(system, adjusted_reset_value);
        return WM_HS_LOAD_IO_ERROR;
    }

    return wm_hs_save_decode(
        system, buffer, size, adjusted_reset_value);
}
