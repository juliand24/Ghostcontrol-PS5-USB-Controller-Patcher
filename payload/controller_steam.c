#include "controller_steam.h"

#include <string.h>

enum {
    STEAM_A            = 1u << 0,
    STEAM_B            = 1u << 1,
    STEAM_X            = 1u << 2,
    STEAM_Y            = 1u << 3,
    STEAM_R3           = 1u << 5,
    STEAM_VIEW         = 1u << 6,
    STEAM_R            = 1u << 9,
    STEAM_DPAD_DOWN    = 1u << 10,
    STEAM_DPAD_RIGHT   = 1u << 11,
    STEAM_DPAD_LEFT    = 1u << 12,
    STEAM_DPAD_UP      = 1u << 13,
    STEAM_MENU         = 1u << 14,
    STEAM_L3           = 1u << 15,
    STEAM_STEAM        = 1u << 16,
    STEAM_L            = 1u << 19,
    STEAM_R2_CLICK     = 1u << 23,
    STEAM_L2_CLICK     = 1u << 27,
};

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int16_t read_i16(const uint8_t *p) {
    return (int16_t)read_u16(p);
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint8_t stick_axis(int16_t value) {
    return (uint8_t)(((int32_t)value + 32768) >> 8);
}

static uint8_t trigger_axis(int16_t value) {
    if (value <= 0) return 0;
    if (value >= 32767) return 255;
    return (uint8_t)(((uint32_t)value * 255u) / 32767u);
}

/*
 * USB report 0x42, excluding the report ID:
 *   0: sequence, 1: buttons (uint32), 5: triggers (int16 pair),
 *   9: sticks (four int16).  The remaining fields are pads and IMU data.
 */
void steam_parse_state(const uint8_t *buf, uint32_t len, ScePadData *out) {
    const uint8_t *p = buf + 1;
    uint32_t buttons = read_u32(p + 1);
    uint8_t l2 = trigger_axis(read_i16(p + 5));
    uint8_t r2 = trigger_axis(read_i16(p + 7));
    int16_t lx = read_i16(p + 9);
    int16_t ly = read_i16(p + 11);
    int16_t rx = read_i16(p + 13);
    int16_t ry = read_i16(p + 15);

    memset(out, 0, sizeof(*out));
    if (buttons & STEAM_A)         out->buttons |= SCE_PAD_BUTTON_CROSS;
    if (buttons & STEAM_B)         out->buttons |= SCE_PAD_BUTTON_CIRCLE;
    if (buttons & STEAM_X)         out->buttons |= SCE_PAD_BUTTON_SQUARE;
    if (buttons & STEAM_Y)         out->buttons |= SCE_PAD_BUTTON_TRIANGLE;
    if (buttons & STEAM_L)         out->buttons |= SCE_PAD_BUTTON_L1;
    if (buttons & STEAM_R)         out->buttons |= SCE_PAD_BUTTON_R1;
    if (buttons & STEAM_L3)        out->buttons |= SCE_PAD_BUTTON_L3;
    if (buttons & STEAM_R3)        out->buttons |= SCE_PAD_BUTTON_R3;
    if (buttons & STEAM_MENU)      out->buttons |= SCE_PAD_BUTTON_OPTIONS;
    if (buttons & STEAM_VIEW)      out->buttons |= SCE_PAD_BUTTON_CREATE;
    if (buttons & STEAM_STEAM)     out->buttons |= SCE_PAD_BUTTON_PS;
    if (buttons & STEAM_DPAD_UP)   out->buttons |= SCE_PAD_BUTTON_UP;
    if (buttons & STEAM_DPAD_DOWN) out->buttons |= SCE_PAD_BUTTON_DOWN;
    if (buttons & STEAM_DPAD_LEFT) out->buttons |= SCE_PAD_BUTTON_LEFT;
    if (buttons & STEAM_DPAD_RIGHT)out->buttons |= SCE_PAD_BUTTON_RIGHT;
    if ((buttons & STEAM_L2_CLICK) || l2 > 16) out->buttons |= SCE_PAD_BUTTON_L2;
    if ((buttons & STEAM_R2_CLICK) || r2 > 16) out->buttons |= SCE_PAD_BUTTON_R2;

    out->leftStick.x = stick_axis(lx);
    out->leftStick.y = (uint8_t)(255u - stick_axis(ly));
    out->rightStick.x = stick_axis(rx);
    out->rightStick.y = (uint8_t)(255u - stick_axis(ry));
    out->analogButtons.l2 = l2;
    out->analogButtons.r2 = r2;
    out->connected = 1;
    out->quat.w = 1.0f;
    (void)len;
}

int steam_handle_packet(const uint8_t *buf, uint32_t len, ScePadData *out) {
    if (len < 18 || buf[0] != STEAM_REPORT_STATE) return 0;
    steam_parse_state(buf, len, out);
    return 1;
}
