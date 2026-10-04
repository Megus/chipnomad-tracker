// Defines the corelib global service instances for the test binary.
//
// In the real app these globals live in the compiled platform sources
// (platforms/<platform>/corelib_input.cpp, corelib_assets.cpp), which are not
// part of the test build. The font global comes from the shared
// corelib_font.cpp which IS linked into the tests.
#include "input_mock.h"
#include "assets_mock.h"
#include "mainloop_mock.h"
#include "input_utils.h"
#include "assets.h"

static InputMock defaultInputMock;
IInput* input = &defaultInputMock;

static AssetsMock defaultAssetsMock;
IAssets* assets = &defaultAssetsMock;

static MainLoopMock defaultMainLoopMock;
IMainLoop* mainLoop = &defaultMainLoopMock;
