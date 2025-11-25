@echo off
echo Building ChipNomad for Windows...

REM Check if MinGW is installed
where mingw32-make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: mingw32-make not found. Please install MinGW and add it to your PATH.
    exit /b 1
)

REM Build the application
mingw32-make windows
if %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed.
    exit /b 1
)

echo Build complete. The executable is located at build\chipnomad.exe

REM Check if SDL_PATH is set
if defined SDL_PATH (
    echo Copying SDL.dll from %SDL_PATH%\bin to build directory...
    if exist "%SDL_PATH%\bin\SDL.dll" (
        copy "%SDL_PATH%\bin\SDL.dll" build\
        echo SDL.dll copied successfully.
    ) else (
        echo Warning: SDL.dll not found at %SDL_PATH%\bin
        echo You may need to manually copy SDL.dll to the build directory.
    )
) else (
    echo Note: SDL_PATH environment variable not set.
    echo If the application fails to run, you may need to copy SDL.dll to the build directory.
    echo You can set SDL_PATH to your SDL installation directory before running this script.
)

echo Done.