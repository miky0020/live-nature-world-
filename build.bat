@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

echo ============================================
echo  LIVE NATURE - Build Script
echo ============================================
echo.

:: ---- Find g++ ----
set "GPP="
if exist "C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" (
    set "GPP=C:\Program Files\CodeBlocks\MinGW\bin\g++.exe"
    goto :found_gpp
)
if exist "C:\Program Files (x86)\CodeBlocks\MinGW\bin\g++.exe" (
    set "GPP=C:\Program Files (x86)\CodeBlocks\MinGW\bin\g++.exe"
    goto :found_gpp
)
for %%X in (g++.exe) do (
    if not "%%~$PATH:X"=="" ( set "GPP=g++" & goto :found_gpp )
)
echo ERROR: Cannot find g++.exe
pause & exit /b 1

:found_gpp
echo Compiler: !GPP!
echo.

:: ---- FreeGLUT: always use the x64 subfolder ----
:: The freeglut package has BOTH 32-bit (lib/) and 64-bit (lib/x64/).
:: Our MinGW is 64-bit so we MUST use lib/x64/
set "GLUT_INC=%~dp0freeglut\include"
set "GLUT_LIB=%~dp0freeglut\lib\x64"

if not exist "!GLUT_INC!\GL\freeglut.h" (
    echo ERROR: Cannot find freeglut\include\GL\freeglut.h
    echo Make sure the freeglut folder is next to build.bat
    pause & exit /b 1
)
if not exist "!GLUT_LIB!\libfreeglut.a" (
    echo ERROR: Cannot find freeglut\lib\x64\libfreeglut.a
    echo The x64 subfolder is missing from your freeglut package.
    pause & exit /b 1
)

echo FreeGLUT include: !GLUT_INC!
echo FreeGLUT lib:     !GLUT_LIB!  [64-bit]
echo.

:: ---- Copy the 64-bit DLL next to the exe ----
if not exist bin mkdir bin
if exist "%~dp0freeglut\bin\x64\freeglut.dll" (
    copy /y "%~dp0freeglut\bin\x64\freeglut.dll" "%~dp0bin\freeglut.dll" >nul
    echo Copied 64-bit freeglut.dll to bin\
)

echo Compiling... (takes 10-30 seconds, please wait)
echo.

"!GPP!" -std=c++11 -O2 -m64 ^
 -DLIVINGISLAND_USE_MINIAUDIO ^
 -Iinclude ^
 -I"!GLUT_INC!" ^
 main.cpp ^
 src\Application.cpp ^
 src\AudioManager.cpp ^
 src\Camera.cpp ^
 src\Campfire.cpp ^
 src\Character.cpp ^
 src\Player.cpp ^
 src\Interaction.cpp ^
 src\Fishing.cpp ^
 src\Swing.cpp ^
 src\Tent.cpp ^
 src\CharacterManager.cpp ^
 src\Environment.cpp ^
 src\Frustum.cpp ^
 src\Input.cpp ^
 src\Lighting.cpp ^
 src\ParticleSystem.cpp ^
 src\Primitives.cpp ^
 src\Sakura.cpp ^
 src\Sky.cpp ^
 src\Terrain.cpp ^
 src\Tree.cpp ^
 src\UI.cpp ^
 src\Utilities.cpp ^
 src\Vegetation.cpp ^
 src\Water.cpp ^
 src\Weather.cpp ^
 src\World.cpp ^
 -o bin\LiveNature.exe ^
 -L"!GLUT_LIB!" ^
 -lfreeglut -lopengl32 -lglu32 -lwinmm -lgdi32 ^
 -static-libstdc++ -static-libgcc -static -lm

if !errorlevel!==0 (
    echo.
    echo ============================================
    echo  Build successful: bin\LiveNature.exe
    echo ============================================
    echo.
    echo Controls: WASD move, Mouse look, TAB sliders, ESC quit
    echo.
    echo Launching...
    cd bin
    LiveNature.exe
    cd ..
) else (
    echo.
    echo ============================================
    echo  BUILD FAILED - see errors above
    echo ============================================
    echo.
)
pause
endlocal
