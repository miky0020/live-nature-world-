// ============================================================================
//  LIVE NATURE
//  An interactive 3D natural-world simulation built with C++, OpenGL and
//  FreeGLUT.
//
//  main() stays deliberately tiny: create the application, initialize it, run
//  it. Everything else lives in Application and the systems it owns.
// ============================================================================
#include "Application.h"
#include "Utilities.h"

int main(int argc, char** argv) {
    Application& app = Application::instance();

    if (!app.initialize(argc, argv)) {
        util::logError("Initialization failed. Exiting.");
        return 1;
    }

    app.run();
    return 0;
}
