// ============================================================================
//  GLIncludes.h - single place where OpenGL / GLUT headers are pulled in.
//  Every other file includes THIS instead of <GL/...> directly, so platform
//  differences never leak into the rest of the project.
// ============================================================================
#ifndef LIVINGISLAND_GLINCLUDES_H
#define LIVINGISLAND_GLINCLUDES_H

#ifdef _WIN32
    // windows.h must come before gl.h on MinGW / MSVC, and we do not want it
    // to drag in the whole Win32 kitchen sink.
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/freeglut.h>
#endif

#endif // LIVINGISLAND_GLINCLUDES_H
