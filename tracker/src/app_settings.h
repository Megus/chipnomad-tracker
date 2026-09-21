#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__

#include "file_system.h"
#include "input_utils.h"
#include "tracker_constants.h"
#include "chipnomad_constants.h"

// ChipNomad color theme. RGB colors
struct ColorTheme {
  int background;
  int textEmpty;
  int textInfo;
  int textDefault;
  int textValue;
  int textTitles;
  int playMarkers;
  int cursor;
  int selection;
  int warning;
};

// Key mapping: 8 buttons x 3 keys each
struct KeyMapping {
  InputCode keyUp[3];
  InputCode keyDown[3];
  InputCode keyLeft[3];
  InputCode keyRight[3];
  InputCode keyEdit[3];
  InputCode keyOpt[3];
  InputCode keyPlay[3];
  InputCode keyShift[3];
};

class AppSettings {
  public:
    // Graphic settings
    int screenWidth; // Screen width in pixels
    int screenHeight; // Screen height in pixels

    // Audio settings
    int audioSampleRate; // Sample rate
    int audioBufferSize; // Mix buffer size (samples)
    float mixVolume; // Mix volume [0.0 - 1.0]
    chipnomad::EmulationQuality quality; // Chip emulation quality

    // Input settings
    int doubleTapFrames; // Max delay to detect a double tap (frames)
    int keyRepeatDelay; // Delay in frames before key press repeat
    int keyRepeatSpeed; // Delay in frames between key repeats
    KeyMapping keyMapping;

    // Color theme
    ColorTheme colorTheme;

    // Chip-specific settings
    // AY Settings (TODO: Wrap in a struct?)
    bool aySampleDithering; // Is AY sample dithering enabled?
    bool pitchConflictWarning; // Is pitch conflict warning enabled?

    // Names and paths
    char themeName[kThemeNameLength + 1];
    char projectFilename[kFileNameLength + 1];
    char projectPath[kPathMaxLength + 1];
    char pitchTablePath[kPathMaxLength + 1];
    char instrumentPath[kPathMaxLength + 1];
    char themePath[kPathMaxLength + 1];
    char fontPath[kPathMaxLength + 1];
    char fontFolderPath[kPathMaxLength + 1];
    char samplePath[kPathMaxLength + 1];
    char wavetablePath[kPathMaxLength + 1];

    AppSettings();
    ~AppSettings() = default;

    bool saveSettings(const char* path);
    bool loadSettings(const char* path);
    bool saveColorTheme(const char* path);
    bool loadColorTheme(const char* path);
    void resetColorTheme();
};

#endif // __APP_SETTINGS_H__