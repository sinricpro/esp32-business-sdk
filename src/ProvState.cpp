#include "ProvState.h"

ProvState& ProvState::getInstance() {
  static ProvState instance;  // Static instance created only once
  return instance;
}