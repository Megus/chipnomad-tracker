#include "help.h"
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "chipnomad_lib.h"
#include "corelib_gfx.h"
#include "common.h"
#include "misc.h"

// Helper function to get modulation type name
static const char* getModulationTypeName(enum ModulationType type) {
  switch (type) {
    case modADSR: return "ADSR";
    case modAHD: return "AHD";
    case modLFO: return "LFO";
    default: return "Unknown";
  }
}

// Helper function to get parameter name based on modulation type
static const char* getModulationParamName(enum ModulationType type, int paramIdx) {
  switch (type) {
    case modADSR:
      switch (paramIdx) {
        case 0: return "Attack";
        case 1: return "Decay";
        case 2: return "Sustain";
        case 3: return "Release";
        default: return "Param";
      }
    case modAHD:
      switch (paramIdx) {
        case 0: return "Attack";
        case 1: return "Hold";
        case 2: return "Decay";
        case 3: return "---";
        default: return "Param";
      }
    case modLFO:
      switch (paramIdx) {
        case 0: return "Shape";
        case 1: return "Trigger";
        case 2: return "Period";
        case 3: return "---";
        default: return "Param";
      }
    default:
      return "Param";
  }
}

const char* helpFXHint(uint8_t* fx, int isTable, uint8_t instrumentIdx) {
  static const int bufferSize = 41; // Max length of a hint string
  static char buffer[bufferSize];

  buffer[0] = 0; // Terminate string for unsupported FX
  int note;

  const char* arpModeHelp[16]={
    "Up (+0 oct)", "Down (+0 oct)", "Up/Down (+0 oct)",
    "Up (+1 oct)", "Down (+1 oct)", "Up/Down (+1 oct)",
    "Up (+2 oct)", "Down (+2 oct)", "Up/Down (+2 oct)",
    "Up (+3 oct)", "Down (+3 oct)", "Up/Down (+3 oct)",
    "Up (+4 oct)", "Down (+4 oct)", "Up/Down (+4 oct)",
    "Up (+5 oct)"
  };

  switch ((enum FX)fx[0]) {
    case fxARP: // Arpeggio
      snprintf(buffer, bufferSize, "Arpeggio 0, %hhu, %hhu semitones", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxARC: // Arpeggio config
      snprintf(buffer, bufferSize, "ARP config: %s, %d tics",arpModeHelp[(fx[1] & 0xf0) >> 4],(fx[1] & 0xf));
      break;
    case fxPVB: // Pitch vibrato
      snprintf(buffer, bufferSize, "Pitch vibrato, speed %hhu, depth %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxPBN: // Pitch bend
      if (chipnomadState->project.linearPitch) {
        // Linear pitch mode: show in semitones (value * 25 cents / 100 cents per semitone)
        float semitones = (int8_t)fx[1] * 0.25f;
        snprintf(buffer, bufferSize, "Pitch bend %+.2f semitones per step", semitones);
      } else {
        snprintf(buffer, bufferSize, "Pitch bend %hhd per step", fx[1]);
      }
      break;
    case fxPSL: // Pitch slide (portamento)
      snprintf(buffer, bufferSize, "Pitch slide for %hhd tics", fx[1]);
      break;
    case fxPIT: // Pitch offset (semitones)
      snprintf(buffer, bufferSize, "Pitch offset by %hhd steps", (int8_t)fx[1]);
      break;
    case fxFIN: // Fine pitch offset
      snprintf(buffer, bufferSize, "Fine pitch offset by %hhd", (int8_t)fx[1]);
      break;
    case fxPRD: // Period offset
      snprintf(buffer, bufferSize, "Period offset by %hhd", fx[1]);
      break;
    case fxVOL: // Volume (relative)
      snprintf(buffer, bufferSize, "Volume offset by %hhd", fx[1]);
      break;
    case fxRET: // Retrigger
      snprintf(buffer, bufferSize, "Retrigger note every %hhd tics", fx[1] & 0xf);
      break;
    case fxDEL: // Delay
      snprintf(buffer, bufferSize, "Delay note by %hhd tics", fx[1]);
      break;
    case fxOFF: // Off
      snprintf(buffer, bufferSize, "Note off after %hhu tics", fx[1]);
      break;
    case fxKIL: // Kill note
      snprintf(buffer, bufferSize, "Kill note after %hhu tics", fx[1]);
      break;
    case fxTIC: // Table speed
      snprintf(buffer, bufferSize, "Set table speed to %hhu tics", fx[1]);
      break;
    case fxTBL: // Set instrument table
      snprintf(buffer, bufferSize, "Instrument table %s", byteToHex(fx[1]));
      break;
    case fxTBX: // Set aux table
      snprintf(buffer, bufferSize, "Aux table %s", byteToHex(fx[1]));
      break;
    case fxTHO: // Table hop
      snprintf(buffer, bufferSize, "Hop to instrument table row %hhX", fx[1] & 0xf);
      break;
    case fxTXH: // Aux table hop
      snprintf(buffer, bufferSize, "Hop to aux table row %hhX", fx[1] & 0xf);
      break;
    case fxGRV: // Track groove
      snprintf(buffer, bufferSize, "Track groove %s", byteToHex(fx[1]));
      break;
    case fxGGR: // Global groove
      snprintf(buffer, bufferSize, "Global groove %s", byteToHex(fx[1]));
      break;
    case fxHOP: // Hop
      if (!isTable && fx[1] == 0xff) {
        snprintf(buffer, bufferSize, "Stop playback");
      } else {
        if ((fx[1] & 0xf0) == 0) {
          snprintf(buffer, bufferSize, "Hop to row %hhX ", fx[1] & 0xf);
        } else {
          snprintf(buffer, bufferSize, "Hop to row %hhX (%hhu times)", fx[1] & 0xf, fx[1] >> 4);
        }
      }
      break;
    case fxSNG: // Song hop
      if (isTable) {
        snprintf(buffer, bufferSize, "No effect");
      } else {
        snprintf(buffer, bufferSize, "Song hop by %hhd", fx[1]);
      }
      break;
    // Modulation FX - context-aware based on instrument
    case fxM1A: case fxM2A: case fxM3A: case fxM4A: {
      // Amount FX
      int modSlot = (fx[0] - fxM1A) / 5; // 0-3
      if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
        enum ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;
        snprintf(buffer, bufferSize, "Mod %d %s amount %+hhd", modSlot + 1, getModulationTypeName(modType), (int8_t)fx[1]);
      } else {
        snprintf(buffer, bufferSize, "Mod %d amount %+hhd", modSlot + 1, (int8_t)fx[1]);
      }
      break;
    }
    case fxM11: case fxM12: case fxM13: case fxM14:
    case fxM21: case fxM22: case fxM23: case fxM24:
    case fxM31: case fxM32: case fxM33: case fxM34:
    case fxM41: case fxM42: case fxM43: case fxM44: {
      // Parameter FX
      int modSlot = (fx[0] - fxM1A) / 5; // 0-3
      int paramIdx = (fx[0] - fxM1A) % 5 - 1; // 0-3 (subtract 1 because M1A is amount)
      if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
        enum ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;
        const char* paramName = getModulationParamName(modType, paramIdx);
        snprintf(buffer, bufferSize, "Mod %d %s %s %+hhd", modSlot + 1, getModulationTypeName(modType), paramName, (int8_t)fx[1]);
      } else {
        snprintf(buffer, bufferSize, "Mod %d parameter %d offset %+hhd", modSlot + 1, paramIdx + 1, (int8_t)fx[1]);
      }
      break;
    }
    // AY-specific FX
    case fxAYM: // AY Mixer settting
      if (fx[1] & 0xf0) {
        snprintf(buffer, bufferSize, "Mix %c%c, env %hhX %s", (fx[1] & 0x1) ? 'T' : '-', (fx[1] & 0x2) ? 'N' : '-', (fx[1] & 0xf0) >> 4, getEnvelopeShapeASCII((fx[1] & 0xf0) >> 4));
      } else {
        snprintf(buffer, bufferSize, "Mix %c%c", (fx[1] & 0x1) ? 'T' : '-', (fx[1] & 0x2) ? 'N' : '-');
      }
      break;
    case fxERT: // Envelope retrigger
      snprintf(buffer, bufferSize, "Retrigger envelope");
      break;
    case fxNOI: // Noise (relative)
      snprintf(buffer, bufferSize, "Noise period offset %hhd", fx[1]);
      break;
    case fxNOA: // Noise (absolute)
      if (fx[1] == EMPTY_VALUE_8) {
        snprintf(buffer, bufferSize, "Noise period off");
      } else {
        snprintf(buffer, bufferSize, "Noise period %s", byteToHex(fx[1]));
      }
      break;
    case fxEAU: // Auto-env setting
      if ((fx[1] & 0xf0) == 0) {
        snprintf(buffer, bufferSize, "Auto-envelope off");
      } else {
        uint8_t n = (fx[1] & 0xf0) >> 4;
        uint8_t d = fx[1] & 0xf;
        if (d == 0) d = 1;
        snprintf(buffer, bufferSize, "Auto-envelope %hhu:%hhu", n, d);
      }
      break;
    case fxEVB: // Envelope vibrato
      snprintf(buffer, bufferSize, "Envelope vibrato, speed %hhu, depth %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxEBN: // Envelope bend
      snprintf(buffer, bufferSize, "Envelope bend %hhd per step", fx[1]);
      break;
    case fxESL: // Envelope slide (portamento)
      snprintf(buffer, bufferSize, "Envelope slide in %hhd tics", fx[1]);
      break;
    case fxENT: // Envelope note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length - chipnomadState->project.pitchTable.octaveSize * 4)
        note = chipnomadState->project.pitchTable.length - 1 - chipnomadState->project.pitchTable.octaveSize * 4;
      snprintf(buffer, bufferSize, "Envelope note %s", noteName(&chipnomadState->project, note));
      break;
    case fxEPT: // Envelope period offset
      snprintf(buffer, bufferSize, "Envelope period offset by %hhd", fx[1]);
      break;
    case fxEPL: // Envelope period L
      snprintf(buffer, bufferSize, "Envelope period Low %s", byteToHex(fx[1]));
      break;
    case fxEPH: // Envelope period H
      snprintf(buffer, bufferSize, "Envelope period High %s", byteToHex(fx[1]));
      break;
    // Common AY FX
    case fxTNN: // Tone specific note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length)
        note = chipnomadState->project.pitchTable.length - 1;
      snprintf(buffer, bufferSize, "Tone note %s", noteName(&chipnomadState->project, note));
      break;
    case fxTNP: // Tone pitch offset
      snprintf(buffer, bufferSize, "Tone pitch offset %+hhd steps", (int8_t)fx[1]);
      break;
    case fxTNF: // Tone fine offset
      snprintf(buffer, bufferSize, "Tone fine offset %+hhd", (int8_t)fx[1]);
      break;
    case fxTRT: // Tone phase retrigger
      snprintf(buffer, bufferSize, "Retrigger tone phase");
      break;
    case fxENN: // Envelope specific note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length)
        note = chipnomadState->project.pitchTable.length - 1;
      snprintf(buffer, bufferSize, "Envelope note %s", noteName(&chipnomadState->project, note));
      break;
    case fxENP: // Envelope pitch offset
      snprintf(buffer, bufferSize, "Envelope pitch offset %+hhd steps", (int8_t)fx[1]);
      break;
    case fxENF: // Envelope fine offset
      snprintf(buffer, bufferSize, "Envelope fine offset %+hhd", (int8_t)fx[1]);
      break;
    // AY2-specific FX (software oscillator)
    case fxSFT: // Software oscillator type
      snprintf(buffer, bufferSize, "Software osc type %hhu", fx[1]);
      break;
    case fxSFN: // Software oscillator specific note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length)
        note = chipnomadState->project.pitchTable.length - 1;
      snprintf(buffer, bufferSize, "Software osc note %s", noteName(&chipnomadState->project, note));
      break;
    case fxSFP: // Software oscillator pitch offset
      snprintf(buffer, bufferSize, "Software osc pitch offset %+hhd steps", (int8_t)fx[1]);
      break;
    case fxSFF: // Software oscillator fine offset
      snprintf(buffer, bufferSize, "Software osc fine offset %+hhd", (int8_t)fx[1]);
      break;
    case fxSRT: // Software oscillator phase retrigger
      snprintf(buffer, bufferSize, "Retrigger software osc phase");
      break;
    case fxSFM: // FM depth
      snprintf(buffer, bufferSize, "FM depth %hhu", fx[1]);
      break;
    case fxPWM: // Pulse width
      snprintf(buffer, bufferSize, "Pulse width %hhu", fx[1]);
      break;
    case fxSPL: // Pulse low level
      snprintf(buffer, bufferSize, "Pulse low level %hhu", fx[1]);
      break;
    case fxSWT: // Wavetable index
      snprintf(buffer, bufferSize, "Wavetable index %hhu", fx[1]);
      break;
    // AYSample-specific FX
    case fxSMS: // Sample start position
      snprintf(buffer, bufferSize, "Sample start position %hhu", fx[1]);
      break;
    default:
      break;
  }

  return buffer;
}


static const char* fxHelpText[] = {
  [fxARP] = "Arpeggio\nCycles through 0, high, low\nsemitone offsets",
  [fxARC] = "Arpeggio Config\nSets arp direction, octave\nand timing",
  [fxPVB] = "Pitch Vibrato\nOscillates pitch up/down\nwith speed and depth",
  [fxPBN] = "Pitch Bend\nSlides pitch by amount\nper step continuously",
  [fxPSL] = "Pitch Slide\nSlides to target pitch\nover specified tics",
  [fxPIT] = "Pitch Offset\nAdds semitone offset\nto note pitch",
  [fxFIN] = "Fine Pitch Offset\nAdds fine offset\nto note pitch",
  [fxPRD] = "Period Offset\nAdds fixed offset\nto chip period",
  [fxVOL] = "Volume Offset\nAdds/subtracts from\ncurrent volume",
  [fxRET] = "Retrigger\nRetriggers note every\nN tics",
  [fxDEL] = "Delay\nDelays note start\nby N tics",
  [fxOFF] = "Note Off\nSends note off\nafter N tics",
  [fxKIL] = "Kill Note\nStops note completely\nafter N tics",
  [fxTIC] = "Table Speed\nSets instrument table\nplayback speed",
  [fxTBL] = "Set Table\nSwitches to specified\ninstrument table",
  [fxTBX] = "Aux Table\nSets auxiliary table\nfor this track",
  [fxTHO] = "Table Hop\nJumps to specific\ninstrument table row",
  [fxTXH] = "Aux Table Hop\nJumps to specific\naux table row",
  [fxGRV] = "Track Groove\nSets groove for\nthis track only",
  [fxGGR] = "Global Groove\nSets groove for\nall tracks",
  [fxHOP] = "Hop\nHops to phrase/table row X times",
  [fxSNG] = "Song Hop\nHops in song by\namount specified",
  // Modulation FX
  [fxM1A] = "Mod 1 Amount\nSets modulation 1\noutput amount",
  [fxM11] = "Mod 1 Param 1\nOffsets modulation 1\nparameter 1",
  [fxM12] = "Mod 1 Param 2\nOffsets modulation 1\nparameter 2",
  [fxM13] = "Mod 1 Param 3\nOffsets modulation 1\nparameter 3",
  [fxM14] = "Mod 1 Param 4\nOffsets modulation 1\nparameter 4",
  [fxM2A] = "Mod 2 Amount\nSets modulation 2\noutput amount",
  [fxM21] = "Mod 2 Param 1\nOffsets modulation 2\nparameter 1",
  [fxM22] = "Mod 2 Param 2\nOffsets modulation 2\nparameter 2",
  [fxM23] = "Mod 2 Param 3\nOffsets modulation 2\nparameter 3",
  [fxM24] = "Mod 2 Param 4\nOffsets modulation 2\nparameter 4",
  [fxM3A] = "Mod 3 Amount\nSets modulation 3\noutput amount",
  [fxM31] = "Mod 3 Param 1\nOffsets modulation 3\nparameter 1",
  [fxM32] = "Mod 3 Param 2\nOffsets modulation 3\nparameter 2",
  [fxM33] = "Mod 3 Param 3\nOffsets modulation 3\nparameter 3",
  [fxM34] = "Mod 3 Param 4\nOffsets modulation 3\nparameter 4",
  [fxM4A] = "Mod 4 Amount\nSets modulation 4\noutput amount",
  [fxM41] = "Mod 4 Param 1\nOffsets modulation 4\nparameter 1",
  [fxM42] = "Mod 4 Param 2\nOffsets modulation 4\nparameter 2",
  [fxM43] = "Mod 4 Param 3\nOffsets modulation 4\nparameter 3",
  [fxM44] = "Mod 4 Param 4\nOffsets modulation 4\nparameter 4",
  // AY-specific FX
  [fxAYM] = "AY Mixer\nControls tone/noise mix\nand envelope shape",
  [fxERT] = "Envelope Retrigger\nRestarts AY envelope\nfrom beginning",
  [fxNOI] = "Noise Offset\nAdds offset to\nnoise period",
  [fxNOA] = "Noise Absolute\nSets noise period\nto exact value",
  [fxEAU] = "Auto Envelope\nAutomatic envelope\nperiod from note",
  [fxEVB] = "Envelope Vibrato\nOscillates envelope\nperiod up/down",
  [fxEBN] = "Envelope Bend\nSlides envelope period\nby amount per step",
  [fxESL] = "Envelope Slide\nSlides to envelope\nperiod over N tics",
  [fxENT] = "Envelope Note\nSets envelope period\nfrom note value",
  [fxEPT] = "Envelope Offset\nAdds offset to\nenvelope period",
  [fxEPL] = "Envelope Low\nSets low byte of\nenvelope period",
  [fxEPH] = "Envelope High\nSets high byte of\nenvelope period",
  // Common AY FX (all AY types)
  [fxTNN] = "Tone Note\nSets tone oscillator\nto specific note",
  [fxTNP] = "Tone Pitch\nOffsets tone oscillator\npitch (steps)",
  [fxTNF] = "Tone Fine\nOffsets tone oscillator\nfine tune (period/cents)",
  [fxTRT] = "Tone Retrigger\nResest tone oscillator phase",
  [fxENN] = "Envelope Note\nSets envelope oscillator\nto specific note",
  [fxENP] = "Envelope Pitch\nOffsets envelope oscillator\npitch (steps)",
  [fxENF] = "Envelope Fine\nOffsets envelope oscillator\nfine tune (period/cents)",
  // AY2-specific FX (software oscillator)
  [fxSFT] = "Software Osc Type\nSets software oscillator type",
  [fxSFN] = "Software Osc Note\nSets software oscillator\nto specific note",
  [fxSFP] = "Software Osc Pitch\nOffsets software oscillator\npitch (steps)",
  [fxSFF] = "Software Osc Fine\nOffsets software oscillator\nfine tune (period/cents)",
  [fxSRT] = "Software Osc Retrig\nRestarts software oscillator\nphase from zero",
  [fxSFM] = "FM Depth\nSets FM modulation depth\nfor software oscillator",
  [fxPWM] = "Pulse Width\nSets pulse width\nfor Pulse oscillator",
  [fxSPL] = "Pulse Low Level\nSets low period level\nfor Pulse oscillator",
  [fxSWT] = "Wavetable Index\nSets wavetable index\nfor Wavetable oscillator",
  // AYSample-specific FX
  [fxSMS] = "Sample Start\nSets sample playback\nstart position"
};

const char* helpFXDescription(enum FX fxIdx, uint8_t instrumentIdx) {
  static const int bufferSize = 120;
  static char buffer[bufferSize]; // Buffer for dynamic description

  // For modulation FX, generate context-aware descriptions
  if (fxIdx >= fxM1A && fxIdx <= fxM44) {
    int modSlot = (fxIdx - fxM1A) / 5; // 0-3
    int paramIdx = (fxIdx - fxM1A) % 5; // 0=amount, 1-4=params

    if (paramIdx == 0) {
      // Amount FX
      if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
        enum ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;
        snprintf(buffer, bufferSize, "Mod %d Amount\n%s: Scales envelope\noutput depth",
                modSlot + 1, getModulationTypeName(modType));
      } else {
        snprintf(buffer, bufferSize, "Mod %d Amount\nSets modulation %d\noutput amount", modSlot + 1, modSlot + 1);
      }
    } else {
      // Parameter FX
      if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
        enum ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;

        // Generate context-specific description
        switch (modType) {
          case modADSR:
            switch (paramIdx - 1) {
              case 0: snprintf(buffer, bufferSize, "Mod %d Param 1\nADSR: Attack time\n(0-255 tics)", modSlot + 1); break;
              case 1: snprintf(buffer, bufferSize, "Mod %d Param 2\nADSR: Decay time\n(0-255 tics)", modSlot + 1); break;
              case 2: snprintf(buffer, bufferSize, "Mod %d Param 3\nADSR: Sustain level\n(0-255)", modSlot + 1); break;
              case 3: snprintf(buffer, bufferSize, "Mod %d Param 4\nADSR: Release time\n(0-255 tics)", modSlot + 1); break;
            }
            break;
          case modAHD:
            switch (paramIdx - 1) {
              case 0: snprintf(buffer, bufferSize, "Mod %d Param 1\nAHD: Attack time\n(0-255 tics)", modSlot + 1); break;
              case 1: snprintf(buffer, bufferSize, "Mod %d Param 2\nAHD: Hold time\n(0-255 tics)", modSlot + 1); break;
              case 2: snprintf(buffer, bufferSize, "Mod %d Param 3\nAHD: Decay time\n(0-255 tics)", modSlot + 1); break;
              case 3: snprintf(buffer, bufferSize, "Mod %d Param 4\nAHD: (unused)", modSlot + 1); break;
            }
            break;
          case modLFO:
            switch (paramIdx - 1) {
              case 0: snprintf(buffer, bufferSize, "Mod %d Param 1\nLFO: Wave shape\n(0-7)", modSlot + 1); break;
              case 1: snprintf(buffer, bufferSize, "Mod %d Param 2\nLFO: Trigger mode\n(0-3)", modSlot + 1); break;
              case 2: snprintf(buffer, bufferSize, "Mod %d Param 3\nLFO: Period\n(0-255 tics)", modSlot + 1); break;
              case 3: snprintf(buffer, bufferSize, "Mod %d Param 4\nLFO: (unused)", modSlot + 1); break;
            }
            break;
          default:
            snprintf(buffer, bufferSize, "Mod %d Param %d\nOffsets modulation %d\nparameter %d",
                    modSlot + 1, paramIdx, modSlot + 1, paramIdx);
            break;
        }
      } else {
        snprintf(buffer, bufferSize, "Mod %d Param %d\nOffsets modulation %d\nparameter %d",
                modSlot + 1, paramIdx, modSlot + 1, paramIdx);
      }
    }
    return buffer;
  }

  // For non-modulation FX, use static descriptions
  if (fxIdx < fxTotalCount && fxHelpText[fxIdx]) {
    return (char*)fxHelpText[fxIdx];
  }
  return (const char*)"";
}

void drawFXHelp(enum FX fxIdx, uint8_t instrumentIdx) {
  const char* helpText = helpFXDescription(fxIdx, instrumentIdx);
  if (!helpText || !helpText[0]) return;

  gfxSetFgColor(appSettings.colorScheme.textDefault);

  char line[41];
  int y = 1;
  int pos = 0;
  int lineStart = 0;

  while (helpText[pos] && y <= 5) {
    if (helpText[pos] == '\n' || pos - lineStart >= 39) {
      int len = pos - lineStart;
      if (len > 39) len = 39;
      strncpy(line, &helpText[lineStart], len);
      line[len] = '\0';
      gfxPrint(1, y++, line);

      if (helpText[pos] == '\n') {
        pos++;
      }
      lineStart = pos;
    } else {
      pos++;
    }
  }

  // Print remaining text if any
  if (lineStart < pos && y <= 5) {
    int len = pos - lineStart;
    if (len > 39) len = 39;
    strncpy(line, &helpText[lineStart], len);
    line[len] = '\0';
    gfxPrint(1, y, line);
  }
}
