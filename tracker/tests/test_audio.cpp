#include "doctest.h"
#include "i_audio.h"
#include "audio_device.h"
#include "audio_mock.h"

TEST_SUITE("audio") {

static void dummyCallback(int16_t*, int) {}

TEST_CASE("mock records setup/pause/cleanup") {
  AudioMock mock;

  CHECK(mock.setup(dummyCallback, 44100, 512) == 1);
  CHECK(mock.setupCalls == 1);
  CHECK(mock.lastCallback == dummyCallback);
  CHECK(mock.lastSampleRate == 44100);
  CHECK(mock.lastBufferSize == 512);

  mock.pause(1);
  CHECK(mock.pauseCalls == 1);
  CHECK(mock.lastPaused == 1);

  mock.pause(0);
  CHECK(mock.lastPaused == 0);

  mock.cleanup();
  CHECK(mock.cleanupCalls == 1);
}

TEST_CASE("mock setup can report failure") {
  AudioMock mock;
  mock.setupResult = 0;
  CHECK(mock.setup(dummyCallback, 44100, 512) == 0);
}

TEST_CASE("global audioDevice instance can be swapped and restored") {
  IAudio* previous = audioDevice;

  AudioMock mock;
  audioDevice = &mock;

  audioDevice->pause(1);
  CHECK(mock.pauseCalls == 1);

  audioDevice = previous; // restore
  CHECK(audioDevice == previous);
}

} // TEST_SUITE("audio")
