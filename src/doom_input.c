#include "PureDOOM/PureDOOM.h"
#include "main.h"

static const int16_t DOOM_DEADZONE = 0x3000;
extern uint8_t doom_initd;
extern int16_t doom_rightx;

static int apply_deadzone(int input, int lower_deadzone, int upper_deadzone)
{
    int is_negative = input < 0;

    if (is_negative) {
        input = -input;
    }

    upper_deadzone = 32767 - upper_deadzone;

    if (input < lower_deadzone) {
        input = 0;
    } else if (input > upper_deadzone) {
        input = 32767;
    } else {
        input = (input - lower_deadzone) * 32767 / (32767 - lower_deadzone);
    }

    if (is_negative) {
        input = -input;
    }

    return input;
}

void dooom_new_input(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry, uint8_t lt, uint8_t rt)
{
    static uint16_t old_buttons = 0;
    static uint8_t old_lt = 0;
    static uint8_t old_rt = 0;
    static int16_t old_ly = 0;
    static int16_t old_ry = 0;
    static int16_t old_lx = 0;
    static int16_t old_rx = 0;
    static uint8_t current_weapon_index = 0xFF;

    if (doom_initd == 0) {
        return;
    }

    // Digitize
    lt = (lt > 0x20) ? 1 : 0;
    rt = (rt > 0x20) ? 1 : 0;
    ly = (ly > DOOM_DEADZONE) ? 1 : (ly < -DOOM_DEADZONE) ? -1 : 0;
    lx = (lx > DOOM_DEADZONE) ? 1 : (lx < -DOOM_DEADZONE) ? -1 : 0;

    const uint16_t buttons_changed = buttons ^ old_buttons;
    const uint8_t lt_changed = old_lt != lt;
    const uint8_t rt_changed = old_rt != rt;
    const int16_t ly_changed = old_ly != ly;
    const int16_t lx_changed = old_lx != lx;

    // Calculate new position for right stick and apply it in main loop
    rx = apply_deadzone(rx, DOOM_DEADZONE, 0x1000);
    doom_rightx = rx;

    if (!buttons_changed && !lt_changed && !rt_changed && !ly_changed && !lx_changed) {
        return;
    }

    vPortEnterCritical();

    // Shoot (Right trigger)
    if (rt_changed) {
        if (rt) {
            doom_key_down(DOOM_KEY_CTRL);
        } else {
            doom_key_up(DOOM_KEY_CTRL);
        }
    }

    // Move Forward/Backward (Left stick)
    if (ly_changed) {
        if (ly == 1) {
            doom_key_down(DOOM_KEY_UP_ARROW);
        } else if (ly == 0 && old_ly == 1) {
            doom_key_up(DOOM_KEY_UP_ARROW);
        } else if (ly == -1) {
            doom_key_down(DOOM_KEY_DOWN_ARROW);
        } else if (ly == 0 && old_ly == -1) {
            doom_key_up(DOOM_KEY_DOWN_ARROW);
        }
    }

    // Strafe Left/Right (Left stick)
    if (lx_changed) {
        if (lx == 1) {
            doom_key_down(DOOM_KEY_PERIOD);
        } else if (lx == 0 && old_lx == 1) {
            doom_key_up(DOOM_KEY_PERIOD);
        } else if (lx == -1) {
            doom_key_down(DOOM_KEY_COMMA);
        } else if (lx == 0 && old_lx == -1) {
            doom_key_up(DOOM_KEY_COMMA);
        }
    }

    // Vanilla Arrow Key Mapping to D-Pad
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_UP) {
        if (buttons & XINPUT_GAMEPAD_DPAD_UP) {
            doom_key_down(DOOM_KEY_UP_ARROW);
        } else {
            doom_key_up(DOOM_KEY_UP_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_DOWN) {
        if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) {
            doom_key_down(DOOM_KEY_DOWN_ARROW);
        } else {
            doom_key_up(DOOM_KEY_DOWN_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_LEFT) {
        if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) {
            doom_key_down(DOOM_KEY_LEFT_ARROW);
        } else {
            doom_key_up(DOOM_KEY_LEFT_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_RIGHT) {
        if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) {
            doom_key_down(DOOM_KEY_RIGHT_ARROW);
        } else {
            doom_key_up(DOOM_KEY_RIGHT_ARROW);
        }
    }

    // Run (Press left stick or left trigger)
    if (buttons_changed & XINPUT_GAMEPAD_LEFT_THUMB) {
        if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
            doom_key_down(DOOM_KEY_SHIFT);
        } else {
            doom_key_up(DOOM_KEY_SHIFT);
        }
    }
    if (lt_changed) {
        if (lt) {
            doom_key_down(DOOM_KEY_SHIFT);
        } else {
            doom_key_up(DOOM_KEY_SHIFT);
        }
    }

    // Enter/Accept (A)
    if (buttons_changed & XINPUT_GAMEPAD_A) {
        if (buttons & XINPUT_GAMEPAD_A) {
            doom_key_down(DOOM_KEY_ENTER);
        } else {
            doom_key_up(DOOM_KEY_ENTER);
        }
    }

    // Show/Hide Menu (Start)
    if (buttons_changed & XINPUT_GAMEPAD_START) {
        if (buttons & XINPUT_GAMEPAD_START) {
            doom_key_down(DOOM_KEY_ESCAPE);
        } else {
            doom_key_up(DOOM_KEY_ESCAPE);
        }
    }

    // Use/Open (X or B)
    if (buttons_changed & XINPUT_GAMEPAD_X) {
        if (buttons & XINPUT_GAMEPAD_X) {
            doom_key_down(DOOM_KEY_SPACE);
        } else {
            doom_key_up(DOOM_KEY_SPACE);
        }
    }

    // Back/Cancel (B)
    if (buttons_changed & XINPUT_GAMEPAD_B) {
        if (buttons & XINPUT_GAMEPAD_B) {
            doom_key_down(DOOM_KEY_BACKSPACE);
        } else {
            doom_key_up(DOOM_KEY_BACKSPACE);
        }
    }

    // Map View (back)
    if (buttons_changed & XINPUT_GAMEPAD_BACK) {
        if (buttons & XINPUT_GAMEPAD_BACK) {
            doom_key_down(DOOM_KEY_TAB);
        } else {
            doom_key_up(DOOM_KEY_TAB);
        }
    }

    // Change Weapon (Y)
    // Game doesnt support weapon cycle, so we do it here
    if (buttons_changed & XINPUT_GAMEPAD_Y) {
        extern player_t players[MAXPLAYERS];
        static int key = DOOM_KEY_1;

        if (buttons & XINPUT_GAMEPAD_Y) {
            if (current_weapon_index == 0xFF) {
                current_weapon_index = players[0].readyweapon;
            }

            current_weapon_index = (current_weapon_index + 1) % NUMWEAPONS;
            for (int i = 0; i < NUMWEAPONS; i++) {
                uint8_t index = (current_weapon_index + i) % NUMWEAPONS;
                if (players[0].weaponowned[index]) {
                    current_weapon_index = index;
                    key = DOOM_KEY_1 + index;
                    break;
                }
            }
            doom_key_down(key);
        } else {
            doom_key_up(key);
        }
    }

    if (buttons_changed & XINPUT_GAMEPAD_LEFT_SHOULDER) {
        if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
            doom_key_down(DOOM_KEY_MINUS);
        } else {
            doom_key_up(DOOM_KEY_MINUS);
        }
    }

    if (buttons_changed & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
        if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
            doom_key_down(DOOM_KEY_EQUALS);
        } else {
            doom_key_up(DOOM_KEY_EQUALS);
        }
    }

    old_buttons = buttons;
    old_lt = lt;
    old_rt = rt;
    old_ly = ly;
    old_ry = ry;
    old_lx = lx;
    old_rx = rx;
    vPortExitCritical();
}
