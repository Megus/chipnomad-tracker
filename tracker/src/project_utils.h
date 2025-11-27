#include <chipnomad_lib.h>

void projectInitAY();


// Does chain have notes?
int8_t chainHasNotes(int chain);
// Does phrase have notes?
int8_t phraseHasNotes(int phrase);
// Instrument name
char* instrumentName(uint8_t instrument);
// Instrument type name
char* instrumentTypeName(uint8_t type);
// Get first note used with an instrument
uint8_t instrumentFirstNote(uint8_t instrument);
// Swap two instruments and their default tables
void instrumentSwap(uint8_t inst1, uint8_t inst2);
// Find empty slots
int findEmptyChain(int start);
int findEmptyPhrase(int start);
int findEmptyInstrument(int start);

