#include "pt3player_bridge.h"

/*
 * The pinned Volutar decoder intentionally exposes only playback functions.
 * W100h needs a tiny amount of read-only sequencer state for a real transport
 * timeline. Compile the pinned implementation in this translation unit so the
 * bridge can read its internal packed structures without modifying upstream.
 */
#include <pt3player.c>

static int valid_channel(int ch) {
    return ch >= 0 && ch < 10;
}

int w100h_pt3_current_position(int ch) {
    if (!valid_channel(ch)) {
        return -1;
    }
    return PlParams[ch].PT3.CurrentPosition;
}

int w100h_pt3_position_count(int ch) {
    if (!valid_channel(ch)) {
        return -1;
    }
    return PlConsts[ch].RAM.PT3_NumberOfPositions;
}

int w100h_pt3_loop_position(int ch) {
    if (!valid_channel(ch)) {
        return -1;
    }
    return PlConsts[ch].RAM.PT3_LoopPosition;
}

int w100h_pt3_pattern_address(int ch, int voice) {
    if (!valid_channel(ch) || voice < 0 || voice >= 3) {
        return -1;
    }
    return PlParams[ch].PT3_[voice].Address_In_Pattern;
}
