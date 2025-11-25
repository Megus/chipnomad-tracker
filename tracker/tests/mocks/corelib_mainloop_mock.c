#include "corelib_mainloop.h"

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata)) {}
void mainLoopDelay(int ms) {}
void mainLoopQuit(void) {}