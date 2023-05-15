#include "PedalEngine.h"

PedalEngine::PedalEngine() {};

unsigned long PedalEngine::getPosition() {
  return position;
}

unsigned long PedalEngine::getMinPosition() {
  return min_position;
}

unsigned long PedalEngine::getMaxPosition() {
  return max_position;
}