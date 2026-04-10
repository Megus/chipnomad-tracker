#include "unity.h"
#include "project.h"
#include <string.h>

static Project p;

void setUp(void) {
  projectInit(&p);
}

void tearDown(void) {
}

// projectInit

void test_projectInit_song_is_empty(void) {
  for (int r = 0; r < PROJECT_MAX_LENGTH; r++)
    for (int c = 0; c < PROJECT_MAX_TRACKS; c++)
      TEST_ASSERT_EQUAL(EMPTY_VALUE_16, p.song[r][c]);
}

void test_projectInit_default_groove(void) {
  TEST_ASSERT_EQUAL(6, p.grooves[0].speed[0]);
  TEST_ASSERT_EQUAL(6, p.grooves[0].speed[1]);
  TEST_ASSERT_EQUAL(EMPTY_VALUE_8, p.grooves[0].speed[2]);
}

void test_projectInit_other_grooves_empty(void) {
  for (int g = 1; g < PROJECT_MAX_GROOVES; g++)
    TEST_ASSERT_TRUE(grooveIsEmpty(&p, g));
}

void test_projectInit_instruments_empty(void) {
  for (int i = 0; i < PROJECT_MAX_INSTRUMENTS; i++)
    TEST_ASSERT_TRUE(instrumentIsEmpty(&p, i));
}

void test_projectInit_phrases_empty(void) {
  for (int i = 0; i < PROJECT_MAX_PHRASES; i++)
    TEST_ASSERT_TRUE(phraseIsEmpty(&p, i));
}

void test_projectInit_chains_empty(void) {
  for (int i = 0; i < PROJECT_MAX_CHAINS; i++)
    TEST_ASSERT_TRUE(chainIsEmpty(&p, i));
}

void test_projectInit_tables_empty(void) {
  for (int i = 0; i < PROJECT_MAX_TABLES; i++)
    TEST_ASSERT_TRUE(tableIsEmpty(&p, i));
}

// instrumentIsEmpty

void test_instrumentIsEmpty_true_for_none(void) {
  TEST_ASSERT_TRUE(instrumentIsEmpty(&p, 0));
}

void test_instrumentIsEmpty_false_for_ay(void) {
  p.instruments[0].type = instAY;
  TEST_ASSERT_FALSE(instrumentIsEmpty(&p, 0));
}

// chainIsEmpty

void test_chainIsEmpty_true_after_init(void) {
  TEST_ASSERT_TRUE(chainIsEmpty(&p, 0));
}

void test_chainIsEmpty_false_with_phrase(void) {
  p.chains[0].rows[5].phrase = 1;
  TEST_ASSERT_FALSE(chainIsEmpty(&p, 0));
}

// phraseIsEmpty

void test_phraseIsEmpty_true_after_init(void) {
  TEST_ASSERT_TRUE(phraseIsEmpty(&p, 0));
}

void test_phraseIsEmpty_false_with_note(void) {
  p.phrases[0].rows[0].note = 42;
  TEST_ASSERT_FALSE(phraseIsEmpty(&p, 0));
}

void test_phraseIsEmpty_false_with_instrument(void) {
  p.phrases[0].rows[3].instrument = 1;
  TEST_ASSERT_FALSE(phraseIsEmpty(&p, 0));
}

void test_phraseIsEmpty_false_with_volume(void) {
  p.phrases[0].rows[7].volume = 10;
  TEST_ASSERT_FALSE(phraseIsEmpty(&p, 0));
}

void test_phraseIsEmpty_false_with_fx(void) {
  p.phrases[0].rows[0].fx[1][0] = fxARP;
  TEST_ASSERT_FALSE(phraseIsEmpty(&p, 0));
}

void test_phraseIsEmpty_false_with_fx_value(void) {
  p.phrases[0].rows[0].fx[2][1] = 0x50;
  TEST_ASSERT_FALSE(phraseIsEmpty(&p, 0));
}

// tableIsEmpty

void test_tableIsEmpty_true_after_init(void) {
  TEST_ASSERT_TRUE(tableIsEmpty(&p, 0));
}

void test_tableIsEmpty_false_with_pitch_flag(void) {
  p.tables[0].rows[0].pitchFlag = 1;
  TEST_ASSERT_FALSE(tableIsEmpty(&p, 0));
}

void test_tableIsEmpty_false_with_pitch_offset(void) {
  p.tables[0].rows[0].pitchOffset = 5;
  TEST_ASSERT_FALSE(tableIsEmpty(&p, 0));
}

void test_tableIsEmpty_false_with_volume(void) {
  p.tables[0].rows[0].volume = 10;
  TEST_ASSERT_FALSE(tableIsEmpty(&p, 0));
}

void test_tableIsEmpty_false_with_fx(void) {
  p.tables[0].rows[0].fx[3][0] = fxVOL;
  TEST_ASSERT_FALSE(tableIsEmpty(&p, 0));
}

// grooveIsEmpty

void test_grooveIsEmpty_true_for_unused(void) {
  TEST_ASSERT_TRUE(grooveIsEmpty(&p, 1));
}

void test_grooveIsEmpty_false_with_speed(void) {
  p.grooves[1].speed[0] = 8;
  TEST_ASSERT_FALSE(grooveIsEmpty(&p, 1));
}

// Clear functions

void test_phraseClear(void) {
  p.phrases[0].rows[0].note = 42;
  p.phrases[0].rows[5].instrument = 1;
  p.phrases[0].rows[10].fx[0][0] = fxARP;
  p.phrases[0].rows[10].fx[0][1] = 0x37;
  phraseClear(&p.phrases[0]);
  TEST_ASSERT_TRUE(phraseIsEmpty(&p, 0));
}

void test_chainClear(void) {
  p.chains[0].rows[0].phrase = 5;
  p.chains[0].rows[0].transpose = 3;
  chainClear(&p.chains[0]);
  TEST_ASSERT_TRUE(chainIsEmpty(&p, 0));
  TEST_ASSERT_EQUAL(0, p.chains[0].rows[0].transpose);
}

void test_instrumentClear(void) {
  p.instruments[0].type = instAY;
  strcpy(p.instruments[0].name, "Test");
  instrumentClear(&p.instruments[0]);
  TEST_ASSERT_TRUE(instrumentIsEmpty(&p, 0));
  TEST_ASSERT_EQUAL_STRING("", p.instruments[0].name);
  TEST_ASSERT_EQUAL(0x01, p.instruments[0].chip.ay.defaultMixer);
}

void test_tableClear(void) {
  p.tables[0].rows[0].pitchFlag = 1;
  p.tables[0].rows[0].pitchOffset = 10;
  p.tables[0].rows[0].fx[0][0] = fxVOL;
  tableClear(&p.tables[0]);
  TEST_ASSERT_TRUE(tableIsEmpty(&p, 0));
}

// noteName

void test_noteName_off(void) {
  TEST_ASSERT_EQUAL_STRING("OFF", noteName(&p, NOTE_OFF));
}

void test_noteName_empty(void) {
  TEST_ASSERT_EQUAL_STRING("---", noteName(&p, EMPTY_VALUE_8));
}

void test_noteName_out_of_range(void) {
  p.pitchTable.length = 12;
  TEST_ASSERT_EQUAL_STRING("---", noteName(&p, 13));
}

void test_noteName_valid(void) {
  p.pitchTable.length = 12;
  strcpy(p.pitchTable.noteNames[0], "C-4");
  TEST_ASSERT_EQUAL_STRING("C-4", noteName(&p, 0));
}

// projectGetChipTracks / projectGetTotalTracks

void test_getChipTracks_ay(void) {
  TEST_ASSERT_EQUAL(3, projectGetChipTracks(&p, 0));
}

void test_getTotalTracks_single_chip(void) {
  p.chipsCount = 1;
  TEST_ASSERT_EQUAL(3, projectGetTotalTracks(&p));
}

void test_getTotalTracks_multiple_chips(void) {
  p.chipsCount = 3;
  TEST_ASSERT_EQUAL(9, projectGetTotalTracks(&p));
}

// fillFXNames

void test_fillFXNames_common(void) {
  fillFXNames();
  TEST_ASSERT_EQUAL_STRING("ARP", fxNames[fxARP].name);
  TEST_ASSERT_EQUAL_STRING("HOP", fxNames[fxHOP].name);
  TEST_ASSERT_EQUAL_STRING("VOL", fxNames[fxVOL].name);
}

void test_fillFXNames_ay(void) {
  fillFXNames();
  TEST_ASSERT_EQUAL_STRING("AYM", fxNames[fxAYM].name);
  TEST_ASSERT_EQUAL_STRING("EPH", fxNames[fxEPH].name);
}

void test_fillFXNames_unknown(void) {
  fillFXNames();
  TEST_ASSERT_EQUAL_STRING("---", fxNames[200].name);
}

int main(void) {
  UNITY_BEGIN();
  // projectInit
  RUN_TEST(test_projectInit_song_is_empty);
  RUN_TEST(test_projectInit_default_groove);
  RUN_TEST(test_projectInit_other_grooves_empty);
  RUN_TEST(test_projectInit_instruments_empty);
  RUN_TEST(test_projectInit_phrases_empty);
  RUN_TEST(test_projectInit_chains_empty);
  RUN_TEST(test_projectInit_tables_empty);
  // isEmpty
  RUN_TEST(test_instrumentIsEmpty_true_for_none);
  RUN_TEST(test_instrumentIsEmpty_false_for_ay);
  RUN_TEST(test_chainIsEmpty_true_after_init);
  RUN_TEST(test_chainIsEmpty_false_with_phrase);
  RUN_TEST(test_phraseIsEmpty_true_after_init);
  RUN_TEST(test_phraseIsEmpty_false_with_note);
  RUN_TEST(test_phraseIsEmpty_false_with_instrument);
  RUN_TEST(test_phraseIsEmpty_false_with_volume);
  RUN_TEST(test_phraseIsEmpty_false_with_fx);
  RUN_TEST(test_phraseIsEmpty_false_with_fx_value);
  RUN_TEST(test_tableIsEmpty_true_after_init);
  RUN_TEST(test_tableIsEmpty_false_with_pitch_flag);
  RUN_TEST(test_tableIsEmpty_false_with_pitch_offset);
  RUN_TEST(test_tableIsEmpty_false_with_volume);
  RUN_TEST(test_tableIsEmpty_false_with_fx);
  RUN_TEST(test_grooveIsEmpty_true_for_unused);
  RUN_TEST(test_grooveIsEmpty_false_with_speed);
  // Clear
  RUN_TEST(test_phraseClear);
  RUN_TEST(test_chainClear);
  RUN_TEST(test_instrumentClear);
  RUN_TEST(test_tableClear);
  // noteName
  RUN_TEST(test_noteName_off);
  RUN_TEST(test_noteName_empty);
  RUN_TEST(test_noteName_out_of_range);
  RUN_TEST(test_noteName_valid);
  // Track counts
  RUN_TEST(test_getChipTracks_ay);
  RUN_TEST(test_getTotalTracks_single_chip);
  RUN_TEST(test_getTotalTracks_multiple_chips);
  // fillFXNames
  RUN_TEST(test_fillFXNames_common);
  RUN_TEST(test_fillFXNames_ay);
  RUN_TEST(test_fillFXNames_unknown);
  return UNITY_END();
}
