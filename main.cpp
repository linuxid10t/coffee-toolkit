/*  main.cpp – Coffee Toolkit
 *
 *  Application entry point and CoffeeToolkitApp class.
 *
 *  Build (from project directory):
 *      make
 *  or manually:
 *      g++ -std=c++17 -o coffee_toolkit \
 *          main.cpp MainWindow.cpp BrewRatioWindow.cpp \
 *          ExtractionWindow.cpp RoastColorWindow.cpp DetailWindow.cpp \
 *          -lbe -lroot -ltranslation -ltracker
 *
 *  Author: Yun (Hangzhou)
 */

#include "MainWindow.h"

#include <Application.h>

class CoffeeToolkitApp : public BApplication {
public:
    CoffeeToolkitApp()
        : BApplication("application/x-vnd.Hangzhou.CoffeeToolkit"),
          fMain(nullptr) {}

    void ReadyToRun() override {
        fMain = new MainWindow();
        fMain->Show();
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
