#include "input_utils.h"

int InputUtils::inputCodeToKey(InputCode input, KeyMapping& mapping) {
  // Logical buttons are not remappable
  if (input.deviceType == InputDeviceType::logical) {
    return input.code;
  }

  // Check key mapping for keyboard and gamepad inputs
  for (int i = 0; i < 3; i++) {
    if (mapping.keyUp[i].deviceType == input.deviceType && mapping.keyUp[i].code == input.code) return keyUp;
    if (mapping.keyLeft[i].deviceType == input.deviceType && mapping.keyLeft[i].code == input.code) return keyLeft;
    if (mapping.keyRight[i].deviceType == input.deviceType && mapping.keyRight[i].code == input.code) return keyRight;
    if (mapping.keyEdit[i].deviceType == input.deviceType && mapping.keyEdit[i].code == input.code) return keyEdit;
    if (mapping.keyOpt[i].deviceType == input.deviceType && mapping.keyOpt[i].code == input.code) return keyOpt;
    if (mapping.keyPlay[i].deviceType == input.deviceType && mapping.keyPlay[i].code == input.code) return keyPlay;
    if (mapping.keyShift[i].deviceType == input.deviceType && mapping.keyShift[i].code == input.code) return keyShift;
  }

  // Return keyUnmapped for any input that doesn't match a mapping
  return keyUnmapped;
}
