#include "visuals.h"
#include <stdio.h>
#include <string.h>

#define GLYPH_CACHE_SIZE 128

struct GlyphCache {
    SDL_Texture* texture;
    int width, height;
    int bearing_x, bearing_y;
    int advance;
};

static struct GlyphCache glyphCache[GLYPH_CACHE_SIZE];

static void renderText(struct VisualState* visualState, const char* text, int x, int y, SDL_Color color) {
    if (!text || strlen(text) == 0) return;
    
    int pen_x = x;
    for (const char* p = text; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c >= GLYPH_CACHE_SIZE) continue;
        
        struct GlyphCache* glyph = &glyphCache[c];
        if (!glyph->texture) {
            pen_x += glyph->advance;
            continue;
        }
        
        SDL_SetTextureColorMod(glyph->texture, color.r, color.g, color.b);
        SDL_Rect dst = {pen_x + glyph->bearing_x, y - glyph->bearing_y, glyph->width, glyph->height};
        SDL_RenderCopy(visualState->renderer, glyph->texture, NULL, &dst);
        pen_x += glyph->advance;
    }
}

int visualsInit(struct VisualState* visualState) {
    if (FT_Init_FreeType(&visualState->ftLibrary)) {
        printf("Error initializing FreeType\n");
        return -1;
    }
    
    if (FT_New_Face(visualState->ftLibrary, visualState->config->fontPath, 0, &visualState->ftFace)) {
        printf("Error loading font\n");
        FT_Done_FreeType(visualState->ftLibrary);
        return -1;
    }
    
    FT_Set_Pixel_Sizes(visualState->ftFace, 0, visualState->config->fontSize);
    
    // Pre-render ASCII glyphs
    for (int c = 0; c < GLYPH_CACHE_SIZE; c++) {
        glyphCache[c].texture = NULL;
        
        if (FT_Load_Char(visualState->ftFace, c, FT_LOAD_RENDER)) continue;
        
        FT_GlyphSlot g = visualState->ftFace->glyph;
        glyphCache[c].advance = g->advance.x >> 6;
        glyphCache[c].bearing_x = g->bitmap_left;
        glyphCache[c].bearing_y = g->bitmap_top;
        
        if (g->bitmap.width == 0 || g->bitmap.rows == 0) continue;
        
        glyphCache[c].width = g->bitmap.width;
        glyphCache[c].height = g->bitmap.rows;
        
        SDL_Surface* surface = SDL_CreateRGBSurface(0, g->bitmap.width, g->bitmap.rows, 32,
                                                   0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (!surface) continue;
        
        Uint32* pixels = (Uint32*)surface->pixels;
        for (int row = 0; row < g->bitmap.rows; row++) {
            for (int col = 0; col < g->bitmap.width; col++) {
                unsigned char gray = g->bitmap.buffer[row * g->bitmap.pitch + col];
                pixels[row * g->bitmap.width + col] = (gray << 24) | 0xFFFFFF;
            }
        }
        
        glyphCache[c].texture = SDL_CreateTextureFromSurface(visualState->renderer, surface);
        SDL_FreeSurface(surface);
    }
    
    return 0;
}

void visualsRender(struct VisualState* visualState) {
    SDL_Color bg = visualState->config->backgroundColor;
    SDL_SetRenderDrawColor(visualState->renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(visualState->renderer);
    
    if (*visualState->isPlaying) {
        // Calculate font-based dimensions
        int fontSize = visualState->config->fontSize;
        int lineHeight = fontSize + fontSize / 4; // 25% spacing
        int charWidth = fontSize * 0.6; // Monospace character width approximation
        
        // Draw phrase visualization
        int trackCount = visualState->project->tracksCount;
        
        // Calculate track content width (note + inst + vol + 3 fx columns)
        int trackContentWidth = charWidth * (3 + 1 + 2 + 1 + 2 + 1 + 5 + 1 + 5 + 1 + 5); // 27 chars
        int trackSpacing = charWidth * 3; // 3 characters between tracks
        
        // Calculate total width and center horizontally
        int totalWidth = trackCount * trackContentWidth + (trackCount - 1) * trackSpacing;
        int startX = (visualState->config->windowWidth - totalWidth) / 2;
        
        // Center content vertically with slight downward adjustment
        int totalHeight = 16 * lineHeight;
        int startY = (visualState->config->windowHeight - totalHeight) / 2 + fontSize / 2;
        
        for (int trackIdx = 0; trackIdx < trackCount; trackIdx++) {
            struct PlaybackTrackState* track = &visualState->playback->tracks[trackIdx];
            
            if (track->mode == playbackModeStopped) continue;
            
            // Get current phrase
            uint16_t chainIdx = visualState->project->song[track->songRow][trackIdx];
            if (chainIdx == EMPTY_VALUE_16) continue;
            
            uint16_t phraseIdx = visualState->project->chains[chainIdx].rows[track->chainRow].phrase;
            if (phraseIdx == EMPTY_VALUE_16) continue;
            
            struct Phrase* phrase = &visualState->project->phrases[phraseIdx];
            
            // Calculate track position
            int trackStartX = startX + trackIdx * (trackContentWidth + trackSpacing);
            
            // Draw phrase rows
            for (int row = 0; row < 16; row++) {
                int x = trackStartX;
                int y = startY + row * lineHeight;
                
                // Draw current row indicator
                if (row == track->phraseRow) {
                    renderText(visualState, ">", x - charWidth, y, visualState->config->currentRowColor);
                }
                
                struct PhraseRow* phraseRow = &phrase->rows[row];
                
                int col = x;
                
                // Note column (3 chars)
                char* noteStr = noteName(phraseRow->note);
                if (strcmp(noteStr, "---") != 0) {
                    renderText(visualState, noteStr, col, y, visualState->config->noteColor);
                } else {
                    renderText(visualState, "---", col, y, visualState->config->dimmedColor);
                }
                col += charWidth * 4; // 3 chars + space
                
                // Instrument column (2 chars)
                if (phraseRow->instrument != EMPTY_VALUE_8) {
                    char instStr[3];
                    sprintf(instStr, "%02X", phraseRow->instrument);
                    renderText(visualState, instStr, col, y, visualState->config->instrumentColor);
                } else {
                    renderText(visualState, "--", col, y, visualState->config->dimmedColor);
                }
                col += charWidth * 3; // 2 chars + space
                
                // Volume column (2 chars)
                if (phraseRow->volume != EMPTY_VALUE_8) {
                    char volStr[3];
                    sprintf(volStr, "%02X", phraseRow->volume);
                    renderText(visualState, volStr, col, y, visualState->config->volumeColor);
                } else {
                    renderText(visualState, "--", col, y, visualState->config->dimmedColor);
                }
                col += charWidth * 3; // 2 chars + space
                
                // FX columns (5 chars each)
                for (int fx = 0; fx < 3; fx++) {
                    if (phraseRow->fx[fx][0] != EMPTY_VALUE_8) {
                        char fxStr[7];
                        sprintf(fxStr, "%s%02X", fxNames[phraseRow->fx[fx][0]].name, phraseRow->fx[fx][1]);
                        renderText(visualState, fxStr, col, y, visualState->config->fxColor);
                    } else {
                        renderText(visualState, "-----", col, y, visualState->config->dimmedColor);
                    }
                    col += charWidth * (fx < 2 ? 6 : 5); // 5 chars + space (except last)
                }
            }
        }
    } else {
        // Show start message
        const char* message = "Press any key to start playback";
        int fontSize = visualState->config->fontSize;
        int charWidth = fontSize * 0.6;
        int messageWidth = strlen(message) * charWidth;
        int x = (visualState->config->windowWidth - messageWidth) / 2;
        int y = visualState->config->windowHeight / 2;
        renderText(visualState, message, x, y, visualState->config->fxColor);
    }
    
    SDL_RenderPresent(visualState->renderer);
}

void visualsCleanup(struct VisualState* visualState) {
    for (int c = 0; c < GLYPH_CACHE_SIZE; c++) {
        if (glyphCache[c].texture) {
            SDL_DestroyTexture(glyphCache[c].texture);
        }
    }
    
    if (visualState->ftFace) {
        FT_Done_Face(visualState->ftFace);
    }
    if (visualState->ftLibrary) {
        FT_Done_FreeType(visualState->ftLibrary);
    }
}