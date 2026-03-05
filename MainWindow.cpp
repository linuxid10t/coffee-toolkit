/*  MainWindow.cpp – Coffee Toolkit */

#include "MainWindow.h"
#include "Constants.h"
#include "BrewRatioWindow.h"
#include "ExtractionWindow.h"
#include "RoastColorWindow.h"
#include "DetailWindow.h"

#include <InterfaceDefs.h>
#include <Message.h>
#include <Rect.h>
#include <View.h>

MainWindow::MainWindow()
    : BWindow(BRect(100, 100,
                    100 + kPad + 4*kBtnW + 3*kPad + kPad,
                    100 + kPad + kBtnH   + kPad),
              "Coffee Toolkit",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE)
{
    BRect content = Bounds();
    BView* bg = new BView(content, "background", B_FOLLOW_ALL, B_WILL_DRAW);
    bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    AddChild(bg);

    auto MakeBtn = [&](const char* label, uint32 cmd, int col) -> BButton* {
        float x = kPad + col * (kBtnW + kPad);
        BButton* btn = new BButton(
            BRect(x, kPad, x + kBtnW - 1, kPad + kBtnH - 1),
            "btn", label, new BMessage(cmd));
        bg->AddChild(btn);
        return btn;
    };

    fBrewRatioBtn  = MakeBtn("Brew Ratio\nCalculator", MSG_BREW_RATIO,  0);
    fParticleBtn   = MakeBtn("Particle\nAnalyzer",     MSG_PARTICLE,    1);
    fExtractionBtn = MakeBtn("Extraction\n%",          MSG_EXTRACTION,  2);
    fRoastBtn      = MakeBtn("Roast Color\nAnalyzer",  MSG_ROAST_COLOR, 3);

    CenterOnScreen();
}

void MainWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case MSG_BREW_RATIO:
            (new BrewRatioWindow())->Show();
            break;
        case MSG_EXTRACTION:
            (new ExtractionWindow())->Show();
            break;
        case MSG_PARTICLE:
            (new DetailWindow("Particle Analyzer"))->Show();
            break;
        case MSG_ROAST_COLOR:
            (new RoastColorWindow())->Show();
            break;
        default:
            BWindow::MessageReceived(msg);
    }
}
