#include "../external/unity/unity.h"
#include "../src/project.h"
#include <string.h>

void setUp(void) {
    projectInit(&project);
}

void tearDown(void) {
    // Cleanup after each test
}

void test_instrumentSwap_should_swap_instrument_data(void) {
    // Setup test data
    project.instruments[1].type = instAY;
    strcpy(project.instruments[1].name, "Test1");
    project.instruments[1].tableSpeed = 10;
    project.instruments[1].transposeEnabled = 1;
    
    project.instruments[2].type = instNone;
    strcpy(project.instruments[2].name, "Test2");
    project.instruments[2].tableSpeed = 20;
    project.instruments[2].transposeEnabled = 0;
    
    // Perform swap
    instrumentSwap(1, 2);
    
    // Verify swap
    TEST_ASSERT_EQUAL(instNone, project.instruments[1].type);
    TEST_ASSERT_EQUAL_STRING("Test2", project.instruments[1].name);
    TEST_ASSERT_EQUAL(20, project.instruments[1].tableSpeed);
    TEST_ASSERT_EQUAL(0, project.instruments[1].transposeEnabled);
    
    TEST_ASSERT_EQUAL(instAY, project.instruments[2].type);
    TEST_ASSERT_EQUAL_STRING("Test1", project.instruments[2].name);
    TEST_ASSERT_EQUAL(10, project.instruments[2].tableSpeed);
    TEST_ASSERT_EQUAL(1, project.instruments[2].transposeEnabled);
}

void test_instrumentSwap_should_swap_default_tables(void) {
    // Setup test data for tables
    project.tables[1].pitchFlags[0] = 1;
    project.tables[1].pitchOffsets[0] = 10;
    project.tables[1].volumes[0] = 15;
    
    project.tables[2].pitchFlags[0] = 0;
    project.tables[2].pitchOffsets[0] = 20;
    project.tables[2].volumes[0] = 8;
    
    // Perform swap
    instrumentSwap(1, 2);
    
    // Verify table swap
    TEST_ASSERT_EQUAL(0, project.tables[1].pitchFlags[0]);
    TEST_ASSERT_EQUAL(20, project.tables[1].pitchOffsets[0]);
    TEST_ASSERT_EQUAL(8, project.tables[1].volumes[0]);
    
    TEST_ASSERT_EQUAL(1, project.tables[2].pitchFlags[0]);
    TEST_ASSERT_EQUAL(10, project.tables[2].pitchOffsets[0]);
    TEST_ASSERT_EQUAL(15, project.tables[2].volumes[0]);
}

void test_instrumentSwap_should_handle_same_instrument(void) {
    // Setup test data
    project.instruments[1].type = instAY;
    strcpy(project.instruments[1].name, "Test");
    
    // Perform swap with same instrument
    instrumentSwap(1, 1);
    
    // Verify no change
    TEST_ASSERT_EQUAL(instAY, project.instruments[1].type);
    TEST_ASSERT_EQUAL_STRING("Test", project.instruments[1].name);
}

void test_instrumentSwap_should_handle_invalid_indices(void) {
    // Setup test data
    project.instruments[1].type = instAY;
    strcpy(project.instruments[1].name, "Test");
    
    // Perform swap with invalid index
    instrumentSwap(1, PROJECT_MAX_INSTRUMENTS);
    
    // Verify no change
    TEST_ASSERT_EQUAL(instAY, project.instruments[1].type);
    TEST_ASSERT_EQUAL_STRING("Test", project.instruments[1].name);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_instrumentSwap_should_swap_instrument_data);
    RUN_TEST(test_instrumentSwap_should_swap_default_tables);
    RUN_TEST(test_instrumentSwap_should_handle_same_instrument);
    RUN_TEST(test_instrumentSwap_should_handle_invalid_indices);
    return UNITY_END();
}