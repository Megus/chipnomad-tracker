#include "chipnomad_lib.h"

void projectInitAY(Project* p);
// Does chain have notes?
int8_t chainHasNotes(Project* p, int chain);
// Does phrase have notes?
int8_t phraseHasNotes(Project* p, int phrase);
// Instrument name
char* instrumentName(Project* p, uint8_t instrument);
// Instrument type name
char* instrumentTypeName(uint8_t type);
// Get first note used with an instrument
uint8_t instrumentFirstNote(Project* p, uint8_t instrument);
// Swap two instruments and their default tables
void instrumentSwap(Project* p, uint8_t inst1, uint8_t inst2);
// Find empty slots
int findEmptyChain(Project* p, int start);
int findEmptyPhrase(Project* p, int start);
int findEmptyInstrument(Project* p, int start);
