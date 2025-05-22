#include <help.h>
#include <string.h>
#include <stdio.h>
#include <utils.h>
#include <project.h>

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
      sprintf(buffer, "Pitch bend %hhd per step", fx[1]);
      break;
    case fxPSL: // Pitch slide (portamento)
      sprintf(buffer, "Pitch slide for %hhd tics", fx[1]);
      break;
    case fxPIT: // Pitch offset
      sprintf(buffer, "Pitch offset by %hhd", fx[1]);
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
      sprintf(buffer, "Jump to table row %hhX", fx[1] & 0xf);
      break;
    case fxGRV: // Track groove
      sprintf(buffer, "Track groove %hhu", fx[1]);
      break;
    case fxGGR: // Global groove
      sprintf(buffer, "Global groove %hhu", fx[1]);
      break;
    case fxHOP: // Hop
      if (isTable) {
        sprintf(buffer, "Jump to row %hhX in this column", fx[1] & 0xf);
      } else {
        if (fx[1] == 0xff) {
          sprintf(buffer, "Stop playback");
        } else {
          sprintf(buffer, "Jump to song row %s", byteToHex(fx[1]));
        }
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
      sprintf(buffer, "Noise pereiod %s", byteToHex(fx[1]));
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
      if (note >= project.pitchTable.length - project.pitchTable.octaveSize * 4)
        note = project.pitchTable.length - 1 - project.pitchTable.octaveSize * 4;
      sprintf(buffer, "Envelope note %s", noteName(note));
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


char* helpFXDescription(enum FX fxIdx) {
  return "";
}
