#include "pt3player.h"
#include "pt3player_test_api.h"
#include "audio/pt3player_bridge.h"

#include <stddef.h>
#include <string.h>

int forced_notetable = -1;

static uint8_t registers[2][14];
static int tick_counts[2];
static int active_chips;
static int current_positions[2];
static int pattern_addresses[2][3];

static const int position_count = 4;
static const int loop_position = 1;
static const int ticks_per_position = 4;

static int is_02ts(const uint8_t* music_ptr, int length) {
    return length >= 4 && memcmp(music_ptr + length - 4, "02TS", 4) == 0;
}

static void reset_decoder_state(void) {
    memset(registers, 0, sizeof(registers));
    memset(tick_counts, 0, sizeof(tick_counts));
    memset(current_positions, 0, sizeof(current_positions));
    memset(pattern_addresses, 0, sizeof(pattern_addresses));
}

void func_mute(void) {
    memset(registers, 0, sizeof(registers));
}

void func_play_tick(int ch) {
    if (ch < 0 || ch >= active_chips || ch >= 2) {
        return;
    }

    ++tick_counts[ch];
    for (int voice = 0; voice < 3; ++voice) {
        ++pattern_addresses[ch][voice];
    }
    if ((tick_counts[ch] % ticks_per_position) == 0) {
        ++current_positions[ch];
        if (current_positions[ch] >= position_count) {
            current_positions[ch] = loop_position;
        }
        for (int voice = 0; voice < 3; ++voice) {
            pattern_addresses[ch][voice] = current_positions[ch] * 100;
        }
    }

    memset(registers[ch], 0, sizeof(registers[ch]));
    registers[ch][0] = (uint8_t)(32 + ch * 32 + tick_counts[ch]);
    registers[ch][1] = 1;
    registers[ch][7] = 0x3e;
    registers[ch][8] = 15;
    registers[ch][13] = tick_counts[ch] == 1 ? 9 : 0xff;
}

int func_setup_music(uint8_t* music_ptr, int length, int ch, int first) {
    (void)ch;
    (void)first;
    if (music_ptr == NULL || length <= 0) {
        return 0;
    }

    active_chips = is_02ts(music_ptr, length) ? 2 : 1;
    reset_decoder_state();
    return active_chips;
}

int func_restart_music(int ch) {
    if (ch < 0 || ch >= active_chips) {
        return 0;
    }
    tick_counts[ch] = 0;
    current_positions[ch] = 0;
    memset(pattern_addresses[ch], 0, sizeof(pattern_addresses[ch]));
    memset(registers[ch], 0, sizeof(registers[ch]));
    return 1;
}

void func_getregs(uint8_t* dest, int ch) {
    if (dest == NULL) {
        return;
    }
    if (ch < 0 || ch >= active_chips || ch >= 2) {
        memset(dest, 0, 14);
        return;
    }
    memcpy(dest, registers[ch], 14);
}

int w100h_pt3_current_position(int ch) {
    if (ch < 0 || ch >= active_chips || ch >= 2) {
        return -1;
    }
    return current_positions[ch];
}

int w100h_pt3_position_count(int ch) {
    return ch >= 0 && ch < active_chips ? position_count : -1;
}

int w100h_pt3_loop_position(int ch) {
    return ch >= 0 && ch < active_chips ? loop_position : -1;
}

int w100h_pt3_pattern_address(int ch, int voice) {
    if (ch < 0 || ch >= active_chips || ch >= 2 || voice < 0 || voice >= 3) {
        return -1;
    }
    return pattern_addresses[ch][voice];
}

int pt3_stub_tick_count(int ch) {
    if (ch < 0 || ch >= 2) {
        return -1;
    }
    return tick_counts[ch];
}
