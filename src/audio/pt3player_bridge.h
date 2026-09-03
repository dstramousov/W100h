#ifndef W100H_PT3PLAYER_BRIDGE_H
#define W100H_PT3PLAYER_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

int w100h_pt3_current_position(int ch);
int w100h_pt3_position_count(int ch);
int w100h_pt3_loop_position(int ch);
int w100h_pt3_pattern_address(int ch, int voice);

#ifdef __cplusplus
}
#endif

#endif
