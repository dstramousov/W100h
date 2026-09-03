#ifndef W100H_TEST_PT3PLAYER_H
#define W100H_TEST_PT3PLAYER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void func_mute(void);
void func_play_tick(int ch);
int func_setup_music(uint8_t* music_ptr, int length, int ch, int first);
int func_restart_music(int ch);
void func_getregs(uint8_t* dest, int ch);

extern int forced_notetable;

#ifdef __cplusplus
}
#endif

#endif
