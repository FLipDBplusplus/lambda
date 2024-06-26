#include "exports.h"

#include <ion.h>

#include "../shared/display.h"
#include "../shared/journal.h"
#include "../shared/keyboard.h"
#include "../shared/window.h"

const char* IonSoftwareVersion() { return Ion::epsilonVersion(); }

const char* IonPatchLevel() { return Ion::patchLevel(); }

void IonSimulatorKeyboardKeyDown(int keyNumber) {
  Ion::Keyboard::Key key = static_cast<Ion::Keyboard::Key>(keyNumber);
  Ion::Simulator::Keyboard::keyDown(key);
}

void IonSimulatorKeyboardKeyUp(int keyNumber) {
  Ion::Keyboard::Key key = static_cast<Ion::Keyboard::Key>(keyNumber);
  Ion::Simulator::Keyboard::keyUp(key);
}

void IonSimulatorEventsPushEvent(int eventNumber) {
  Ion::Events::Journal* j = Ion::Simulator::Journal::replayJournal();
  if (j != nullptr) {
    Ion::Events::Event event(eventNumber);
    j->pushEvent(event);
    Ion::Events::replayFrom(j);
  }
}
