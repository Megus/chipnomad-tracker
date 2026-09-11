#include "playback.h"
#include "chipnomad_lib.h"
#include <stdio.h>
#include <string.h>

namespace chipnomad {

  ///////////////////////////////////////////////////////////////////////////////
  //
  // Common logic
  //

  static int moveToNextPhraseRow(Player* player, int trackIdx);

  static void resetTrackFXAuxState(Player* player, int trackIdx) {
    PlaybackTrackState* track = &player->tracks[trackIdx];
    memset(track->fxAuxState, 0, sizeof(track->fxAuxState));
  }

  static void resetTableFXAuxState(PlaybackTableState* tableState) {
    memset(tableState->fxAuxState, 0, sizeof(tableState->fxAuxState));
  }

  static void resetNoteFX(Player* player, int trackIdx) {
    PlaybackTrackState* track = &player->tracks[trackIdx];
    for (int i = 0; i < fxTotalCount; i++) {
      track->note.fx[i].isOn = 0;
      track->note.fx[i].counter = 0;
      track->note.fx[i].acc = 0;
    }
  }

  static void resetTrack(Player* player, int trackIdx, bool resetGroove) {
    PlaybackTrackState* track = &player->tracks[trackIdx];

    // Set specific values that shouldn't be zero
    track->songRow = EMPTY_VALUE_16;
    track->chainRow = 0;
    track->phraseRow = 0;
    track->frameCounter = 0;
    if (resetGroove) {
      track->grooveIdx = 0;
      track->pendingGrooveIdx = 0;
    }
    track->grooveRow = 0;

    track->note.pitchBase = EMPTY_VALUE_8;
    track->note.pitchFinal = EMPTY_VALUE_8;
    track->note.pitchOffset = 0;
    track->note.fineOffset = 0;
    track->note.instrument = EMPTY_VALUE_8;
    track->note.volume = 0;
    track->note.volume1 = 0;
    track->note.volume2 = 0;
    track->note.volume3 = 0;

    track->note.instrument = EMPTY_VALUE_8;
    track->note.instrumentTable.tableIdx = EMPTY_VALUE_8;
    track->note.auxTable.tableIdx = EMPTY_VALUE_8;
    track->mode = PlaybackMode::stopped;

    resetNoteFX(player, trackIdx);

    resetTrackFXAuxState(player, trackIdx);
    resetTableFXAuxState(&track->note.instrumentTable);
    resetTableFXAuxState(&track->note.auxTable);

    // Clear cached phrase row
    memset(&track->currentPhraseRow, EMPTY_VALUE_8, sizeof(track->currentPhraseRow));

    player->resetTrackAY(trackIdx);
  }

  void Player::tableInit(int trackIdx, struct PlaybackTableState* table, int tableIdx, int row, int speed) {
    table->tableIdx = tableIdx;
    if (tableIdx == EMPTY_VALUE_8) return;

    resetTableFXAuxState(table);

    for (int i = 0; i < 4; i++) {
      table->counters[i] = 0;
      table->rows[i] = row;
      table->speed[i] = speed;

      // Check if row 15 has TIC effect for this column
      if (p->tables[tableIdx].rows[15].fx[i][0] == fxTIC) {
        table->speed[i] = p->tables[tableIdx].rows[15].fx[i][1];
      }

      tableReadFX(trackIdx, table, i);
    }
  }

  void Player::hopToTableRow(int trackIdx, PlaybackTableState* table, int tableRow) {
    for (int c = 0; c < 4; c++) {
      table->counters[c] = 0;
      table->rows[c] = tableRow;
      tableReadFX(trackIdx, table, c);
    }
  }

  void Player::tableReadFX(int trackIdx, struct PlaybackTableState* table, int fxIdx) {
    uint8_t tableIdx = table->tableIdx;
    if (tableIdx == EMPTY_VALUE_8) return;

    int tableRow = table->rows[fxIdx];
    uint8_t fxType = p->tables[tableIdx].rows[tableRow].fx[fxIdx][0];
    if (fxType == fxTBL) {
      tableInit(trackIdx, &tracks[trackIdx].note.instrumentTable, p->tables[tableIdx].rows[tableRow].fx[fxIdx][1], 0, 1);
    } else if (fxType == fxTBX) {
      tableInit(trackIdx, &tracks[trackIdx].note.auxTable, p->tables[tableIdx].rows[tableRow].fx[fxIdx][1], 0, 1);
    } else {
      initFX(trackIdx, p->tables[tableIdx].rows[tableRow].fx[fxIdx], table, fxIdx);
    }
  }

  static void tableProgress(Player* player, int trackIdx, struct PlaybackTableState* table) {
    if (table->tableIdx == EMPTY_VALUE_8) return;
    Project* p = player->p;

    for (int i = 0; i < 4; i++) {
      table->counters[i]++;

      if (table->counters[i] >= table->speed[i]) {
        table->counters[i] = 0;
        uint8_t row = table->rows[i];

        // Check if any column has THO on current row
        int thoTarget = -1;
        for (int col = 0; col < 4; col++) {
          if (p->tables[table->tableIdx].rows[row].fx[col][0] == fxTHO) {
            thoTarget = p->tables[table->tableIdx].rows[row].fx[col][1] & 0xf;
            break;
          }
        }

        if (thoTarget >= 0 && thoTarget == row) {
          // THO pointing to same row - stay here
          player->tableReadFX(trackIdx, table, i);
          continue;
        }

        // Check HOP on current row pointing to same row
        uint8_t fxType = p->tables[table->tableIdx].rows[row].fx[i][0];
        uint8_t fxValue = p->tables[table->tableIdx].rows[row].fx[i][1];

        if (fxType == fxHOP && (fxValue & 0xf) == row) {
          if (fxValue & 0xf0) {
            // Loop counter
            table->fxAuxState[row][i]++;
            if (table->fxAuxState[row][i] <= ((fxValue & 0xf0) >> 4)) {
              player->tableReadFX(trackIdx, table, i);
              continue;
            }
          } else {
            // Unconditional hop to same row - stay here
            player->tableReadFX(trackIdx, table, i);
            continue;
          }
        }

        // Progress to next row
        table->rows[i] = (row + 1) & 15;

        if (table->rows[i] == 0) {
          // Reset all loop counters for this column
          for (int c = 0; c < 16; c++) {
            table->fxAuxState[c][i] = 0;
          }
        }

        row = table->rows[i];

        // Check if any column has THO on new row
        thoTarget = -1;
        for (int col = 0; col < 4; col++) {
          if (p->tables[table->tableIdx].rows[row].fx[col][0] == fxTHO) {
            thoTarget = p->tables[table->tableIdx].rows[row].fx[col][1] & 0xf;
            break;
          }
        }

        if (thoTarget >= 0) {
          // THO found - hop this column
          table->rows[i] = thoTarget;
          player->tableReadFX(trackIdx, table, i);
          continue;
        }

        // Check HOP on new row
        fxType = p->tables[table->tableIdx].rows[row].fx[i][0];
        fxValue = p->tables[table->tableIdx].rows[row].fx[i][1];

        if (fxType == fxHOP) {
          uint8_t hopTarget = fxValue & 0xf;
          if (fxValue & 0xf0) {
            // Loop counter
            table->fxAuxState[row][i]++;
            if (table->fxAuxState[row][i] <= ((fxValue & 0xf0) >> 4)) {
              // Reset "nested" loops when hopping back
              if (hopTarget < row) {
                for (int c = hopTarget; c < row; c++) {
                  table->fxAuxState[c][i] = 0;
                }
              }
              table->rows[i] = hopTarget;
              player->tableReadFX(trackIdx, table, i);
              continue;
            }
          } else {
            // Unconditional hop
            table->rows[i] = hopTarget;
            player->tableReadFX(trackIdx, table, i);
            continue;
          }
        }

        // No hop - read FX from current row
        player->tableReadFX(trackIdx, table, i);
      }
    }
  }

  void Player::handleNoteOff(int trackIdx) {
    PlaybackTrackState* track = &tracks[trackIdx];

    if (track->note.instrument == EMPTY_VALUE_8) return;

    InstrumentType instType = p->instruments[track->note.instrument].type;

    if (instType == InstrumentType::none) return;

    int hasVolumeADSR = 0;

    if (instType == InstrumentType::AY1) {
      // Handle legacy AY1 volume modulation
      playbackModNoteOff(&track->note.chip.ay.volumeModulation);
      hasVolumeADSR = 1;
    }

    // Common note off for all instruments - send note off to all modulations
    for (int i = 0; i < 4; i++) {
      if (track->note.modulation[i].modulation->destination == 1 && track->note.modulation[i].modulation->type == ModulationType::ADSR) {
        hasVolumeADSR = 1;
      }
      playbackModNoteOff(&track->note.modulation[i]);
    }

    if (!hasVolumeADSR) {
      // If no volume ADSR, turn off note immediately
      track->note.pitchBase = EMPTY_VALUE_8;
    }
  }

  static void initModulations(Player* player, int trackIdx, uint8_t oldInstrument, uint8_t newInstrument) {
    PlaybackTrackState* track = &player->tracks[trackIdx];
    Project* p = player->p;

    if (newInstrument == EMPTY_VALUE_8) return;

    const Modulation* mods = p->instruments[newInstrument].modulation;

    int instrumentChanged = (oldInstrument != newInstrument);

    // Initialize all modulation slots
    for (int i = 0; i < 4; i++) {
      const Modulation* mod = &mods[i];

      // Check if this is an LFO with "free" trigger mode
      // LFO parameters: p1=shape, p2=trigger, p3=period
      int isLFOFree = (mod->type == ModulationType::LFO && mod->p2 == static_cast<uint8_t>(LFOTrigger::free));

      // Initialize modulation if:
      // 1. Instrument changed (always reinit), OR
      // 2. Not an LFO with free trigger mode (retrig/hold/once always reinit)
      if (instrumentChanged || !isLFOFree) {
        playbackModInit(&track->note.modulation[i], (Modulation*)mod);
      }
    }
  }

  void Player::readPhraseRowDirect(int trackIdx, PhraseRow* phraseRow, int skipDelCheck) {
    PlaybackTrackState* track = &tracks[trackIdx];

    uint8_t note = phraseRow->note;
    uint8_t instrument = phraseRow->instrument;
    uint8_t volume = phraseRow->volume;

    uint8_t auxTable = EMPTY_VALUE_8;
    uint8_t auxTableRow = EMPTY_VALUE_8;
    uint8_t instrumentTable = EMPTY_VALUE_8;
    uint8_t instrumentTableRow = EMPTY_VALUE_8;

    // Check for pending groove change
    if (track->pendingGrooveIdx != track->grooveIdx) {
      track->grooveIdx = track->pendingGrooveIdx;
      track->grooveRow = 0;
      track->frameCounter = 0;
    }

    // Pre-scan FX for special commands
    for (int i = 0; i < 3; i++) {
      uint8_t fxType = phraseRow->fx[i][0];
      uint8_t fxValue = phraseRow->fx[i][1];

      if (!skipDelCheck && (fxType == fxDEL && fxValue != 0)) {
        initFX(trackIdx, phraseRow->fx[i], NULL, -1);
        return;
      } else if (fxType == fxTBL) {
        instrumentTable = fxValue;
      } else if (fxType == fxTBX) {
        auxTable = fxValue;
      } else if (fxType == fxTHO) {
        instrumentTableRow = fxValue & 0xf;
      } else if (fxType == fxTXH) {
        auxTableRow = fxValue & 0xf;
      }
    }

    if (instrumentTable != EMPTY_VALUE_8 && instrumentTableRow == EMPTY_VALUE_8) instrumentTableRow = 0;
    if (auxTable != EMPTY_VALUE_8 && auxTableRow == EMPTY_VALUE_8) auxTableRow = 0;

    // Instrument
    if (instrument != EMPTY_VALUE_8) {
      uint8_t oldInstrument = track->note.instrument;
      track->note.instrument = instrument;

      // Turn off all FX on a new instrument
      resetNoteFX(this, trackIdx);
      resetOffsets(trackIdx);

      // Reset AUX table
      tableInit(trackIdx, &track->note.auxTable, EMPTY_VALUE_8, 0, 1);

      // Initialize modulations
      initModulations(this, trackIdx, oldInstrument, instrument);

      // Setup instrument
      setupInstrument(trackIdx);
      if (instrumentTable == EMPTY_VALUE_8) {
        instrumentTable = instrument;
        if (instrumentTableRow == EMPTY_VALUE_8) {
          instrumentTableRow = 0;
        }
      }
    }

    if (instrument == EMPTY_VALUE_8 && (note != EMPTY_VALUE_8 && note != NOTE_OFF)) {
      restartFX(trackIdx);
    }

    // Init/hop tables
    if (instrumentTable != EMPTY_VALUE_8) {
      tableInit(trackIdx, &track->note.instrumentTable, instrumentTable, instrumentTableRow, p->instruments[instrument].tableSpeed);
    } else if (instrumentTableRow != EMPTY_VALUE_8) {
      hopToTableRow(trackIdx, &track->note.instrumentTable, instrumentTableRow);
    }

    if (auxTable != EMPTY_VALUE_8) {
      tableInit(trackIdx, &track->note.auxTable, auxTable, auxTableRow, 1);
    } else if (auxTableRow != EMPTY_VALUE_8) {
      hopToTableRow(trackIdx, &track->note.auxTable, auxTableRow);
    }

    // Read new FX
    for (int i = 0; i < 3; i++) {
      if (phraseRow->fx[i][0] != fxDEL) {
        initFX(trackIdx, phraseRow->fx[i], NULL, -1);
      }
    }

    // Note
    if (note != EMPTY_VALUE_8) {
      if (note == NOTE_OFF) {
        handleNoteOff(trackIdx);
      } else {
        track->note.pitchBase = note;
      }
    }

    // Apply chain transpose (if the instrument allows it)
    if (note != EMPTY_VALUE_8 && note != NOTE_OFF && track->mode != PlaybackMode::phraseRow) {
      uint16_t chainIdx = p->song[track->songRow][trackIdx];
      if (chainIdx != EMPTY_VALUE_16) {
        int8_t transpose = p->chains[chainIdx].rows[track->chainRow].transpose;
        if (p->instruments[track->note.instrument].transposeEnabled) {
          track->note.pitchBase += transpose;
        }
      }
    }

    // Volume
    if (volume != EMPTY_VALUE_8) {
      track->note.volume = volume;
    }
  }

  void Player::readPhraseRow(int trackIdx, int skipDelCheck) {
    PlaybackTrackState* track = &tracks[trackIdx];

    // If using phrase row mode, use cached phrase row
    if (track->mode == PlaybackMode::phraseRow) {
      readPhraseRowDirect(trackIdx, &track->currentPhraseRow, skipDelCheck);
      return;
    }

    // If nothing is playing, skip it
    if (track->mode == PlaybackMode::stopped || track->songRow == EMPTY_VALUE_16) return;

    uint16_t chainIdx = p->song[track->songRow][trackIdx];
    if (chainIdx != EMPTY_VALUE_16) {
      uint16_t phraseIdx = p->chains[chainIdx].rows[track->chainRow].phrase;
      if (phraseIdx != EMPTY_VALUE_16) {
        int phraseRow = track->phraseRow;
        Phrase* phrase = &p->phrases[phraseIdx];
        PhraseRow* currentRow = &phrase->rows[phraseRow];

        // Check for SNG command in Song mode
        if (track->mode == PlaybackMode::song) {
          for (int i = 0; i < 3; i++) {
            if (currentRow->fx[i][0] == fxSNG && currentRow->fx[i][1] != 0) {
              int8_t offset = (int8_t)currentRow->fx[i][1];
              int newSongRow = track->songRow + offset;

              // Check if jump is negative and loop is disabled
              if (offset < 0 && !track->loop) {
                resetTrack(this, trackIdx, false);
                return;
              }

              // Validate target song position
              if (newSongRow >= 0 && newSongRow < PROJECT_MAX_LENGTH) {
                uint16_t targetChainIdx = p->song[newSongRow][trackIdx];
                if (targetChainIdx != EMPTY_VALUE_16) {
                  uint16_t targetPhraseIdx = p->chains[targetChainIdx].rows[0].phrase;
                  if (targetPhraseIdx != EMPTY_VALUE_16) {
                    // Valid target, perform jump and read from new position
                    track->songRow = newSongRow;
                    track->chainRow = 0;
                    track->phraseRow = 0;
                    resetTrackFXAuxState(this, trackIdx);
                    readPhraseRow(trackIdx, skipDelCheck);
                    return;
                  }
                }
              }
              // Invalid target or out of bounds, ignore SNG command and continue normally
              break;
            }
          }
        }

        // Check for HOP command
        for (int i = 0; i < 3; i++) {
          if (currentRow->fx[i][0] == fxHOP) {
            uint8_t hopValue = currentRow->fx[i][1];

            // 0xFF = stop track
            if (hopValue == 0xFF) {
              resetTrack(this, trackIdx, false);
              return;
            }

            uint8_t targetRow = hopValue & 0x0F;
            uint8_t loopCount = (hopValue & 0xF0) >> 4;

            if (loopCount == 0) {
              // Unconditional jump to next phrase
              track->phraseRow = 15;
              if (moveToNextPhraseRow(this, trackIdx)) {
                return;
              }
              track->phraseRow = targetRow;
              resetTrackFXAuxState(this, trackIdx);
              readPhraseRow(trackIdx, skipDelCheck);
              return;
            } else {
              // Conditional jump with loop counter
              track->fxAuxState[phraseRow][i]++;
              if (track->fxAuxState[phraseRow][i] <= loopCount) {
                // Reset nested loop counters when hopping backwards
                if (targetRow < phraseRow) {
                  for (int c = targetRow; c < phraseRow; c++) {
                    track->fxAuxState[c][i] = 0;
                  }
                }
                track->phraseRow = targetRow;
                currentRow = &phrase->rows[targetRow];
              }
            }
            break;
          }
        }

        readPhraseRowDirect(trackIdx, currentRow, skipDelCheck);
      } else {
        // Safeguard for phrase in chain
        resetTrack(this, trackIdx, false);
      }
    } else {
      // Safeguard for chain in song
      resetTrack(this, trackIdx, false);
    }
  }

  void Player::resetOffsets(int trackIdx) {
    PlaybackTrackState* track = &tracks[trackIdx];
    track->note.pitchOffset = 0;
    track->note.fineOffset = 0;
    track->note.periodOffset = 0;
    track->note.volumeOffset = 0;

    // Reset modulation offsets
    for (int i = 0; i < 4; i++) {
      track->note.modulation[i].amountOffset = 0;
      track->note.modulation[i].p1Offset = 0;
      track->note.modulation[i].p2Offset = 0;
      track->note.modulation[i].p3Offset = 0;
      track->note.modulation[i].p4Offset = 0;
    }

    // Dispatch to chip-specific offset reset based on instrument type
    if (track->note.instrument != EMPTY_VALUE_8) {
      InstrumentType instType = p->instruments[track->note.instrument].type;
      switch (instType) {
        case InstrumentType::AY1:
        case InstrumentType::AY2:
        case InstrumentType::AYSample:
          resetOffsetsAY(trackIdx);
          break;
        default:
          break;
      }
    }
  }

  static void processModulations(Player* player, int trackIdx) {
    PlaybackTrackState* track = &player->tracks[trackIdx];

    if (track->note.instrument == EMPTY_VALUE_8) return;

    for (int i = 0; i < 4; i++) {
      PlaybackModState* mod = &track->note.modulation[i];
      // Skip if modulation not initialized or destination == 0 (no destination)
      if (!mod->modulation || mod->modulation->destination == 0) continue;
      playbackModNext(mod);
    }
  }

  static void handleInstrument(Player* player, int trackIdx) {
    PlaybackTrackState* track = &player->tracks[trackIdx];
    Project* p = player->p;

    if (track->note.instrument == EMPTY_VALUE_8) return;
    if (track->note.pitchBase == EMPTY_VALUE_8) return;

    InstrumentType instType = p->instruments[track->note.instrument].type;
    switch (instType) {
    case InstrumentType::AY1:
      player->handleInstrumentAY1(trackIdx);
      break;
    case InstrumentType::AY2:
      player->handleInstrumentAY2(trackIdx);
      break;
    case InstrumentType::AYSample:
      player->handleInstrumentAYSample(trackIdx);
      break;
    case InstrumentType::none:
      break;
    }
  }

  static void processTrackFrame(Player* player, int trackIdx, int chipIdx) {
    PlaybackTrackState* track = &player->tracks[trackIdx];
    Project* p = player->p;

    // Is the channel playing?
    if (track->songRow == EMPTY_VALUE_16) {
      track->note.pitchFinal = EMPTY_VALUE_8;
      return;
    }

    player->resetOffsets(trackIdx);
    player->handleFX(trackIdx, chipIdx);
    processModulations(player, trackIdx);
    handleInstrument(player, trackIdx);

    // Final pitch calculation
    if (track->note.pitchBase == EMPTY_VALUE_8) {
      track->note.pitchFinal = EMPTY_VALUE_8;
    } else {
      // Base pitch
      int16_t pitch = track->note.pitchBase;

      // Tables
      int tableIdx = track->note.instrumentTable.tableIdx;
      if (tableIdx != EMPTY_VALUE_8) {
        if (p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].pitchFlag) {
          pitch = p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].pitchOffset;
        } else {
          pitch += (int8_t)(p->tables[tableIdx].rows[track->note.instrumentTable.rows[0]].pitchOffset);
        }
      }

      tableIdx = track->note.auxTable.tableIdx;
      if (tableIdx != EMPTY_VALUE_8) {
        if (p->tables[tableIdx].rows[track->note.auxTable.rows[0]].pitchFlag) {
          pitch = p->tables[tableIdx].rows[track->note.auxTable.rows[0]].pitchOffset;
        } else {
          pitch += (int8_t)(p->tables[tableIdx].rows[track->note.auxTable.rows[0]].pitchOffset);
        }
      }

      // Offset from FX
      pitch += track->note.pitchOffset;

      // Clamp pitch to valid range
      pitch = clampInt16(pitch, 0, p->pitchTable.length - 1);

      track->note.pitchFinal = pitch;
    }
  }

  static int moveToNextPhraseRow(Player* player, int trackIdx) {
    int stopped = 0;
    struct Project *p = player->p;
    PlaybackTrackState* track = &player->tracks[trackIdx];

    // Check phrase-level loop before incrementing
    if (player->loopRange.enabled && player->loopRange.level == 2 && track->loop &&
        track->songRow == player->loopRange.endSongRow &&
        track->chainRow == player->loopRange.endChainRow &&
        track->phraseRow == player->loopRange.endPhraseRow) {
      track->phraseRow = player->loopRange.startPhraseRow;
      resetTrackFXAuxState(player, trackIdx);
      return stopped;
    }

    track->phraseRow++;

    if (track->phraseRow >= 16) {
      track->phraseRow = 0;

      // Check chain-level loop after phrase overflow
      if (player->loopRange.enabled && player->loopRange.level == 1 && track->loop &&
          track->songRow == player->loopRange.endSongRow &&
          track->chainRow == player->loopRange.endChainRow) {
        track->chainRow = player->loopRange.startChainRow;
        track->phraseRow = player->loopRange.startPhraseRow;
        resetTrackFXAuxState(player, trackIdx);
        return stopped;
      }

      // Play mode logic:
      // Song playback
      if (track->mode == PlaybackMode::song) {
        // Next chain row
        int chain = p->song[track->songRow][trackIdx];
        if (chain != EMPTY_VALUE_16) {
          int chainRow = track->chainRow + 1;
          if (chainRow >= 16 || p->chains[chain].rows[chainRow].phrase == EMPTY_VALUE_16) {
            // Check song-level loop before advancing song row
            if (player->loopRange.enabled && player->loopRange.level == 0 && track->loop &&
                track->songRow == player->loopRange.endSongRow) {
              track->songRow = player->loopRange.startSongRow;
              track->chainRow = player->loopRange.startChainRow;
              track->phraseRow = player->loopRange.startPhraseRow;
              resetTrackFXAuxState(player, trackIdx);
              return stopped;
            }

            // Next song row
            int songRow = track->songRow + 1;
            track->chainRow = 0;
            if (songRow >= PROJECT_MAX_LENGTH || p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
              if (track->loop) {
                while (songRow > 0) {
                  songRow--;
                  if (p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
                    songRow++;
                    break;
                  }
                }
              } else {
                songRow = -1;
              }
            }
            if (songRow < 0 || p->song[songRow][trackIdx] == EMPTY_VALUE_16) {
              resetTrack(player, trackIdx, false);
              stopped = 1;
            } else {
              track->songRow = songRow;
            }
          } else {
            track->chainRow = chainRow;
          }
        } else {
          resetTrack(player, trackIdx, false);
        }
      }
      // Chain playback
      else if (track->mode == PlaybackMode::chain) {
        int chain = p->song[track->songRow][trackIdx];
        int chainRow = track->chainRow + 1;
        if (chainRow >= 16 || p->chains[chain].rows[chainRow].phrase == EMPTY_VALUE_16) {
          chainRow = track->loop ? 0 : -1;
        }
        if (chainRow < 0 || p->chains[chain].rows[chainRow].phrase == EMPTY_VALUE_16) {
          resetTrack(player, trackIdx, false);
          stopped = 1;
        } else {
          track->chainRow = chainRow;
        }
      }
      // Phrase playback
      else if (track->mode == PlaybackMode::phrase) {
        if (track->loop) {
          track->chainRow = track->queue.chainRow;
        } else {
          resetTrack(player, trackIdx, false);
          stopped = 1;
        }
      }
      // TODO: If in the future I will add NTH command from M8, this logic will need to be updated
      resetTrackFXAuxState(player, trackIdx);
    }

    return stopped;
  }

  static int skipZeroGrooveRows(Player* player, int trackIdx) {
    PlaybackTrackState* track = &player->tracks[trackIdx];
    Project* p = player->p;

    int curGrooveRow = track->grooveRow;
    while (p->grooves[track->grooveIdx].speed[track->grooveRow] == 0) {
      moveToNextPhraseRow(player, trackIdx);
      track->grooveRow++;
      if (track->grooveRow == 16 || p->grooves[track->grooveIdx].speed[track->grooveRow] == EMPTY_VALUE_8) {
        track->grooveRow = 0;
      }
      if (track->grooveRow == curGrooveRow) {
        // All rows are zero, stop playback
        resetTrack(player, trackIdx, true);
        return 0;
      }
    }

    return 1;
  }


  ///////////////////////////////////////////////////////////////////////////////
  //
  // Public interface
  //

  Player::Player() {
    p = nullptr;
    memset(tracks, 0, sizeof(tracks));
    memset(chips, 0, sizeof(chips));
    memset(trackEnabled, 0, sizeof(trackEnabled));
    memset(&loopRange, 0, sizeof(loopRange));
    memset(fxHandlers, 0, sizeof(fxHandlers));
  }

  Player::~Player() {
  }

  void Player::init(Project* project) {
    p = project;

    initFXHandlers();
    initAYSampleTables();

    for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
      resetTrack(this, c, true);
      tracks[c].queue.mode = PlaybackMode::none;
      tracks[c].queue.loop = 0;
      trackEnabled[c] = 1;
    }

    // Initialize loop range as disabled
    loopRange.enabled = 0;

    // TODO: Properly initialize other global chip states, but for now it's AY only
    for (int c = 0; c < PROJECT_MAX_CHIPS; c++) {
      chips[c].ay.envShape = 0;
    }
  }

  bool Player::isPlaying() {
    for (int trackIdx = 0; trackIdx < p->tracksCount; trackIdx++) {
      if (tracks[trackIdx].mode != PlaybackMode::stopped) return true;
    }
    return false;
  }

  void Player::setLoopRange(LoopRange range) {
    loopRange = range;
  }

  void Player::clearLoopRange() {
    loopRange.enabled = 0;
  }

  void Player::playSong(int songRow, int chainRow, int loop) {
    if (isPlaying()) return;

    for (int trackIdx = 0; trackIdx < p->tracksCount; trackIdx++) {
      PlaybackTrackState* track = &tracks[trackIdx];

      if (p->song[songRow][trackIdx] != EMPTY_VALUE_16 && p->chains[p->song[songRow][trackIdx]].rows[chainRow].phrase != EMPTY_VALUE_16) {
        track->queue.mode = PlaybackMode::song;
        track->queue.songRow = songRow;
        track->queue.chainRow = chainRow;
        track->queue.phraseRow = 0;
        track->queue.loop = loop;
      }
    }
  }

  void Player::playChain(int trackIdx, int songRow, int chainRow, int loop) {
    if (isPlaying()) return;

    PlaybackTrackState* track = &tracks[trackIdx];

    if (p->chains[p->song[songRow][trackIdx]].rows[chainRow].phrase != EMPTY_VALUE_16) {
      track->queue.mode = PlaybackMode::chain;
      track->queue.songRow = songRow;
      track->queue.chainRow = chainRow;
      track->queue.phraseRow = 0;
      track->queue.loop = loop;
    }
  }

  void Player::playPhrase(int trackIdx, int songRow, int chainRow, int loop) {
    if (isPlaying()) return;

    PlaybackTrackState* track = &tracks[trackIdx];

    track->queue.mode = PlaybackMode::phrase;
    track->queue.songRow = songRow;
    track->queue.chainRow = chainRow;
    track->queue.phraseRow = 0;
    track->queue.loop = loop;
  }

  void Player::playPhraseRow(int trackIdx, PhraseRow* phraseRow) {
    resetTrack(this, trackIdx, false);

    PlaybackTrackState* track = &tracks[trackIdx];

    // Set up phrase row playback
    track->queue.mode = PlaybackMode::phraseRow;
    track->songRow = 0;
    track->currentPhraseRow = *phraseRow;
  }

  void Player::queuePhrase(int trackIdx, int songRow, int chainRow) {
    PlaybackTrackState* track = &tracks[trackIdx];
    if (track->mode != PlaybackMode::phrase) return;
    if (track->songRow != songRow) return;
    // Ignore queued phrases when ranged loop is enabled
    if (loopRange.enabled) return;
    track->queue.mode = PlaybackMode::phrase;
    track->queue.songRow = songRow;
    track->queue.chainRow = chainRow;
    track->queue.phraseRow = 0;
    track->queue.loop = track->loop;
  }

  void Player::previewNote(int trackIdx, uint8_t note, uint8_t instrument) {
    // Create a phrase row for preview
    PhraseRow phraseRow = {0};
    phraseRow.note = note;
    phraseRow.instrument = instrument;
    phraseRow.volume = 15; // TODO: Use max volume for the chip

    // Set up empty FX
    for (int i = 0; i < 3; i++) {
      phraseRow.fx[i][0] = EMPTY_VALUE_8;
      phraseRow.fx[i][1] = EMPTY_VALUE_8;
    }

    // Use unified phrase row playback
    playPhraseRow(trackIdx, &phraseRow);
  }

  void Player::stop() {
    for (int c = 0; c < PROJECT_MAX_TRACKS; c++) {
      resetTrack(this, c, false);
      tracks[c].queue.mode = PlaybackMode::none;
    }
    // TODO: Move to AY-specific code when other chip types are added
    // Reset chip states to ensure envelope shapes retrigger on next playback
    for (int c = 0; c < PROJECT_MAX_CHIPS; c++) {
      chips[c].ay.envShape = 0;
    }
  }

  void Player::stopPreview(int trackIdx) {
    if (tracks[trackIdx].mode == PlaybackMode::phraseRow) {
      resetTrack(this, trackIdx, false);
    }
  }

  int Player::nextFrame(Engine* engine) {
    int hasActiveTracks = 0;

    int chipIdx = 0;
    int chipTracksCount = projectGetChipTracks(p, chipIdx);
    int nextChipTrackIdx = chipTracksCount;

    // Process all tracks left to right
    for (int trackIdx = 0; trackIdx < p->tracksCount; trackIdx++) {
      // Track chip index
      if (trackIdx >= nextChipTrackIdx) {
        chipIdx++;
        chipTracksCount = projectGetChipTracks(p, chipIdx);
        nextChipTrackIdx += chipTracksCount;
      }

      PlaybackTrackState* track = &tracks[trackIdx];

      // Check queued play event for stopped track or when a track is in phrase row playback mode
      if ((track->mode == PlaybackMode::stopped && track->queue.mode != PlaybackMode::none) ||
      (track->mode == PlaybackMode::phraseRow && track->queue.mode == PlaybackMode::phraseRow)) {
        track->mode = track->queue.mode;
        track->songRow = track->queue.songRow;
        track->chainRow = track->queue.chainRow;
        track->phraseRow = track->queue.phraseRow;
        track->loop = track->queue.loop;

        // Consume queued event
        track->queue.mode = PlaybackMode::none;

        skipZeroGrooveRows(this, trackIdx);
        readPhraseRow(trackIdx, 0);
      }
      // Advance further in the track
      else {
        tableProgress(this, trackIdx, &track->note.instrumentTable);
        tableProgress(this, trackIdx, &track->note.auxTable);

        // Don't do any playhead movement for phrase row
        if (track->mode != PlaybackMode::phraseRow && track->songRow != EMPTY_VALUE_16) {
          uint8_t grooveValue = p->grooves[track->grooveIdx].speed[track->grooveRow];

          if (grooveValue == EMPTY_VALUE_8) {
            // The current groove row doesn't have a value, stop playback
            resetTrack(this, trackIdx, false);
          } else {
            track->frameCounter++;

            if (track->frameCounter >= grooveValue) {
              // Go to the next groove row
              track->grooveRow++;
              if (track->grooveRow == 16 || p->grooves[track->grooveIdx].speed[track->grooveRow] == EMPTY_VALUE_8) {
                track->grooveRow = 0;
              }

              // Go to the next phrase row
              track->frameCounter = 0;
              moveToNextPhraseRow(this, trackIdx);
              skipZeroGrooveRows(this, trackIdx);
              readPhraseRow(trackIdx, 0);
            }
          }
        }
      }

      processTrackFrame(this, trackIdx, chipIdx);

      // Check if the track is still playing something
      if (track->songRow == EMPTY_VALUE_16) {
        track->mode = PlaybackMode::stopped;
      } else {
        hasActiveTracks = 1;
      }
    }

    // Output registers for all chips
    for (int ci = 0; ci < p->chipsCount; ci++) {
      outputRegistersAY(engine, ci * projectGetChipTracks(p, ci), ci);
    }

    return !hasActiveTracks;
  }
};
