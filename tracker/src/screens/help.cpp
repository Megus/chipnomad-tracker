#include "help.h"
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "chipnomad_lib.h"
#include "corelib_gfx.h"
#include "common.h"
#include "misc.h"

// Helper function to get modulation type name
static const char* getModulationTypeName(ModulationType type) {
  switch (type) {
    case ModulationType::ADSR: return "ADSR";
    case ModulationType::AHD: return "AHD";
    case ModulationType::LFO: return "LFO";
    default: return "Unknown";
  }
}

// Helper function to get parameter name based on modulation type
static const char* getModulationParamName(ModulationType type, int paramIdx) {
  switch (type) {
    case ModulationType::ADSR:
      switch (paramIdx) {
        case 0: return "Attack";
        case 1: return "Decay";
        case 2: return "Sustain";
        case 3: return "Release";
        default: return "Param";
      }
    case ModulationType::AHD:
      switch (paramIdx) {
        case 0: return "Attack";
        case 1: return "Hold";
        case 2: return "Decay";
        case 3: return "---";
        default: return "Param";
      }
    case ModulationType::LFO:
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
      snprintf(buffer, bufferSize, "Arp intervals: %hhu, %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxARC: // Arpeggio config
      snprintf(buffer, bufferSize, "ARP config: %s, %d tics",arpModeHelp[(fx[1] & 0xf0) >> 4],(fx[1] & 0xf));
      break;
    case fxPVB: // Pitch vibrato
      snprintf(buffer, bufferSize, "Vibrato: speed %hhu, depth %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
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
      snprintf(buffer, bufferSize, "Pitch: %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxFIN: // Fine pitch offset
      snprintf(buffer, bufferSize, "Finetune: %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxPRD: // Period offset
      snprintf(buffer, bufferSize, "Period: %+hhd (relative)", fx[1]);
      break;
    case fxVOL: // Volume (relative)
      snprintf(buffer, bufferSize, "Volume: %+hhd (relative)", fx[1]);
      break;
    case fxVSL: // Volume slide
      snprintf(buffer, bufferSize, "Volume slide %+hhd per step", (int8_t)fx[1]);
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
        ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;
        snprintf(buffer, bufferSize, "Mod %d %s amount %+hhd (relative)", modSlot + 1, getModulationTypeName(modType), (int8_t)fx[1]);
      } else {
        snprintf(buffer, bufferSize, "Mod %d amount %+hhd (relative)", modSlot + 1, (int8_t)fx[1]);
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
        ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;
        const char* paramName = getModulationParamName(modType, paramIdx);
        snprintf(buffer, bufferSize, "Mod %d %s %s: %+hhd (relative)", modSlot + 1, getModulationTypeName(modType), paramName, (int8_t)fx[1]);
      } else {
        snprintf(buffer, bufferSize, "Mod %d param %d: %+hhd (relative)", modSlot + 1, paramIdx + 1, (int8_t)fx[1]);
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
      snprintf(buffer, bufferSize, "Noise period: %+hhd (relative)", fx[1]);
      break;
    case fxNOA: // Noise (absolute)
      if (fx[1] == EMPTY_VALUE_8) {
        snprintf(buffer, bufferSize, "Noise period: off");
      } else {
        snprintf(buffer, bufferSize, "Noise period: %s (absolute)", byteToHex(fx[1]));
      }
      break;
    case fxEAU: // Auto-env setting
      if ((fx[1] & 0xf0) == 0) {
        snprintf(buffer, bufferSize, "Auto-envelope: off");
      } else {
        uint8_t n = (fx[1] & 0xf0) >> 4;
        uint8_t d = fx[1] & 0xf;
        if (d == 0) d = 1;
        snprintf(buffer, bufferSize, "Auto-envelope: %hhu:%hhu", n, d);
      }
      break;
    case fxEVB: // Envelope vibrato
      snprintf(buffer, bufferSize, "Envelope vibrato: speed %hhu, depth %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxEBN: // Envelope bend
      snprintf(buffer, bufferSize, "Envelope bend %+hhd per step", (int8_t)fx[1]);
      break;
    case fxESL: // Envelope slide (portamento)
      snprintf(buffer, bufferSize, "Envelope slide for %hhd tics", fx[1]);
      break;
    case fxENT: // Envelope note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length - chipnomadState->project.pitchTable.octaveSize * 4)
        note = chipnomadState->project.pitchTable.length - 1 - chipnomadState->project.pitchTable.octaveSize * 4;
      snprintf(buffer, bufferSize, "Envelope note %s", noteName(&chipnomadState->project, note));
      break;
    case fxEPT: // Envelope period offset
      snprintf(buffer, bufferSize, "Envelope period: %+hhd (relative)", fx[1]);
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
      snprintf(buffer, bufferSize, "Tone pitch: %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxTNF: // Tone fine offset
      snprintf(buffer, bufferSize, "Tone finetune: %+hhd (relative)", (int8_t)fx[1]);
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
      snprintf(buffer, bufferSize, "Envelope pitch: %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxENF: // Envelope fine offset
      snprintf(buffer, bufferSize, "Envelope finetune: %+hhd (relative)", (int8_t)fx[1]);
      break;
    // AY2-specific FX (software oscillator)
    case fxSFT: // Software oscillator type
    {
      static const char* oscTypeNames[] = {
        "Off", "Pulse", "SyncTone", "SyncEnv", "Wavetbl", "ToneFM", "EnvFM"
      };
      const char* name = (fx[1] < static_cast<uint8_t>(AYSoftwareOscType::sample)) ? oscTypeNames[fx[1]] : "?";
      snprintf(buffer, bufferSize, "Software osc: %s", name);
      break;
    }
    case fxSFN: // Software oscillator specific note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length)
        note = chipnomadState->project.pitchTable.length - 1;
      snprintf(buffer, bufferSize, "Software osc note %s", noteName(&chipnomadState->project, note));
      break;
    case fxSFP: // Software oscillator pitch offset
      snprintf(buffer, bufferSize, "Software osc pitch: %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxSFF: // Software oscillator fine offset
      snprintf(buffer, bufferSize, "Software osc fine tune: %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxSRT: // Software oscillator phase retrigger
      snprintf(buffer, bufferSize, "Reset software osc phase");
      break;
    case fxSFM: // FM depth
      snprintf(buffer, bufferSize, "FM depth %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxPWM: // Pulse width
      snprintf(buffer, bufferSize, "Pulse width %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxSPL: // Pulse low level
      snprintf(buffer, bufferSize, "Pulse low level %+hhd (relative)", (int8_t)fx[1]);
      break;
    case fxSWT: // Wavetable index
      snprintf(buffer, bufferSize, "Wavetable index %+hhd (relative)", (int8_t)fx[1]);
      break;
    // AYSample-specific FX
    case fxSMS: // Sample start position
      snprintf(buffer, bufferSize, "Sample start position %04hX", (uint16_t)fx[1] * 64);
      break;
    default:
      break;
  }

  return buffer;
}


// FX help text storage - initialized at first use
static const char* fxHelpText[256] = {nullptr};
static bool fxHelpTextInitialized = false;

static void initFxHelpText() {
  if (fxHelpTextInitialized) return;

  fxHelpText[fxARP] = "Arpeggio\nFast arpeggio through 3 notes\nwith the specified intervals";
  fxHelpText[fxARC] = "Arpeggio Config\nSets arpeggio direction, range\nand speed";
  fxHelpText[fxPVB] = "Pitch Vibrato\nOscillates pitch up/down\nwith specified speed and depth";
  fxHelpText[fxPBN] = "Pitch Bend\nSlides pitch by specified amount\nper step continuously";
  fxHelpText[fxPSL] = "Pitch Slide\nSlides to target pitch\nover specified duration";
  fxHelpText[fxPIT] = "Pitch (relative)\nAdds pitch offset to note";
  fxHelpText[fxFIN] = "Finetune (relative)\nAdds finetune offset to note";
  fxHelpText[fxPRD] = "Period (relative)\nAdds offset to note period\n(chip specific)";
  fxHelpText[fxVOL] = "Volume (relative)\nAdds volume offset";
  fxHelpText[fxVSL] = "Volume Slide\nChanges volume by specified\namount per step continuously";
  fxHelpText[fxRET] = "Retrigger\nRetriggers note every N tics";
  fxHelpText[fxDEL] = "Delay\nDelays note start by N tics";
  fxHelpText[fxOFF] = "Note Off\nSends note off after N tics";
  fxHelpText[fxKIL] = "Kill Note\nStops note completely\nafter N tics";
  fxHelpText[fxTIC] = "Table Speed\nSets table playback speed";
  fxHelpText[fxTBL] = "Set Table\nSwitches to specified\ninstrument table";
  fxHelpText[fxTBX] = "Aux Table\nSets auxiliary table\nfor this note";
  fxHelpText[fxTHO] = "Table Hop\nJumps to specific\ninstrument table row";
  fxHelpText[fxTXH] = "Aux Table Hop\nJumps to specific\naux table row";
  fxHelpText[fxGRV] = "Track Groove\nSets groove for this track only";
  fxHelpText[fxGGR] = "Global Groove\nSets groove for all tracks";
  fxHelpText[fxHOP] = "Hop\nHops to phrase/table row X times";
  fxHelpText[fxSNG] = "Song Hop\nHops in song by N rows";
  // Modulation FX
  fxHelpText[fxM1A] = "Mod 1 Amount (relative)\nOffsets modulation 1\noutput amount";
  fxHelpText[fxM11] = "Mod 1 Param 1 (relative)\nOffsets modulation 1\nparameter 1";
  fxHelpText[fxM12] = "Mod 1 Param 2 (relative)\nOffsets modulation 1\nparameter 2";
  fxHelpText[fxM13] = "Mod 1 Param 3 (relative)\nOffsets modulation 1\nparameter 3";
  fxHelpText[fxM14] = "Mod 1 Param 4 (relative)\nOffsets modulation 1\nparameter 4";
  fxHelpText[fxM2A] = "Mod 2 Amount (relative)\nOffsets modulation 2\noutput amount";
  fxHelpText[fxM21] = "Mod 2 Param 1 (relative)\nOffsets modulation 2\nparameter 1";
  fxHelpText[fxM22] = "Mod 2 Param 2 (relative)\nOffsets modulation 2\nparameter 2";
  fxHelpText[fxM23] = "Mod 2 Param 3 (relative)\nOffsets modulation 2\nparameter 3";
  fxHelpText[fxM24] = "Mod 2 Param 4 (relative)\nOffsets modulation 2\nparameter 4";
  fxHelpText[fxM3A] = "Mod 3 Amount (relative)\nOffsets modulation 3\noutput amount";
  fxHelpText[fxM31] = "Mod 3 Param 1 (relative)\nOffsets modulation 3\nparameter 1";
  fxHelpText[fxM32] = "Mod 3 Param 2 (relative)\nOffsets modulation 3\nparameter 2";
  fxHelpText[fxM33] = "Mod 3 Param 3 (relative)\nOffsets modulation 3\nparameter 3";
  fxHelpText[fxM34] = "Mod 3 Param 4 (relative)\nOffsets modulation 3\nparameter 4";
  fxHelpText[fxM4A] = "Mod 4 Amount (relative)\nOffsets modulation 4\noutput amount";
  fxHelpText[fxM41] = "Mod 4 Param 1 (relative)\nOffsets modulation 4\nparameter 1";
  fxHelpText[fxM42] = "Mod 4 Param 2 (relative)\nOffsets modulation 4\nparameter 2";
  fxHelpText[fxM43] = "Mod 4 Param 3 (relative)\nOffsets modulation 4\nparameter 3";
  fxHelpText[fxM44] = "Mod 4 Param 4 (relative)\nOffsets modulation 4\nparameter 4";
  // AY-specific FX
  fxHelpText[fxAYM] = "AY Mixer\nControls tone/noise mix\nand envelope shape";
  fxHelpText[fxERT] = "Envelope Retrigger\nRestarts AY envelope\nfrom beginning";
  fxHelpText[fxNOI] = "Noise (relative)\nAdds offset to noise period";
  fxHelpText[fxNOA] = "Noise (absolute)\nSets noise period to exact value";
  fxHelpText[fxEAU] = "Auto Envelope\nSet automatic envelope\nparameters (N:D)";
  fxHelpText[fxEVB] = "Envelope Vibrato\nOscillates envelope\nperiod up/down";
  fxHelpText[fxEBN] = "Envelope Bend\nSlides envelope period\nby amount per step";
  fxHelpText[fxESL] = "Envelope Slide\nSlides to envelope\nperiod over N tics";
  fxHelpText[fxENT] = "Envelope Note\nSets envelope period\nfrom note value";
  fxHelpText[fxEPT] = "Envelope (relative)\nAdds offset to envelope period";
  fxHelpText[fxEPL] = "Envelope Low\nSets low byte of\nenvelope period";
  fxHelpText[fxEPH] = "Envelope High\nSets high byte of\nenvelope period";
  // Common AY FX (all AY types)
  fxHelpText[fxTNN] = "Tone Note\nSets tone oscillator\nto specific note";
  fxHelpText[fxTNP] = "Tone Pitch (relative)\nAdds tone pitch offset";
  fxHelpText[fxTNF] = "Tone Finetune (relative)\nAdd tone finetune offset";
  fxHelpText[fxTRT] = "Tone Retrigger\nResets tone oscillator phase";
  fxHelpText[fxENN] = "Envelope Note\nSets envelope oscillator\nto specific note";
  fxHelpText[fxENP] = "Envelope Pitch (relative)\nAdds envelope pitch offset";
  fxHelpText[fxENF] = "Envelope Fine (relative)\nAdds envelope finetune offset";
  // AY2-specific FX (software oscillator)
  fxHelpText[fxSFT] = "Software Osc Type\nSets software oscillator type";
  fxHelpText[fxSFN] = "Software Osc Note\nSets software oscillator\nto specific note";
  fxHelpText[fxSFP] = "Software Osc Pitch (relative)\nAdds pitch offset\nto softwareoscillator";
  fxHelpText[fxSFF] = "Software Osc Fine (relative)\nAdds finetune offset\nto software oscillator";
  fxHelpText[fxSRT] = "Software Osc Retrigger\nResets software oscillator phase";
  fxHelpText[fxSFM] = "FM Depth (relative)\nOffsets FM modulation depth\nfor software oscillator";
  fxHelpText[fxPWM] = "Pulse Width (relative)\nOffsets pulse width\nfor software oscillator";
  fxHelpText[fxSPL] = "Pulse Low Level (relative)\nOffsets low period level\nfor Pulse oscillator";
  fxHelpText[fxSWT] = "Wavetable Index (relative)\nOffsets wavetable index\nfor Wavetable oscillator";
  // AYSample-specific FX
  fxHelpText[fxSMS] = "Sample Start\nSets sample playback\nstart position";

  fxHelpTextInitialized = true;
}

const char* helpFXDescription(enum FX fxIdx, uint8_t instrumentIdx) {
  initFxHelpText(); // Initialize on first use

  static const int bufferSize = 120;
  static char buffer[bufferSize]; // Buffer for dynamic description

  // For modulation FX, generate context-aware descriptions
  if (fxIdx >= fxM1A && fxIdx <= fxM44) {
    int modSlot = (fxIdx - fxM1A) / 5; // 0-3
    int paramIdx = (fxIdx - fxM1A) % 5; // 0=amount, 1-4=params

    if (paramIdx == 0) {
      // Amount FX
      if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
        ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;
        snprintf(buffer, bufferSize, "Mod %d Amount (relative)\n%s: Offsets modulation\namount",
                modSlot + 1, getModulationTypeName(modType));
      } else {
        snprintf(buffer, bufferSize, "Mod %d Amount (relative)\nOffsets modulation %d amount", modSlot + 1, modSlot + 1);
      }
    } else {
      // Parameter FX
      if (instrumentIdx != EMPTY_VALUE_8 && instrumentIdx < PROJECT_MAX_INSTRUMENTS) {
        ModulationType modType = chipnomadState->project.instruments[instrumentIdx].modulation[modSlot].type;

        // Generate context-specific description
        switch (modType) {
          case ModulationType::ADSR:
            switch (paramIdx - 1) {
              case 0: snprintf(buffer, bufferSize, "Mod %d Param 1 (relative)\nADSR: Attack time offset", modSlot + 1); break;
              case 1: snprintf(buffer, bufferSize, "Mod %d Param 2 (relative)\nADSR: Decay time offset", modSlot + 1); break;
              case 2: snprintf(buffer, bufferSize, "Mod %d Param 3 (relative)\nADSR: Sustain level offset", modSlot + 1); break;
              case 3: snprintf(buffer, bufferSize, "Mod %d Param 4 (relative)\nADSR: Release time offset", modSlot + 1); break;
            }
            break;
          case ModulationType::AHD:
            switch (paramIdx - 1) {
              case 0: snprintf(buffer, bufferSize, "Mod %d Param 1 (relative)\nAHD: Attack time offset\n(0-255 tics)", modSlot + 1); break;
              case 1: snprintf(buffer, bufferSize, "Mod %d Param 2 (relative)\nAHD: Hold time offset\n(0-255 tics)", modSlot + 1); break;
              case 2: snprintf(buffer, bufferSize, "Mod %d Param 3 (relative)\nAHD: Decay time offset\n(0-255 tics)", modSlot + 1); break;
              case 3: snprintf(buffer, bufferSize, "Mod %d Param 4 (relative)\nAHD: (unused)", modSlot + 1); break;
            }
            break;
          case ModulationType::LFO:
            switch (paramIdx - 1) {
              case 0: snprintf(buffer, bufferSize, "Mod %d Param 1 (relative)\nLFO: Wave shape offset", modSlot + 1); break;
              case 1: snprintf(buffer, bufferSize, "Mod %d Param 2 (relative)\nLFO: Trigger mode offset", modSlot + 1); break;
              case 2: snprintf(buffer, bufferSize, "Mod %d Param 3 (relative)\nLFO: Period offset", modSlot + 1); break;
              case 3: snprintf(buffer, bufferSize, "Mod %d Param 4 (relative)\nLFO: (unused)", modSlot + 1); break;
            }
            break;
          default:
            snprintf(buffer, bufferSize, "Mod %d Param %d (relative)\nOffsets modulation %d\nparameter %d",
                    modSlot + 1, paramIdx, modSlot + 1, paramIdx);
            break;
        }
      } else {
        snprintf(buffer, bufferSize, "Mod %d Param %d (relative)\nOffsets modulation %d\nparameter %d",
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


  char line[41];
  int y = 1;
  int pos = 0;
  int lineStart = 0;

  gfxSetFgColor(appSettings.colorScheme.textValue);

  while (helpText[pos] && y <= 5) {
    if (helpText[pos] == '\n' || pos - lineStart >= 39) {
      int len = pos - lineStart;
      if (len > 39) len = 39;
      strncpy(line, &helpText[lineStart], len);
      line[len] = '\0';
      gfxPrint(1, y++, line);
      gfxSetFgColor(appSettings.colorScheme.textDefault);

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
