/*  main.cpp – Coffee Toolkit
 *
 *  Application entry point and CoffeeToolkitApp class.
 *
 *  Build (from project directory):
 *      make
 *  or manually:
 *      g++ -std=c++17 -I/boot/system/develop/headers/private \
 *          -o coffee_toolkit \
 *          main.cpp MainWindow.cpp BrewRatioWindow.cpp \
 *          ExtractionWindow.cpp RoastColorWindow.cpp DetailWindow.cpp \
 *          -lbe -lroot -ltranslation -ltracker -lshared
 *
 *  Author: David Masson
 */

#include "MainWindow.h"

#include <interface/AboutWindow.h>
#include <Application.h>

class CoffeeToolkitApp : public BApplication {
public:
    CoffeeToolkitApp()
        : BApplication("application/x-vnd.DavidMasson.CoffeeToolkit"),
          fMain(nullptr) {}

    void ReadyToRun() override {
        fMain = new MainWindow();
        fMain->Show();
    }

    void AboutRequested() override {
        BAboutWindow* about = new BAboutWindow(
            "Coffee Toolkit", "application/x-vnd.DavidMasson.CoffeeToolkit");
        about->AddDescription(
            "A native Haiku coffee brewing toolkit.\n\n"
            "Tools: Brew Ratio Calculator, Extraction Calculator, "
            "and Roast Color Analyzer.");
        about->AddCopyright(2025, "David Masson");
        about->SetLook(B_TITLED_WINDOW_LOOK);
        about->Show();
    }

    bool QuitRequested() override {
        return true;
    }

private:
    MainWindow* fMain;
};

int main()
{
    return CoffeeToolkitApp().Run();
}
