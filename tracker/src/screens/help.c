#include "help.h"
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "chipnomad_lib.h"
#include "corelib_gfx.h"
#include "common.h"

char* helpFXHint(uint8_t* fx, int isTable) {
  static char buffer[41]; // Max length of a hint string

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
      sprintf(buffer, "Arpeggio 0, %hhu, %hhu semitones", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxARC: // Arpeggio config
      sprintf(buffer, "ARP config: %s, %d tics",arpModeHelp[(fx[1] & 0xf0) >> 4],(fx[1] & 0xf));
      break;
    case fxPVB: // Pitch vibrato
      sprintf(buffer, "Pitch vibrato, speed %hhu, depth %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxPBN: // Pitch bend
      if (chipnomadState->project.linearPitch) {
        // Linear pitch mode: show in semitones (value * 25 cents / 100 cents per semitone)
        float semitones = (int8_t)fx[1] * 0.25f;
        sprintf(buffer, "Pitch bend %+.2f semitones per step", semitones);
      } else {
        sprintf(buffer, "Pitch bend %hhd per step", fx[1]);
      }
      break;
    case fxPSL: // Pitch slide (portamento)
      sprintf(buffer, "Pitch slide for %hhd tics", fx[1]);
      break;
    case fxPIT: // Pitch offset (semitones)
      sprintf(buffer, "Pitch offset by %hhd semitones", (int8_t)fx[1]);
      break;
    case fxFIN: // Fine pitch offset
      sprintf(buffer, "Fine pitch offset by %hhd", (int8_t)fx[1]);
      break;
    case fxPRD: // Period offset
      sprintf(buffer, "Period offset by %hhd", fx[1]);
      break;
    case fxVOL: // Volume (relative)
      sprintf(buffer, "Volume offset by %hhd", fx[1]);
      break;
    case fxRET: // Retrigger
      sprintf(buffer, "Retrigger note every %hhd tics", fx[1] & 0xf);
      break;
    case fxDEL: // Delay
      sprintf(buffer, "Delay note by %hhd tics", fx[1]);
      break;
    case fxOFF: // Off
      sprintf(buffer, "Note off after %hhu tics", fx[1]);
      break;
    case fxKIL: // Kill note
      sprintf(buffer, "Kill note after %hhu tics", fx[1]);
      break;
    case fxTIC: // Table speed
      sprintf(buffer, "Set table speed to %hhu tics", fx[1]);
      break;
    case fxTBL: // Set instrument table
      sprintf(buffer, "Instrument table %s", byteToHex(fx[1]));
      break;
    case fxTBX: // Set aux table
      sprintf(buffer, "Aux table %s", byteToHex(fx[1]));
      break;
    case fxTHO: // Table hop
      sprintf(buffer, "Hop to instrument table row %hhX", fx[1] & 0xf);
      break;
    case fxTXH: // Aux table hop
      sprintf(buffer, "Hop to aux table row %hhX", fx[1] & 0xf);
      break;
    case fxGRV: // Track groove
      sprintf(buffer, "Track groove %s", byteToHex(fx[1]));
      break;
    case fxGGR: // Global groove
      sprintf(buffer, "Global groove %s", byteToHex(fx[1]));
      break;
    case fxHOP: // Hop
      if (!isTable && fx[1] == 0xff) {
        sprintf(buffer, "Stop playback");
      } else {
        if ((fx[1] & 0xf0) == 0) {
          sprintf(buffer, "Hop to row %hhX ", fx[1] & 0xf);
        } else {
          sprintf(buffer, "Hop to row %hhX (%hhu times)", fx[1] & 0xf, fx[1] >> 4);
        }
      }
      break;
    case fxSNG: // Song hop
      if (isTable) {
        sprintf(buffer, "No effect");
      } else {
        sprintf(buffer, "Song hop by %hhd", fx[1]);
      }
      break;
    // AY-specific FX
    case fxAYM: // AY Mixer settting
      if (fx[1] & 0xf0) {
        sprintf(buffer, "Mix %c%c, env %hhX", (fx[1] & 0x1) ? 'T' : '-', (fx[1] & 0x2) ? 'N' : '-', (fx[1] & 0xf0) >> 4);
      } else {
        sprintf(buffer, "Mix %c%c", (fx[1] & 0x1) ? 'T' : '-', (fx[1] & 0x2) ? 'N' : '-');
      }
      break;
    case fxERT: // Envelope retrigger
      sprintf(buffer, "Retrigger envelope");
      break;
    case fxNOI: // Noise (relative)
      sprintf(buffer, "Noise period offset %hhd", fx[1]);
      break;
    case fxNOA: // Noise (absolute)
      if (fx[1] == EMPTY_VALUE_8) {
        sprintf(buffer, "Noise period off");
      } else {
        sprintf(buffer, "Noise period %s", byteToHex(fx[1]));
      }
      break;
    case fxEAU: // Auto-env setting
      if ((fx[1] & 0xf0) == 0) {
        sprintf(buffer, "Auto-envelope off");
      } else {
        uint8_t n = (fx[1] & 0xf0) >> 4;
        uint8_t d = fx[1] & 0xf;
        if (d == 0) d = 1;
        sprintf(buffer, "Auto-envelope %hhu:%hhu", n, d);
      }
      break;
    case fxEVB: // Envelope vibrato
      sprintf(buffer, "Envelope vibrato, speed %hhu, depth %hhu", (fx[1] & 0xf0) >> 4, (fx[1] & 0xf));
      break;
    case fxEBN: // Envelope bend
      sprintf(buffer, "Envelope bend %hhd per step", fx[1]);
      break;
    case fxESL: // Envelope slide (portamento)
      sprintf(buffer, "Envelope slide in %hhd tics", fx[1]);
      break;
    case fxENT: // Envelope note
      note = fx[1];
      if (note >= chipnomadState->project.pitchTable.length - chipnomadState->project.pitchTable.octaveSize * 4)
        note = chipnomadState->project.pitchTable.length - 1 - chipnomadState->project.pitchTable.octaveSize * 4;
      sprintf(buffer, "Envelope note %s", noteName(&chipnomadState->project, note));
      break;
    case fxEPT: // Envelope period offset
      sprintf(buffer, "Envelope offset by %hhd", fx[1]);
      break;
    case fxEPL: // Envelope period L
      sprintf(buffer, "Envelope period Low %s", byteToHex(fx[1]));
      break;
    case fxEPH: // Envelope period H
      sprintf(buffer, "Envelope period High %s", byteToHex(fx[1]));
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
  [fxAYM] = "AY Mixer\nControls tone/noise mix\nand envelope mode",
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
  [fxEPH] = "Envelope High\nSets high byte of\nenvelope period"
};

char* helpFXDescription(enum FX fxIdx) {
  if (fxIdx < fxTotalCount && fxHelpText[fxIdx]) {
    return (char*)fxHelpText[fxIdx];
  }
  return "";
}

void drawFXHelp(enum FX fxIdx) {
  const char* helpText = helpFXDescription(fxIdx);
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
