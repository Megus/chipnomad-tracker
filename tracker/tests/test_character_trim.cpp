// Test for character editing string trimming behavior
#include "../external/doctest.h"
#include "../src/string_utils.h"
#include <string.h>

TEST_SUITE("Character Edit Trimming") {
  TEST_CASE("Trim leading spaces") {
    char str[32] = "   Hello";
    trimString(str);
    CHECK(strcmp(str, "Hello") == 0);
  }

  TEST_CASE("Trim trailing spaces") {
    char str[32] = "Hello   ";
    trimString(str);
    CHECK(strcmp(str, "Hello") == 0);
  }

  TEST_CASE("Trim both leading and trailing spaces") {
    char str[32] = "   Hello   ";
    trimString(str);
    CHECK(strcmp(str, "Hello") == 0);
  }

  TEST_CASE("Preserve internal spaces") {
    char str[32] = "  Hello World  ";
    trimString(str);
    CHECK(strcmp(str, "Hello World") == 0);
  }

  TEST_CASE("Handle all spaces string") {
    char str[32] = "     ";
    trimString(str);
    CHECK(strcmp(str, "") == 0);
  }

  TEST_CASE("Handle empty string") {
    char str[32] = "";
    trimString(str);
    CHECK(strcmp(str, "") == 0);
  }

  TEST_CASE("Handle single character") {
    char str[32] = "A";
    trimString(str);
    CHECK(strcmp(str, "A") == 0);
  }

  TEST_CASE("Handle single character with spaces") {
    char str[32] = "  A  ";
    trimString(str);
    CHECK(strcmp(str, "A") == 0);
  }
}
