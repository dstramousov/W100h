#include "pt3player.h"
#include "pt3player_test_api.h"

#include <stddef.h>
#include <string.h>

int forced_notetable = -1;

static uint8_t registers[2][14];
static int tick_counts[2];
static int active_chips;

static int is_02ts(const uint8_t* music_ptr, int length) {
    return length >= 4 && memcmp(music_ptr + length - 4, "02TS", 4) == 0;
}

void func_mute(void) {
    memset(registers, 0, sizeof(registers));
}

void func_play_tick(int ch) {
    if (ch < 0 || ch >= active_chips || ch >= 2) {
        return;
    }

    ++tick_counts[ch];
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
    memset(registers, 0, sizeof(registers));
    memset(tick_counts, 0, sizeof(tick_counts));
    return active_chips;
}

int func_restart_music(int ch) {
    if (ch < 0 || ch >= active_chips) {
        return 0;
    }
    tick_counts[ch] = 0;
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

int pt3_stub_tick_count(int ch) {
    if (ch < 0 || ch >= 2) {
        return -1;
    }
    return tick_counts[ch];
}
