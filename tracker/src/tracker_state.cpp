#include "tracker_state.h"

TrackerState::TrackerState() {
  engine = nullptr;
  projectModified = 0;
}

TrackerState::~TrackerState() {
  delete engine;
  engine = nullptr;
}

void TrackerState::initEngine(int sampleRate, chipnomad::ChipFactory factory) {
  delete engine;
  engine = new chipnomad::Engine(factory, sampleRate);
  engine->setProject(&project);
}
