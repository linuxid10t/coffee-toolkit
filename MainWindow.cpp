/*  MainWindow.cpp – Coffee Toolkit */

#include "MainWindow.h"
#include "Constants.h"
#include "BrewRatioWindow.h"
#include "ExtractionWindow.h"
#include "RoastColorWindow.h"
#include "DetailWindow.h"

#include <Application.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Rect.h>
#include <View.h>

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 200, 200),
              "Coffee Toolkit",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
    BMenuBar* menuBar = new BMenuBar("menubar");
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
                                   new BMessage(B_ABOUT_REQUESTED)));
    menuBar->AddItem(helpMenu);

    auto MakeBtn = [&](const char* label, uint32 cmd) -> BButton* {
        BButton* btn = new BButton("btn", label, new BMessage(cmd));
        btn->SetExplicitMinSize(BSize(kBtnW, kBtnH));
        btn->SetExplicitMaxSize(BSize(kBtnW, kBtnH));
        return btn;
    };

    fBrewRatioBtn  = MakeBtn("Brew Ratio\nCalculator", MSG_BREW_RATIO);
    fParticleBtn   = MakeBtn("Particle\nAnalyzer",     MSG_PARTICLE);
    fExtractionBtn = MakeBtn("Extraction\n%",          MSG_EXTRACTION);
    fRoastBtn      = MakeBtn("Roast Color\nAnalyzer",  MSG_ROAST_COLOR);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .AddGroup(B_HORIZONTAL, kPad)
            .SetInsets(kPad, kPad, kPad, kPad)
            .Add(fBrewRatioBtn)
            .Add(fParticleBtn)
            .Add(fExtractionBtn)
            .Add(fRoastBtn)
            .End();

    ResizeTo(GetLayout()->PreferredSize().width,
             GetLayout()->PreferredSize().height);
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
