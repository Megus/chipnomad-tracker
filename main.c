#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <simplegfx.h>

char last_key = 0;
keyinfo_t * keyinfo = NULL;
char text[32] = {0};
char menu = 0;
int compute_loops = 0;
int mb = 0;

extern uint32_t elm;
extern int printf_len;

void gfx_app(int init) {
  if (init) {
    printf("App started\n");
  } else {
    printf("App stopped\n");
  }
}

void gfx_process_data(int compute_time) {
  gfx_delay(1);
  compute_loops++;
}

void gfx_draw(float fps) {
  int y;
  if (printf_len == 0) {
    gfx_set_color(255, 0, 255);
    y = gfx_font_table(10, 50, 1);
    gfx_set_color(255, 255, 0);
    y = gfx_font_table(10, y + 10, 2);
    gfx_set_color(0, 255, 255);
    y = gfx_font_table(10, y + 10, 3);
  } else if (!mb) {
    gfx_set_color(255, 0, 255);
    y = gfx_text(printf_buf, 10, 50, 1);
    gfx_set_color(255, 255, 0);
    y = gfx_text(printf_buf, 10, y + 10, 2);
    gfx_set_color(0, 255, 255);
    y = gfx_text(printf_buf, 10, y + 10, 3);
  } else {
    gfx_set_color(57, 255, 20);
    y = gfx_text(printf_buf, 8, 50, 1);
  }

  gfx_set_color(255, 255, 255);
  if (last_key != 0) {
    if (keyinfo) {
      sprintf(text, "key: %s/%d", keyinfo->name, last_key);
    } else {
      sprintf(text, "key: %d", last_key);
    }
    gfx_text(text, 10, 10, 2);
  } else {
    gfx_text("\xae Press MENU + X to exit \xaf", 10, 10, 2);
  }

  sprintf(text, "%.1f fps | %.1fk draws ", fps, elm/1000.0);
  gfx_text(text, 480, 8, 1);
  sprintf(text, "compute %d | buffer %.1fk", compute_loops, printf_len/1000.0);
  gfx_text(text, 480, 18, 1);
  compute_loops = 0;
}

int gfx_on_key(char key, int down) {
  for (int i = 0; keymap[i].name; i++) {
    if (keymap[i].key == key) {
      keyinfo = &keymap[i];
      break;
    }
  }
  if (mb) {
    mb = 0;
    gfx_clear_text_buffer();
  }
  if (down) {
    if (keyinfo) {
      gfx_printf("+%s ", keyinfo->name);
    } else {
      gfx_printf("+%d ", key);
    }
    if (key == BTN_MENU) {
      menu = 1;
    }
    if (menu && key == BTN_X) {
      return 1;
    }
    last_key = key;
  } else {
    if (keyinfo) {
      gfx_printf("-%s ", keyinfo->name);
    } else {
      gfx_printf("-%d ", key);
    }
    if (key == BTN_MENU) {
      gfx_clear_text_buffer();
      menu = 0;
    }
    last_key = 0;
    keyinfo = NULL;
  }
  return 0;
}



int main(int argv, char** args) {
  if (gfx_setup() != 0) {
    return 1;
  }

  ayumi_set_tone(chip, 0, 512);
  ayumi_set_volume(chip, 0, 15);
  ayumi_set_mixer(chip, 0, 0, 1, 0);

  gfx_set_font(&font5x7);
  gfx_run();
  gfx_app(0);
  gfx_cleanup();
  return 0;
}
