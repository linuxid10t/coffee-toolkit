#pragma once
/*  MainWindow.h – Coffee Toolkit
 *
 *  The application's main window — four large buttons that
 *  open each tool window.
 */

#include <Window.h>
#include <Button.h>

class MainWindow : public BWindow {
public:
    MainWindow();
    void MessageReceived(BMessage* msg) override;

private:
    BButton* fBrewRatioBtn;
    BButton* fParticleBtn;
    BButton* fExtractionBtn;
    BButton* fRoastBtn;
};
