/*  DetailWindow.cpp – Coffee Toolkit */

#include "DetailWindow.h"
#include "Constants.h"

#include <LayoutBuilder.h>
#include <StringView.h>

DetailWindow::DetailWindow(const char* title)
    : BWindow(BRect(220, 170, 460, 280), title,
              B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
    BStringView* lbl = new BStringView("placeholder", "Coming soon!");
    lbl->SetFont(be_bold_font);
    lbl->SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(30, 30, 30, 30)
        .AddGlue()
        .Add(lbl)
        .AddGlue();

    ResizeTo(GetLayout()->PreferredSize().width  + 60,
             GetLayout()->PreferredSize().height + 60);
    CenterOnScreen();
}

void DetailWindow::MessageReceived(BMessage* msg)
{
    BWindow::MessageReceived(msg);
}
