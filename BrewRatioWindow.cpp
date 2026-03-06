/*  BrewRatioWindow.cpp – Coffee Toolkit */

#include "BrewRatioWindow.h"
#include "Constants.h"

#include <Application.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <stdio.h>
#include <stdlib.h>

BrewRatioWindow::BrewRatioWindow()
    : BWindow(BRect(200, 150, 520, 420), "Brew Ratio Calculator",
              B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fCustomMode(false),
      fCurrentRatio(15.0f)
{
    fRatioMenu = new BPopUpMenu("1:15  (standard)");

    struct Preset { const char* label; float ratio; };
    Preset presets[] = {
        { "1:12  (strong)",     12.0f },
        { "1:15  (standard)",   15.0f },
        { "1:16  (light)",      16.0f },
        { "1:17  (very light)", 17.0f },
        { "Custom...",           0.0f },
    };
    for (auto& p : presets) {
        BMessage* m = new BMessage(MSG_RATIO_PICKED);
        m->AddFloat("ratio", p.ratio);
        fRatioMenu->AddItem(new BMenuItem(p.label, m));
    }
    fRatioMenu->ItemAt(1)->SetMarked(true);
    fRatioField = new BMenuField("ratio_field", "Brew ratio:", fRatioMenu);

    fCustomRatioLbl = new BStringView("custom_ratio_lbl", "Custom 1:X:");
    fCustomRatioLbl->SetExplicitMinSize(BSize(kLblW, B_SIZE_UNSET));
    fCustomRatioLbl->SetExplicitMaxSize(BSize(kLblW, B_SIZE_UNSET));
    fCustomRatioCtl = new BTextControl("custom_ratio", "", "15", nullptr);
    fCustomRatioCtl->Hide();
    fCustomRatioLbl->Hide();

    fWaterLbl = new BStringView("water_lbl", "Water (ml):");
    fWaterLbl->SetExplicitMinSize(BSize(kLblW, B_SIZE_UNSET));
    fWaterLbl->SetExplicitMaxSize(BSize(kLblW, B_SIZE_UNSET));
    fWaterCtl = new BTextControl("water_ml", "", "250", nullptr);

    fCalcBtn = new BButton("calc_btn", "Calculate", new BMessage(MSG_CALCULATE));
    fCalcBtn->MakeDefault(true);

    fResultView = new BStringView("result", "");
    fResultView->SetFont(be_bold_font);
    fResultView->SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    BMenuBar* menuBar = new BMenuBar("menubar");
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
                                   new BMessage(B_ABOUT_REQUESTED)));
    menuBar->AddItem(helpMenu);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .AddGroup(B_VERTICAL, kPad)
            .SetInsets(kPad*2, kPad*2, kPad*2, kPad*2)
            .Add(fRatioField)
            .AddGroup(B_HORIZONTAL, 4)
                .Add(fCustomRatioLbl)
                .Add(fCustomRatioCtl)
                .End()
            .AddGroup(B_HORIZONTAL, 4)
                .Add(fWaterLbl)
                .Add(fWaterCtl)
                .End()
            .AddGlue()
            .Add(fCalcBtn)
            .AddGlue()
            .Add(fResultView)
            .End();

    ResizeTo(GetLayout()->PreferredSize().width + 40,
             GetLayout()->PreferredSize().height + 20);
    CenterOnScreen();
}

float BrewRatioWindow::CurrentRatio() const
{
    if (fCustomMode) {
        float v = atof(fCustomRatioCtl->Text());
        return (v > 0.0f) ? v : 15.0f;
    }
    return fCurrentRatio;
}

void BrewRatioWindow::Calculate()
{
    float water = atof(fWaterCtl->Text());
    if (water <= 0.0f) {
        fResultView->SetText("Please enter a valid water amount.");
        return;
    }
    float ratio  = CurrentRatio();
    float coffee = water / ratio;
    char buf[128];
    snprintf(buf, sizeof(buf),
             "%.0f ml water  /  1:%.1f  =  %.1f g coffee grounds",
             water, ratio, coffee);
    fResultView->SetText(buf);
}

void BrewRatioWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case MSG_RATIO_PICKED: {
            float ratio = 0.0f;
            msg->FindFloat("ratio", &ratio);
            if (ratio <= 0.0f) {
                fCustomMode = true;
                fCustomRatioLbl->Show();
                fCustomRatioCtl->Show();
            } else {
                fCustomMode   = false;
                fCurrentRatio = ratio;
                fCustomRatioLbl->Hide();
                fCustomRatioCtl->Hide();
            }
            fResultView->SetText("");
            break;
        }
        case MSG_CALCULATE:
            Calculate();
            break;
        default:
            BWindow::MessageReceived(msg);
    }
}
