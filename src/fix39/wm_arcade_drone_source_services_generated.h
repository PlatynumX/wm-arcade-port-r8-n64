#ifndef WM_ARCADE_DRONE_SOURCE_SERVICES_GENERATED_H
#define WM_ARCADE_DRONE_SOURCE_SERVICES_GENERATED_H
#include <stdint.h>
#define WM_FIX39_DRONE_SERVICES_GENERATED 1
#define WM_FIX39_DRONE_SERVICE_COUNT 15
typedef enum WmFix39DroneServiceId {
    WM_FIX39_DRONE_SERVICE_INVALID = -1,
    WM_FIX39_DRONE_SERVICE_DRN_SEEK_EXGPC_0000 = 0,
    WM_FIX39_DRONE_SERVICE_DRN_RETREAT_EXGPC_0000 = 1,
    WM_FIX39_DRONE_SERVICE_DRONE_CHRG = 2,
    WM_FIX39_DRONE_SERVICE_DRN_CLIMBTB_EXGPC_0000 = 3,
    WM_FIX39_DRONE_SERVICE_DRN_TAUNT_EXGPC_0000 = 4,
    WM_FIX39_DRONE_SERVICE_DRN_ENTERRING_EXGPC_0000 = 5,
    WM_FIX39_DRONE_SERVICE_DRN_OPINAIR_EXGPC_0000 = 6,
    WM_FIX39_DRONE_SERVICE_DRN_OPRUN_EXGPC_0000 = 7,
    WM_FIX39_DRONE_SERVICE_DRN_ROLL_EXGPC_0000 = 8,
    WM_FIX39_DRONE_SERVICE_DRN_INAIR_EXGPC_0000 = 9,
    WM_FIX39_DRONE_SERVICE_DRN_ONTB_EXGPC_0000 = 10,
    WM_FIX39_DRONE_SERVICE_DRN_RUN_EXGPC_0000 = 11,
    WM_FIX39_DRONE_SERVICE_DRN_COMBO_EXGPC_0000 = 12,
    WM_FIX39_DRONE_SERVICE_DRN_SEEKCLOSE_EXGPC_0000 = 13,
    WM_FIX39_DRONE_SERVICE_DRN_OPPDEAD_EXGPC_0000 = 14,
} WmFix39DroneServiceId;
typedef enum WmFix39DroneServiceKind {
    WM_FIX39_DRONE_SERVICE_CALL_CODE = 0,
    WM_FIX39_DRONE_SERVICE_EXGPC_INLINE = 1
} WmFix39DroneServiceKind;
static const char *const wm_fix39_drone_service_labels[WM_FIX39_DRONE_SERVICE_COUNT] = {
    "drn_seek@EXGPC_0000",
    "drn_retreat@EXGPC_0000",
    "drone_chrg",
    "drn_climbtb@EXGPC_0000",
    "drn_taunt@EXGPC_0000",
    "drn_enterring@EXGPC_0000",
    "drn_opinair@EXGPC_0000",
    "drn_oprun@EXGPC_0000",
    "drn_roll@EXGPC_0000",
    "drn_inair@EXGPC_0000",
    "drn_ontb@EXGPC_0000",
    "drn_run@EXGPC_0000",
    "drn_combo@EXGPC_0000",
    "drn_seekclose@EXGPC_0000",
    "drn_oppdead@EXGPC_0000",
};
static const uint32_t wm_fix39_drone_service_source_addr[WM_FIX39_DRONE_SERVICE_COUNT] = {
    0x0000a4a0u,
    0x0000a510u,
    0x000088c0u,
    0x0000a800u,
    0x0000a9f0u,
    0x0000a9a0u,
    0x0000a900u,
    0x0000a740u,
    0x0000a790u,
    0x0000a8d0u,
    0x0000a820u,
    0x0000a570u,
    0x00009200u,
    0x0000a4e0u,
    0x0000ab40u,
};
static const uint8_t wm_fix39_drone_service_kind[WM_FIX39_DRONE_SERVICE_COUNT] = {
    1u,
    1u,
    0u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
    1u,
};
#endif
