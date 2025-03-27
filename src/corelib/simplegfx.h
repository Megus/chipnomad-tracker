#ifndef SIMPLEGFX_H
#define SIMPLEGFX_H

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 640
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 480
#endif

#ifndef FPS
#define FPS 30
#endif

#include <SDL/SDL.h>
#undef main

extern SDL_Surface * screen;

extern uint32_t elm;
extern char * printf_buf;
extern int printf_size;
extern int printf_len;
extern int full_kb;

#include <time.h>
#include <math.h>
#include <keymap.h>
#include <simplefont.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

#ifndef STD_RAND
#define rand gfx_fast_rand
#endif

int gfx_setup(void);
void gfx_cleanup(void);
void gfx_point(int x, int y);
void gfx_set_color(int r, int g, int b);
void gfx_set_font(font_t * font);
font_t * gfx_get_font(void);
int gfx_font_table(int x, int y, int size);
int gfx_text(const char * text, int x, int y, int size);
int gfx_clear_text_buffer(void);
int gfx_printf(const char * format, ...);
void gfx_clear(void);
void gfx_fill_rect(int x, int y, int w, int h);
void gfx_run(void);
void gfx_screenshot(const char * filename);
void gfx_delay(int ms);

int gfx_fast_rand(void);

extern void gfx_app(int init);
extern void gfx_draw(float fps);
extern int gfx_on_key(char key, int down);
extern void gfx_process_data(int compute_time);

#endif