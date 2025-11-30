#ifndef __APP_H__
#define __APP_H__

#include "common.h"

void appSetup(void);
void appCleanup(void);
void appDraw(void);
void appOnEvent(enum MainLoopEvent event, int value, void* userdata);

#endif