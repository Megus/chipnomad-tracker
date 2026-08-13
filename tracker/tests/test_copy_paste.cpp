#include "doctest.h"
#include "chipnomad_lib.h"
#include "copy_paste.h"
#include "project_instruments.h"

#include <cstring>
#include <cstdlib>

// Stubs for symbols required by copy_paste.cpp but not used in these tests
int cInstrument = 0;

void getSelectionBounds(ScreenData*, int*, int*, int*, int*) {}

TEST_SUITE("copy_paste") {

struct CopyPasteFixture {
  ChipNomadState* state;

  CopyPasteFixture() {
    state = chipnomadCreate();
    projectInit(&state->project);
    chipnomadState = state;
    cInstrument = 0;
    resetCopyBuffers();
  }

  ~CopyPasteFixture() {
    // Free any sample data that tests allocated
    for (int i = 0; i < PROJECT_MAX_INSTRUMENTS; i++) {
      if (state->project.instruments[i].type == InstrumentType::AYSample) {
        getInstrumentFunctions(InstrumentType::AYSample).free(&state->project.instruments[i]);
      }
    }
    chipnomadDestroy(state);
    chipnomadState = nullptr;
  }

  // Helper to set up an AYSample instrument with test data
  void setupSampleInstrument(int idx, uint16_t length) {
    Instrument* inst = &state->project.instruments[idx];
    getInstrumentFunctions(InstrumentType::AYSample).init(inst);
    std::strcpy(inst->name, "TestSample");

    uint8_t* data = (uint8_t*)std::malloc(length);
    for (uint16_t i = 0; i < length; i++) {
      data[i] = (uint8_t)(i & 0xFF);
    }
    inst->chip.aySample.sampleData = data;
    inst->chip.aySample.fileLength = length;
    inst->chip.aySample.sampleLength = length;
    inst->chip.aySample.sampleRate = 8000;
  }
};

// ============================================================================
// Groove copy/paste
// ============================================================================

TEST_CASE_FIXTURE(CopyPasteFixture, "groove copy and paste") {
  Groove* g = &state->project.grooves[0];
  g->speed[0] = 6;
  g->speed[1] = 4;
  g->speed[2] = 8;
  g->speed[3] = 3;

  copyGroove(0, 0, 3, 0);
  int pasted = pasteGroove(1, 2);

  CHECK(pasted == 4);
  CHECK(state->project.grooves[1].speed[2] == 6);
  CHECK(state->project.grooves[1].speed[3] == 4);
  CHECK(state->project.grooves[1].speed[4] == 8);
  CHECK(state->project.grooves[1].speed[5] == 3);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "groove cut zeroes source") {
  Groove* g = &state->project.grooves[0];
  g->speed[0] = 5;
  g->speed[1] = 7;

  copyGroove(0, 0, 1, 1);  // isCut = 1

  CHECK(g->speed[0] == 0);
  CHECK(g->speed[1] == 0);

  // Buffer still holds original values
  int pasted = pasteGroove(2, 0);
  CHECK(pasted == 2);
  CHECK(state->project.grooves[2].speed[0] == 5);
  CHECK(state->project.grooves[2].speed[1] == 7);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "groove paste truncates at boundary") {
  Groove* g = &state->project.grooves[0];
  for (int i = 0; i < 16; i++) g->speed[i] = (uint8_t)(i + 1);

  copyGroove(0, 0, 15, 0);  // Copy all 16 rows
  int pasted = pasteGroove(1, 14);  // Paste at row 14, only 2 fit

  CHECK(pasted == 2);
  CHECK(state->project.grooves[1].speed[14] == 1);
  CHECK(state->project.grooves[1].speed[15] == 2);
}

// ============================================================================
// Chain copy/paste
// ============================================================================

TEST_CASE_FIXTURE(CopyPasteFixture, "chain copy and paste both columns") {
  Chain* c = &state->project.chains[0];
  c->rows[0].phrase = 0x10;
  c->rows[0].transpose = 3;
  c->rows[1].phrase = 0x20;
  c->rows[1].transpose = 254;  // uint8_t wraps for signed interpretation

  copyChain(0, 0, 0, 1, 1, 0);  // startCol=0, endCol=1
  int pasted = pasteChain(1, 0, 4);

  CHECK(pasted == 2);
  CHECK(state->project.chains[1].rows[4].phrase == 0x10);
  CHECK(state->project.chains[1].rows[4].transpose == 3);
  CHECK(state->project.chains[1].rows[5].phrase == 0x20);
  CHECK(state->project.chains[1].rows[5].transpose == 254);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "chain copy phrase column only") {
  Chain* c = &state->project.chains[0];
  c->rows[2].phrase = 0x42;
  c->rows[2].transpose = 5;

  copyChain(0, 0, 2, 0, 2, 0);  // startCol=0, endCol=0 (phrase only)
  int pasted = pasteChain(1, 0, 0);

  CHECK(pasted == 1);
  CHECK(state->project.chains[1].rows[0].phrase == 0x42);
  // Transpose should not be pasted
  CHECK(state->project.chains[1].rows[0].transpose == 0);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "chain paste wrong column returns 0") {
  Chain* c = &state->project.chains[0];
  c->rows[0].phrase = 0x10;

  copyChain(0, 0, 0, 0, 0, 0);  // Copy col 0
  int pasted = pasteChain(1, 1, 0);  // Try to paste at col 1

  CHECK(pasted == 0);
}

// ============================================================================
// Phrase copy/paste
// ============================================================================

TEST_CASE_FIXTURE(CopyPasteFixture, "phrase copy and paste note column") {
  Phrase* p = &state->project.phrases[0];
  p->rows[0].note = 36;
  p->rows[1].note = 48;
  p->rows[2].note = 60;

  copyPhrase(0, 0, 0, 0, 2, 0);  // col 0 only, rows 0-2
  int pasted = pastePhrase(1, 0, 5);

  CHECK(pasted == 3);
  CHECK(state->project.phrases[1].rows[5].note == 36);
  CHECK(state->project.phrases[1].rows[6].note == 48);
  CHECK(state->project.phrases[1].rows[7].note == 60);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "phrase copy multiple columns") {
  Phrase* p = &state->project.phrases[0];
  p->rows[0].note = 40;
  p->rows[0].instrument = 5;
  p->rows[0].volume = 0x0C;

  copyPhrase(0, 0, 0, 2, 0, 0);  // cols 0-2, row 0
  int pasted = pastePhrase(1, 0, 0);

  CHECK(pasted == 1);
  CHECK(state->project.phrases[1].rows[0].note == 40);
  CHECK(state->project.phrases[1].rows[0].instrument == 5);
  CHECK(state->project.phrases[1].rows[0].volume == 0x0C);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "phrase cut clears source") {
  Phrase* p = &state->project.phrases[0];
  p->rows[3].note = 55;
  p->rows[3].instrument = 2;

  copyPhrase(0, 0, 3, 1, 3, 1);  // cols 0-1, row 3, isCut=1

  CHECK(p->rows[3].note == EMPTY_VALUE_8);
  CHECK(p->rows[3].instrument == EMPTY_VALUE_8);
}

// ============================================================================
// Wavetable copy/paste
// ============================================================================

TEST_CASE_FIXTURE(CopyPasteFixture, "wavetable copy and paste") {
  // Fill wavetable 0 with a pattern
  for (int i = 0; i < 32; i++) {
    state->project.ayWavetables[0][i] = (uint8_t)(i * 2);
  }

  copyWavetable(0);
  pasteWavetable(1);

  CHECK(std::memcmp(state->project.ayWavetables[0], state->project.ayWavetables[1], 32) == 0);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "wavetable paste without copy does nothing") {
  // Fill wavetable 0 with known values
  for (int i = 0; i < 32; i++) {
    state->project.ayWavetables[0][i] = 0xAA;
  }

  // Don't copy - resetCopyBuffers was called in fixture
  pasteWavetable(0);

  // Should remain unchanged (paste is a no-op if no valid buffer)
  CHECK(state->project.ayWavetables[0][0] == 0xAA);
}

// ============================================================================
// Instrument copy/paste (sample deep copy)
// ============================================================================

TEST_CASE_FIXTURE(CopyPasteFixture, "paste instrument deep copies sample data") {
  setupSampleInstrument(0, 256);

  Instrument* src = &state->project.instruments[0];
  Instrument* dst = &state->project.instruments[1];

  copyInstrument(0);
  pasteInstrument(1);

  // Destination should have its own buffer
  REQUIRE(dst->chip.aySample.sampleData != nullptr);
  CHECK(dst->chip.aySample.sampleData != src->chip.aySample.sampleData);

  // Content should match
  CHECK(dst->chip.aySample.fileLength == 256);
  CHECK(std::memcmp(src->chip.aySample.sampleData, dst->chip.aySample.sampleData, 256) == 0);

  // Modifying one should not affect the other
  src->chip.aySample.sampleData[0] = 0xAA;
  CHECK(dst->chip.aySample.sampleData[0] != 0xAA);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "clone instrument deep copies sample data") {
  setupSampleInstrument(0, 512);

  Instrument* src = &state->project.instruments[0];

  cloneInstrument(0, 1);

  Instrument* dst = &state->project.instruments[1];

  // Destination should have its own buffer
  REQUIRE(dst->chip.aySample.sampleData != nullptr);
  CHECK(dst->chip.aySample.sampleData != src->chip.aySample.sampleData);

  // Content should match
  CHECK(dst->chip.aySample.fileLength == 512);
  CHECK(std::memcmp(src->chip.aySample.sampleData, dst->chip.aySample.sampleData, 512) == 0);

  // Modifying one should not affect the other
  dst->chip.aySample.sampleData[10] = 0xBB;
  CHECK(src->chip.aySample.sampleData[10] != 0xBB);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "clone instrument to next deep copies sample data") {
  setupSampleInstrument(0, 100);

  Instrument* src = &state->project.instruments[0];

  int newIdx = cloneInstrumentToNext(0);
  REQUIRE(newIdx != EMPTY_VALUE_8);

  Instrument* dst = &state->project.instruments[newIdx];

  REQUIRE(dst->chip.aySample.sampleData != nullptr);
  CHECK(dst->chip.aySample.sampleData != src->chip.aySample.sampleData);
  CHECK(std::memcmp(src->chip.aySample.sampleData, dst->chip.aySample.sampleData, 100) == 0);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "paste non-sample instrument does not allocate") {
  Instrument* src = &state->project.instruments[0];
  getInstrumentFunctions(InstrumentType::AY1).init(src);
  std::strcpy(src->name, "AY1Inst");

  copyInstrument(0);
  pasteInstrument(1);

  Instrument* dst = &state->project.instruments[1];
  CHECK(dst->type == InstrumentType::AY1);
  CHECK(std::strcmp(dst->name, "AY1Inst") == 0);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "paste sample with null data handles gracefully") {
  Instrument* src = &state->project.instruments[0];
  getInstrumentFunctions(InstrumentType::AYSample).init(src);

  CHECK(src->chip.aySample.sampleData == nullptr);

  copyInstrument(0);
  pasteInstrument(1);

  Instrument* dst = &state->project.instruments[1];
  CHECK(dst->type == InstrumentType::AYSample);
  CHECK(dst->chip.aySample.sampleData == nullptr);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "paste instrument copies table") {
  Instrument* src = &state->project.instruments[0];
  getInstrumentFunctions(InstrumentType::AY1).init(src);

  // Set up some table data
  state->project.tables[0].rows[0].volume = 0x0F;
  state->project.tables[0].rows[0].pitchOffset = 12;
  state->project.tables[0].rows[1].volume = 0x0A;

  copyInstrument(0);
  pasteInstrument(1);

  CHECK(state->project.tables[1].rows[0].volume == 0x0F);
  CHECK(state->project.tables[1].rows[0].pitchOffset == 12);
  CHECK(state->project.tables[1].rows[1].volume == 0x0A);
}

// ============================================================================
// Clone functionality
// ============================================================================

TEST_CASE_FIXTURE(CopyPasteFixture, "clone chain copies all rows") {
  Chain* src = &state->project.chains[0];
  src->rows[0].phrase = 0x05;
  src->rows[0].transpose = 2;
  src->rows[1].phrase = 0x0A;
  src->rows[1].transpose = 253;

  int result = cloneChain(0, 1);
  CHECK(result == 1);

  Chain* dst = &state->project.chains[1];
  CHECK(dst->rows[0].phrase == 0x05);
  CHECK(dst->rows[0].transpose == 2);
  CHECK(dst->rows[1].phrase == 0x0A);
  CHECK(dst->rows[1].transpose == 253);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "clone phrase copies all rows") {
  Phrase* src = &state->project.phrases[0];
  src->rows[0].note = 48;
  src->rows[0].instrument = 3;
  src->rows[0].volume = 0x0E;
  src->rows[5].note = 60;

  int result = clonePhrase(0, 1);
  CHECK(result == 1);

  Phrase* dst = &state->project.phrases[1];
  CHECK(dst->rows[0].note == 48);
  CHECK(dst->rows[0].instrument == 3);
  CHECK(dst->rows[0].volume == 0x0E);
  CHECK(dst->rows[5].note == 60);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "clone chain to next finds empty slot") {
  Chain* src = &state->project.chains[0];
  src->rows[0].phrase = 0x01;

  int newIdx = cloneChainToNext(0);
  REQUIRE(newIdx != EMPTY_VALUE_16);
  CHECK(newIdx > 0);
  CHECK(state->project.chains[newIdx].rows[0].phrase == 0x01);
}

TEST_CASE_FIXTURE(CopyPasteFixture, "clone phrase to next finds empty slot") {
  Phrase* src = &state->project.phrases[0];
  src->rows[0].note = 72;

  int newIdx = clonePhraseToNext(0);
  REQUIRE(newIdx != EMPTY_VALUE_16);
  CHECK(newIdx > 0);
  CHECK(state->project.phrases[newIdx].rows[0].note == 72);
}

} // TEST_SUITE("copy_paste")
