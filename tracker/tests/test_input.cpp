#include "doctest.h"
#include "input_utils.h"
#include "common.h"
#include "input_mock.h"

#include <cstring>

TEST_SUITE("input") {

TEST_CASE("mock records initDefaultKeyMapping and returns fixed key name") {
  InputMock mock;
  AppSettings settings;
  mock.initDefaultKeyMapping(&settings);
  CHECK(mock.initCalls == 1);
  CHECK(mock.lastSettings == &settings);

  InputCode code = {InputDeviceType::keyboard, 65};
  CHECK(std::strcmp(mock.getKeyName(code), "?") == 0);
}

TEST_CASE("initDefaultKeyMapping writes into the provided settings, not a global") {
  // Verify the key API change: the mapping is written into the caller's
  // AppSettings. Using the mock we only check the seam; a real InputSDL2 would
  // populate the bindings, but that pulls in SDL and isn't linked into tests.
  InputMock mock;
  AppSettings a;
  AppSettings b;
  mock.initDefaultKeyMapping(&a);
  CHECK(mock.lastSettings == &a);
  mock.initDefaultKeyMapping(&b);
  CHECK(mock.lastSettings == &b);
  CHECK(mock.initCalls == 2);
}

TEST_CASE("global input instance can be swapped and restored") {
  IInput* previous = input;

  InputMock mock;
  input = &mock;

  AppSettings settings;
  input->initDefaultKeyMapping(&settings);
  CHECK(mock.initCalls == 1);
  CHECK(mock.lastSettings == &settings);

  InputCode code = {InputDeviceType::gamepad, 1};
  CHECK(std::strcmp(input->getKeyName(code), "?") == 0);

  input = previous; // restore
  CHECK(input == previous);
}

} // TEST_SUITE("input")
