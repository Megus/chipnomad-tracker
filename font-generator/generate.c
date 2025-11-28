#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
  int width, height;
} FontSize;

const FontSize sizes[] = {{12,16}, {16,24}, {24,36}, {32,48}, {48,54}};
const int num_sizes = 5;

void generate_preview_image(FT_Face face) {
  const char* sample = "ChipNomad 0123456789 !@#$%^&*()";
  int img_width = 800, img_height = 600;
  unsigned char* img = calloc(img_width * img_height * 3, 1);
  
  int y_pos = 20;
  for (int i = 0; i < num_sizes; i++) {
      FT_Set_Pixel_Sizes(face, 0, sizes[i].height);
      
      int x_pos = 10;
      for (const char* p = sample; *p; p++) {
      if (FT_Load_Char(face, *p, FT_LOAD_RENDER | FT_LOAD_MONOCHROME)) continue;
      
      FT_Bitmap* bitmap = &face->glyph->bitmap;
      int left = face->glyph->bitmap_left;
      int top = face->glyph->bitmap_top;
      
      for (unsigned int by = 0; by < bitmap->rows; by++) {
          for (unsigned int bx = 0; bx < bitmap->width; bx++) {
        int src_byte = by * bitmap->pitch + bx / 8;
        int src_bit = 7 - (bx % 8);
        
        if (bitmap->buffer[src_byte] & (1 << src_bit)) {
            int px = x_pos + left + bx;
            int py = y_pos + sizes[i].height - top + by;
            
            if (px >= 0 && px < img_width && py >= 0 && py < img_height) {
        int idx = (py * img_width + px) * 3;
        img[idx] = img[idx+1] = img[idx+2] = 255;
            }
        }
          }
      }
      x_pos += face->glyph->advance.x >> 6;
      }
      y_pos += sizes[i].height + 20;
  }
  
  stbi_write_png("font_preview.png", img_width, img_height, 3, img, img_width * 3);
  free(img);
  printf("Generated font_preview.png\n");
}

void generate_font_file(FT_Face face, FontSize size) {
  char filename[64];
  snprintf(filename, sizeof(filename), "font%dx%d.c", size.width, size.height);
  
  FILE* f = fopen(filename, "w");
  if (!f) return;
  
  fprintf(f, "#include <stdint.h>\n\n");
  fprintf(f, "const uint8_t font%dx%d[] = {\n", size.width, size.height);
  
  FT_Set_Pixel_Sizes(face, 0, size.height);
  
  for (int c = 32; c <= 126; c++) {
      if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_MONOCHROME)) continue;
      
      FT_Bitmap* bitmap = &face->glyph->bitmap;
      int bytes_per_row = (size.width + 7) / 8;
      
      fprintf(f, "    // Character '%c'\n", c);
      
      int baseline = size.height * 4 / 5; // Place baseline at 80% of font height
      int glyph_top = baseline - face->glyph->bitmap_top;
      int glyph_left = (size.width - (int)bitmap->width) / 2; // Center horizontally
      
      for (int y = 0; y < size.height; y++) {
      fprintf(f, "    ");
      for (int byte_x = 0; byte_x < bytes_per_row; byte_x++) {
          uint8_t byte_val = 0;
          for (int bit = 0; bit < 8; bit++) {
        int x = byte_x * 8 + bit;
        int bitmap_y = y - glyph_top;
        int bitmap_x = x - glyph_left;
        if (x < size.width && bitmap_y >= 0 && bitmap_y < (int)bitmap->rows && bitmap_x >= 0 && bitmap_x < (int)bitmap->width) {
            int src_byte = bitmap_y * bitmap->pitch + bitmap_x / 8;
            int src_bit = 7 - (bitmap_x % 8);
            if (bitmap->buffer[src_byte] & (1 << src_bit)) {
        byte_val |= (1 << (7 - bit));
            }
        }
          }
          fprintf(f, "0x%02X", byte_val);
          if (byte_x < bytes_per_row - 1) fprintf(f, ", ");
      }
      if (y < size.height - 1 || c < 126) fprintf(f, ",");
      fprintf(f, "\n");
      }
  }
  
  fprintf(f, "};\n");
  fclose(f);
  printf("Generated %s\n", filename);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
      printf("Usage: %s <font.ttf>\n", argv[0]);
      return 1;
  }
  
  FT_Library library;
  FT_Face face;
  
  if (FT_Init_FreeType(&library)) {
      printf("FreeType init failed\n");
      return 1;
  }
  
  if (FT_New_Face(library, argv[1], 0, &face)) {
      printf("Font loading failed: %s\n", argv[1]);
      FT_Done_FreeType(library);
      return 1;
  }
  
  for (int i = 0; i < num_sizes; i++) {
      generate_font_file(face, sizes[i]);
  }
  
  generate_preview_image(face);
  
  FT_Done_Face(face);
  FT_Done_FreeType(library);
  return 0;
}