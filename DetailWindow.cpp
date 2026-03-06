/*  DetailWindow.cpp – Coffee Toolkit */

#include "DetailWindow.h"
#include "Constants.h"
#include "Settings.h"

#include <Application.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>

DetailWindow::DetailWindow(const char* title)
    : BWindow(BRect(220, 170, 460, 280), title,
              B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
    BStringView* lbl = new BStringView("placeholder", "Coming soon!");
    lbl->SetFont(be_bold_font);
    lbl->SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    BMenuBar* menuBar = new BMenuBar("menubar");
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
                                   new BMessage(B_ABOUT_REQUESTED)));
    menuBar->AddItem(helpMenu);
    CoffeeSettings::BuildSettingsMenu(menuBar);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .AddGroup(B_VERTICAL, 0)
            .SetInsets(30, 30, 30, 30)
            .AddGlue()
            .Add(lbl)
            .AddGlue()
            .End();

    ResizeTo(GetLayout()->PreferredSize().width  + 60,
             GetLayout()->PreferredSize().height + 60);
    CenterOnScreen();
}

void DetailWindow::MessageReceived(BMessage* msg)
{
    if (CoffeeSettings::Get()->HandleSettingsMessage(msg))
        return;
    BWindow::MessageReceived(msg);
}
