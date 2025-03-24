#ifndef KEYMAP_H
#define KEYMAP_H

#if defined(__arm__) && defined(__linux__)
#define BTN_UP          119
#define BTN_DOWN        115
#define BTN_LEFT        113
#define BTN_RIGHT       100
#define BTN_A           97
#define BTN_B           98
#define BTN_X           120
#define BTN_Y           121
#define BTN_L1          104
#define BTN_R1          108
#define BTN_L2          106
#define BTN_R2          107
#define BTN_SELECT      110
#define BTN_START       109
#define BTN_MENU        117
#define BTN_VOLUME_UP   114
#define BTN_VOLUME_DOWN 116
#define BTN_POWER       0
#define BTN_EXIT        0
#else
#define FULL_KEYBOARD
#define BTN_UP          82
#define BTN_DOWN        81
#define BTN_LEFT        80
#define BTN_RIGHT       79
#define BTN_A           100
#define BTN_B           120
#define BTN_X           119
#define BTN_Y           97
#define BTN_L1          49
#define BTN_R1          48
#define BTN_L2          50
#define BTN_R2          57
#define BTN_SELECT      32
#define BTN_START       13
#define BTN_MENU        101
#define BTN_VOLUME_UP   61
#define BTN_VOLUME_DOWN 45
#define BTN_POWER       27
#define BTN_EXIT        113
#endif

typedef struct {
  int key;
  const char * name;
} keyinfo_t;

extern keyinfo_t keymap[];

#endif