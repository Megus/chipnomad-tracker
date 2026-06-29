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

// Add current position to track history
static void addToHistory(TrackHistory* history, ScrollPosition pos) {
  history->positions[history->writeIndex] = pos;
  history->writeIndex = (history->writeIndex + 1) % MAX_HISTORY_SIZE;
  if (history->count < MAX_HISTORY_SIZE) {
    history->count++;
  }
}

// Get position from history (0 = most recent, 1 = one before, etc.)
static ScrollPosition getFromHistory(TrackHistory* history, int stepsBack) {
  if (stepsBack >= history->count) {
    ScrollPosition invalid = {0, 0, 0, 0};
    return invalid;
  }

  int index = (history->writeIndex - 1 - stepsBack + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
  return history->positions[index];
}

// Navigate forward by one row, respecting groove (skipping rows with groove value 0)
static ScrollPosition scrollPositionNextWithGroove(Project* project, PlaybackTrackState* track, int trackIdx, ScrollPosition pos, int* grooveRow) {
  ScrollPosition next = pos;

  if (!next.valid) return next;

  // Get current groove
  Groove* groove = &project->grooves[track->grooveIdx];

  // Keep advancing until we find a non-skipped row or reach end
  while (1) {
    next.phraseRow++;
    (*grooveRow)++;

    // Check if groove loops
    if (*grooveRow >= 16 || groove->speed[*grooveRow] == EMPTY_VALUE_8) {
      *grooveRow = 0;
    }

    if (next.phraseRow >= 16) {
      next.phraseRow = 0;
      next.chainRow++;

      // Check if we need to move to next song row
      while (next.chainRow < 16) {
        uint16_t chainIdx = project->song[next.songRow][trackIdx];
        if (chainIdx == EMPTY_VALUE_16) {
          next.valid = 0;
          return next;
        }

        uint16_t phraseIdx = project->chains[chainIdx].rows[next.chainRow].phrase;
        if (phraseIdx != EMPTY_VALUE_16) {
          break; // Found valid phrase
        }

        // Empty chain slot, move to next song row
        next.chainRow = 0;
        next.songRow++;

        if (next.songRow >= PROJECT_MAX_LENGTH) {
          next.valid = 0;
          return next;
        }
      }

      if (next.chainRow >= 16) {
        next.chainRow = 0;
        next.songRow++;

        if (next.songRow >= PROJECT_MAX_LENGTH) {
          next.valid = 0;
          return next;
        }

        // Check if new song position is valid
        uint16_t chainIdx = project->song[next.songRow][trackIdx];
        if (chainIdx == EMPTY_VALUE_16) {
          next.valid = 0;
          return next;
        }
      }
    }

    // Check if this row should be skipped (groove value is 0)
    if (groove->speed[*grooveRow] != 0) {
      break; // Found a non-skipped row
    }

    // If groove value is 0, continue loop to skip this row
  }

  return next;
}

// Navigate forward by one row in the song structure
static ScrollPosition scrollPositionNext(Project* project, int trackIdx, ScrollPosition pos) {
  ScrollPosition next = pos;

  if (!next.valid) return next;

  next.phraseRow++;
  if (next.phraseRow >= 16) {
    next.phraseRow = 0;
    next.chainRow++;

    // Check if we need to move to next song row
    while (next.chainRow < 16) {
      uint16_t chainIdx = project->song[next.songRow][trackIdx];
      if (chainIdx == EMPTY_VALUE_16) {
        next.valid = 0;
        return next;
      }

      uint16_t phraseIdx = project->chains[chainIdx].rows[next.chainRow].phrase;
      if (phraseIdx != EMPTY_VALUE_16) {
        break; // Found valid phrase
      }

      // Empty chain slot, move to next song row
      next.chainRow = 0;
      next.songRow++;

      if (next.songRow >= PROJECT_MAX_LENGTH) {
        next.valid = 0;
        return next;
      }
    }

    if (next.chainRow >= 16) {
      next.chainRow = 0;
      next.songRow++;

      if (next.songRow >= PROJECT_MAX_LENGTH) {
        next.valid = 0;
        return next;
      }

      // Check if new song position is valid
      uint16_t chainIdx = project->song[next.songRow][trackIdx];
      if (chainIdx == EMPTY_VALUE_16) {
        next.valid = 0;
        return next;
      }
    }
  }

  return next;
}

// Navigate backward by one row in the song structure
static ScrollPosition scrollPositionPrev(Project* project, int trackIdx, ScrollPosition pos) {
  ScrollPosition prev = pos;

  if (!prev.valid) return prev;

  prev.phraseRow--;
  if (prev.phraseRow < 0) {
    prev.phraseRow = 15;
    prev.chainRow--;

    // Check if we need to move to previous song row
    while (prev.chainRow < 0) {
      prev.songRow--;

      if (prev.songRow < 0) {
        prev.valid = 0;
        return prev;
      }

      uint16_t chainIdx = project->song[prev.songRow][trackIdx];
      if (chainIdx == EMPTY_VALUE_16) {
        prev.valid = 0;
        return prev;
      }

      // Find last valid chain row in this song position
      prev.chainRow = 15;
      while (prev.chainRow >= 0) {
        uint16_t phraseIdx = project->chains[chainIdx].rows[prev.chainRow].phrase;
        if (phraseIdx != EMPTY_VALUE_16) {
          break;
        }
        prev.chainRow--;
      }

      if (prev.chainRow < 0) {
        // No valid phrases in this chain, continue to previous song row
        continue;
      }
      break;
    }
  }

  return prev;
}

// Get phrase row at a scroll position (returns NULL if invalid)
static PhraseRow* getScrollPhraseRow(Project* project, int trackIdx, ScrollPosition pos) {
  if (!pos.valid) return NULL;
  if (pos.songRow < 0 || pos.songRow >= PROJECT_MAX_LENGTH) return NULL;

  uint16_t chainIdx = project->song[pos.songRow][trackIdx];
  if (chainIdx == EMPTY_VALUE_16) return NULL;

  if (pos.chainRow < 0 || pos.chainRow >= 16) return NULL;

  uint16_t phraseIdx = project->chains[chainIdx].rows[pos.chainRow].phrase;
  if (phraseIdx == EMPTY_VALUE_16) return NULL;

  if (pos.phraseRow < 0 || pos.phraseRow >= 16) return NULL;

  return &project->phrases[phraseIdx].rows[pos.phraseRow];
}

static void renderText(VisualState* visualState, const char* text, int x, int y, SDL_Color color);

// Render a single phrase row at given position
static void renderPhraseRow(VisualState* visualState, PhraseRow* phraseRow, int x, int y, int charWidth) {
  int col = x;

  // In compact mode, check if we should show FX instead of note/instrument
  int showFxInNoteColumn = 0;
  int fxToShow = -1;
  if (visualState->config->phraseRenderMode == PHRASE_RENDER_MODE_COMPACT) {
    // Only show FX if note is empty
    if (phraseRow->note == EMPTY_VALUE_8) {
      // Find leftmost non-empty FX
      for (int fx = 0; fx < 3; fx++) {
        if (phraseRow->fx[fx][0] != EMPTY_VALUE_8) {
          fxToShow = fx;
          showFxInNoteColumn = 1;
          break;
        }
      }
    }
  }

  // Note column (3 chars)
  if (showFxInNoteColumn && fxToShow >= 0) {
    // Show FX name in note column (dimmed by 50%)
    SDL_Color dimmedFxColor = {
      static_cast<Uint8>(visualState->config->fxColor.r / 2),
      static_cast<Uint8>(visualState->config->fxColor.g / 2),
      static_cast<Uint8>(visualState->config->fxColor.b / 2),
      255
    };
    char fxNameStr[5];
    snprintf(fxNameStr, sizeof(fxNameStr), "%s ", fxNames[phraseRow->fx[fxToShow][0]].name);
    renderText(visualState, fxNameStr, col, y, dimmedFxColor);
  } else {
    const char* noteStr = noteName(visualState->project, phraseRow->note);
    if (strcmp(noteStr, "---") != 0) {
      renderText(visualState, noteStr, col, y, visualState->config->noteColor);
    } else {
      renderText(visualState, "---", col, y, visualState->config->dimmedColor);
    }
  }
  col += charWidth * 4; // 3 chars + space

  // Instrument column (2 chars)
  if (showFxInNoteColumn && fxToShow >= 0) {
    // Show FX value in instrument column (dimmed by 50%)
    SDL_Color dimmedFxColor = {
      static_cast<Uint8>(visualState->config->fxColor.r / 2),
      static_cast<Uint8>(visualState->config->fxColor.g / 2),
      static_cast<Uint8>(visualState->config->fxColor.b / 2),
      255
    };
    char fxValStr[3];
    snprintf(fxValStr, sizeof(fxValStr), "%02X", phraseRow->fx[fxToShow][1]);
    renderText(visualState, fxValStr, col, y, dimmedFxColor);
  } else {
    if (phraseRow->instrument != EMPTY_VALUE_8) {
      char instStr[3];
      snprintf(instStr, sizeof(instStr), "%02X", phraseRow->instrument);
      renderText(visualState, instStr, col, y, visualState->config->instrumentColor);
    } else {
      renderText(visualState, "--", col, y, visualState->config->dimmedColor);
    }
  }
  col += charWidth * 3; // 2 chars + space

  // Volume column (2 chars)
  if (phraseRow->volume != EMPTY_VALUE_8) {
    char volStr[3];
    snprintf(volStr, sizeof(volStr), "%02X", phraseRow->volume);
    renderText(visualState, volStr, col, y, visualState->config->volumeColor);
  } else {
    renderText(visualState, "--", col, y, visualState->config->dimmedColor);
  }
  col += charWidth * 3; // 2 chars + space

  // FX columns
  if (visualState->config->phraseRenderMode == PHRASE_RENDER_MODE_FULL) {
    // Full mode: show all 3 FX columns
    for (int fx = 0; fx < 3; fx++) {
      if (phraseRow->fx[fx][0] != EMPTY_VALUE_8) {
        char fxStr[7];
        snprintf(fxStr, sizeof(fxStr), "%s%02X", fxNames[phraseRow->fx[fx][0]].name, phraseRow->fx[fx][1]);
        renderText(visualState, fxStr, col, y, visualState->config->fxColor);
      } else {
        renderText(visualState, "-----", col, y, visualState->config->dimmedColor);
      }
      col += charWidth * (fx < 2 ? 6 : 5); // 5 chars + space (except last)
    }
  } else if (visualState->config->phraseRenderMode == PHRASE_RENDER_MODE_MEDIUM) {
    // Medium mode: show 1 FX column (leftmost non-empty)
    int fxToShow = -1;
    for (int fx = 0; fx < 3; fx++) {
      if (phraseRow->fx[fx][0] != EMPTY_VALUE_8) {
        fxToShow = fx;
        break;
      }
    }

    if (fxToShow >= 0) {
      char fxStr[7];
      snprintf(fxStr, sizeof(fxStr), "%s%02X", fxNames[phraseRow->fx[fxToShow][0]].name, phraseRow->fx[fxToShow][1]);
      renderText(visualState, fxStr, col, y, visualState->config->fxColor);
    } else {
      renderText(visualState, "-----", col, y, visualState->config->dimmedColor);
    }
  }
  // Compact mode: no FX columns shown
}

static void renderText(VisualState* visualState, const char* text, int x, int y, SDL_Color color) {
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

int visualsInit(VisualState* visualState) {
  // Initialize track history buffers
  for (int i = 0; i < PROJECT_MAX_TRACKS; i++) {
    visualState->trackHistory[i].count = 0;
    visualState->trackHistory[i].writeIndex = 0;
  }

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

void visualsRender(VisualState* visualState) {
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

    // Calculate track content width based on render mode
    int trackContentWidth;
    if (visualState->config->phraseRenderMode == PHRASE_RENDER_MODE_COMPACT) {
      // Compact mode: note + inst + vol (no FX columns)
      trackContentWidth = charWidth * (3 + 1 + 2 + 1 + 2); // 9 chars
    } else if (visualState->config->phraseRenderMode == PHRASE_RENDER_MODE_MEDIUM) {
      // Medium mode: note + inst + vol + 1 fx column
      trackContentWidth = charWidth * (3 + 1 + 2 + 1 + 2 + 1 + 5); // 15 chars
    } else {
      // Full mode: note + inst + vol + 3 fx columns
      trackContentWidth = charWidth * (3 + 1 + 2 + 1 + 2 + 1 + 5 + 1 + 5 + 1 + 5); // 27 chars
    }
    int trackSpacing = charWidth * 3; // 3 characters between tracks

    // Calculate total width and center horizontally
    int totalWidth = trackCount * trackContentWidth + (trackCount - 1) * trackSpacing;
    int startX = (visualState->config->windowWidth - totalWidth) / 2;

    if (visualState->config->playerMode == PLAYER_MODE_PHRASE) {
      // PHRASE MODE: Show current phrase for each track

      // Center content vertically with slight downward adjustment
      int totalHeight = 16 * lineHeight;
      int startY = (visualState->config->windowHeight - totalHeight) / 2 + fontSize / 2;

      for (int trackIdx = 0; trackIdx < trackCount; trackIdx++) {
        PlaybackTrackState* track = &visualState->playback->tracks[trackIdx];

        if (track->mode == playbackModeStopped) continue;

        // Get current phrase
        uint16_t chainIdx = visualState->project->song[track->songRow][trackIdx];
        if (chainIdx == EMPTY_VALUE_16) continue;

        uint16_t phraseIdx = visualState->project->chains[chainIdx].rows[track->chainRow].phrase;
        if (phraseIdx == EMPTY_VALUE_16) continue;

        Phrase* phrase = &visualState->project->phrases[phraseIdx];

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

          PhraseRow* phraseRow = &phrase->rows[row];
          renderPhraseRow(visualState, phraseRow, x, y, charWidth);
        }
      }
    } else {
      // SCROLL MODE: Show scrolling view for each track

      int scrollRows = visualState->config->scrollRows;
      int centerRow = scrollRows / 2;

      // Center content vertically
      int totalHeight = scrollRows * lineHeight;
      int startY = (visualState->config->windowHeight - totalHeight) / 2 + fontSize / 2;

      for (int trackIdx = 0; trackIdx < trackCount; trackIdx++) {
        PlaybackTrackState* track = &visualState->playback->tracks[trackIdx];

        if (track->mode == playbackModeStopped) continue;

        // Calculate track position
        int trackStartX = startX + trackIdx * (trackContentWidth + trackSpacing);

        // Current track position
        ScrollPosition currentPos = {
          .songRow = track->songRow,
          .chainRow = track->chainRow,
          .phraseRow = track->phraseRow,
          .valid = 1
        };

        // Update history with current position
        TrackHistory* history = &visualState->trackHistory[trackIdx];

        // Check if this is a new position (not same as last in history)
        int isNewPosition = 1;
        if (history->count > 0) {
          ScrollPosition lastPos = getFromHistory(history, 0);
          if (lastPos.songRow == currentPos.songRow &&
              lastPos.chainRow == currentPos.chainRow &&
              lastPos.phraseRow == currentPos.phraseRow) {
            isNewPosition = 0;
          }
        }

        if (isNewPosition) {
          addToHistory(history, currentPos);
        }

        // Render rows above center (from history)
        for (int row = centerRow - 1; row >= 0; row--) {
          int stepsBack = centerRow - row;
          ScrollPosition pos = getFromHistory(history, stepsBack);

          int x = trackStartX;
          int y = startY + row * lineHeight;

          // Get and render phrase row
          PhraseRow* phraseRow = getScrollPhraseRow(visualState->project, trackIdx, pos);
          if (phraseRow) {
            renderPhraseRow(visualState, phraseRow, x, y, charWidth);
          }
          // If phraseRow is NULL, row remains empty (out of bounds)
        }

        // Render center row (current position)
        {
          int x = trackStartX;
          int y = startY + centerRow * lineHeight;

          renderText(visualState, ">", x - charWidth, y, visualState->config->currentRowColor);

          PhraseRow* phraseRow = getScrollPhraseRow(visualState->project, trackIdx, currentPos);
          if (phraseRow) {
            renderPhraseRow(visualState, phraseRow, x, y, charWidth);
          }
        }

        // Render rows below center (going forwards with groove awareness)
        ScrollPosition pos = currentPos;
        int grooveRow = track->grooveRow;
        for (int row = centerRow + 1; row < scrollRows; row++) {
          pos = scrollPositionNextWithGroove(visualState->project, track, trackIdx, pos, &grooveRow);

          int x = trackStartX;
          int y = startY + row * lineHeight;

          // Get and render phrase row
          PhraseRow* phraseRow = getScrollPhraseRow(visualState->project, trackIdx, pos);
          if (phraseRow) {
            renderPhraseRow(visualState, phraseRow, x, y, charWidth);
          }
          // If phraseRow is NULL, row remains empty (out of bounds)
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

void visualsCleanup(VisualState* visualState) {
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