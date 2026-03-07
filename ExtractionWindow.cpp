/*  ExtractionWindow.cpp – Coffee Toolkit */

#include "ExtractionWindow.h"
#include "Constants.h"
#include "Settings.h"

#include <Application.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// -------------------------------------------------------
// ExtractionBarView
// -------------------------------------------------------
const float ExtractionBarView::kBarMax = 35.0f;

ExtractionBarView::ExtractionBarView(BRect frame)
    : BView(frame, "extraction_bar", B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
            B_WILL_DRAW),
      fExtraction(-1.0f),
      fIdealLo(18.0f),
      fIdealHi(22.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void ExtractionBarView::SetExtraction(float extraction,
                                      float idealLo, float idealHi)
{
    fExtraction = extraction;
    fIdealLo    = idealLo;
    fIdealHi    = idealHi;
    Invalidate();
}

void ExtractionBarView::Draw(BRect /*updateRect*/)
{
    CoffeeSettings* s = CoffeeSettings::Get();
    rgb_color bg      = s->ThemePanelBg();
    rgb_color text    = s->ThemeTextColor();
    rgb_color dim     = s->ThemeDimTextColor();
    rgb_color outline = s->ThemeOutlineColor();

    BRect b = Bounds();
    float barTop    = 18.0f;
    float barBottom = 44.0f;
    float barH      = barBottom - barTop;

    // Clear background
    SetHighColor(bg);
    FillRect(b);

    auto pctToX = [&](float pct) -> float {
        return b.left + 2.0f + (pct / kBarMax) * (b.Width() - 4.0f);
    };

    // Background track
    SetHighColor(200, 200, 200);
    FillRect(BRect(b.left+2, barTop, b.right-2, barBottom));

    // Under zone – steel blue
    float xIdLo = pctToX(fIdealLo);
    SetHighColor(100, 140, 200);
    FillRect(BRect(b.left+2, barTop, xIdLo, barBottom));

    // Ideal zone – forest green
    float xIdHi = pctToX(fIdealHi);
    SetHighColor(60, 160, 80);
    FillRect(BRect(xIdLo, barTop, xIdHi, barBottom));

    // Over zone – brick red
    SetHighColor(200, 70, 60);
    FillRect(BRect(xIdHi, barTop, b.right-2, barBottom));

    // Outline
    SetHighColor(outline);
    StrokeRect(BRect(b.left+2, barTop, b.right-2, barBottom));

    // Scale ticks
    BFont small; small.SetSize(9.0f); SetFont(&small);
    SetHighColor(dim);
    for (int pct = 0; pct <= (int)kBarMax; pct += 5) {
        float x = pctToX((float)pct);
        StrokeLine(BPoint(x, barBottom), BPoint(x, barBottom + 4));
        char lbl[8]; snprintf(lbl, sizeof(lbl), "%d%%", pct);
        DrawString(lbl, BPoint(x - 6, barBottom + 14));
    }

    // Zone labels
    SetHighColor(255, 255, 255);
    BFont tiny; tiny.SetSize(8.5f); SetFont(&tiny);
    DrawString("Under", BPoint(b.left + 4, barTop + barH * 0.65f));
    float idealMid = (xIdLo + xIdHi) / 2.0f;
    DrawString("Ideal", BPoint(idealMid - 10, barTop + barH * 0.65f));
    DrawString("Over",  BPoint(xIdHi + 4,    barTop + barH * 0.65f));

    // Pointer
    if (fExtraction >= 0.0f) {
        float x = pctToX(fExtraction);
        SetHighColor(text);
        FillTriangle(BPoint(x, barTop - 1),
                     BPoint(x - 5, barTop - 9),
                     BPoint(x + 5, barTop - 9));
        SetHighColor(outline);
        StrokeLine(BPoint(x, barTop), BPoint(x, barBottom));
        char val[16]; snprintf(val, sizeof(val), "%.1f%%", fExtraction);
        BFont normal; normal.SetSize(10.0f); SetFont(&normal);
        SetHighColor(text);
        DrawString(val, BPoint(x - 12, barTop - 11));
    }
}

// -------------------------------------------------------
// ExtractionWindow
// -------------------------------------------------------
ExtractionWindow::ExtractionWindow()
    : BWindow(BRect(160, 100, 660, 700), "Extraction Calculator",
              B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fUseBrix(false),
      fIsPercolation(true)
{
    fTdsRadio  = new BRadioButton("tds_radio",  "TDS %",
                                  new BMessage(MSG_USE_TDS));
    fBrixRadio = new BRadioButton("brix_radio", "Brix",
                                  new BMessage(MSG_USE_BRIX));
    fTdsRadio->SetValue(B_CONTROL_ON);

    fMeasureLbl = new BStringView("measure_lbl", "TDS (%):");
    fMeasureLbl->SetExplicitMinSize(BSize(kLblW, B_SIZE_UNSET));
    fMeasureLbl->SetExplicitMaxSize(BSize(kLblW, B_SIZE_UNSET));
    fMeasureCtl = new BTextControl("measure", "", "", nullptr);

    fPercRadio = new BRadioButton("perc_radio", "Percolation",
                                  new BMessage(MSG_BREW_PERC));
    fImmsRadio = new BRadioButton("imms_radio", "Immersion",
                                  new BMessage(MSG_BREW_IMMS));
    fPercRadio->SetValue(B_CONTROL_ON);

    fStatusView = new BStringView("status",
                                  "Mode: TDS  |  Percolation  (ideal 18-22%)");
    fStatusView->SetFont(be_bold_font);

    fLiquidLbl = new BStringView("liquid_lbl", "Brew weight (g):");
    fLiquidLbl->SetExplicitMinSize(BSize(kLblW, B_SIZE_UNSET));
    fLiquidLbl->SetExplicitMaxSize(BSize(kLblW, B_SIZE_UNSET));
    fLiquidCtl = new BTextControl("liquid", "", "", nullptr);

    fDoseLbl = new BStringView("dose_lbl", "Coffee dose (g):");
    fDoseLbl->SetExplicitMinSize(BSize(kLblW, B_SIZE_UNSET));
    fDoseLbl->SetExplicitMaxSize(BSize(kLblW, B_SIZE_UNSET));
    fDoseCtl = new BTextControl("dose", "", "", nullptr);

    fCalcBtn = new BButton("calc_btn", "Calculate Extraction",
                           new BMessage(MSG_EXT_CALC));
    fCalcBtn->MakeDefault(true);

    fBarView = new ExtractionBarView(BRect(0, 0, 460, 70));
    fBarView->SetExplicitMinSize(BSize(460, 70));
    fBarView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 70));

    BRect tvFrame(0, 0, 440, 150);
    fTipsView = new BTextView(tvFrame, "tips", tvFrame.OffsetToCopy(B_ORIGIN),
                              B_FOLLOW_ALL, B_WILL_DRAW);
    fTipsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fTipsView->MakeEditable(false);
    fTipsView->MakeSelectable(false);
    fTipsView->SetWordWrap(true);
    fTipsView->SetText("Enter your values above and press Calculate.");
    {
        CoffeeSettings* s = CoffeeSettings::Get();
        fTipsView->SetViewColor(s->ThemePanelBg());
        rgb_color tc = s->ThemeTextColor();
        fTipsView->SetFontAndColor(0, fTipsView->TextLength(),
                                   be_plain_font, B_FONT_ALL, &tc);
    }

    fTipsScroll = new BScrollView("tips_scroll", fTipsView,
                                  B_FOLLOW_ALL, 0, false, true);
    fTipsScroll->SetExplicitMinSize(BSize(460, 130));
    fTipsScroll->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 180));

    // Wrap each radio pair in its own BGroupView so the two pairs
    // don't form one mutual-exclusion group.
    fMeasureGroup = new BGroupView(B_HORIZONTAL, 10);
    fMeasureGroup->AddChild(fTdsRadio);
    fMeasureGroup->AddChild(fBrixRadio);

    fBrewGroup = new BGroupView(B_HORIZONTAL, 10);
    fBrewGroup->AddChild(fPercRadio);
    fBrewGroup->AddChild(fImmsRadio);

    BMenuBar* menuBar = new BMenuBar("menubar");
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
                                   new BMessage(B_ABOUT_REQUESTED)));
    menuBar->AddItem(helpMenu);
    CoffeeSettings::BuildSettingsMenu(menuBar);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .AddGroup(B_VERTICAL, kPad)
            .SetInsets(kPad*2, kPad*2, kPad*2, kPad*2)
            .Add(new BStringView("lbl_mode", "Measurement input:"))
            .Add(fMeasureGroup)
            .AddGroup(B_HORIZONTAL, 4)
                .Add(fMeasureLbl)
                .Add(fMeasureCtl)
                .End()
            .Add(new BStringView("sep1", ""))
            .Add(new BStringView("lbl_brew", "Brew type:"))
            .Add(fBrewGroup)
            .Add(fStatusView)
            .Add(new BStringView("sep2", ""))
            .AddGroup(B_HORIZONTAL, 4)
                .Add(fLiquidLbl)
                .Add(fLiquidCtl)
                .End()
            .AddGroup(B_HORIZONTAL, 4)
                .Add(fDoseLbl)
                .Add(fDoseCtl)
                .End()
            .Add(new BStringView("sep3", ""))
            .Add(fCalcBtn)
            .Add(new BStringView("sep4", ""))
            .Add(new BStringView("bar_lbl", "Extraction gauge:"))
            .Add(fBarView)
            .Add(new BStringView("sep5", ""))
            .Add(new BStringView("tips_lbl", "Tips & guidance:"))
            .Add(fTipsScroll)
            .End();

    ResizeTo(GetLayout()->PreferredSize().width  + 40,
             GetLayout()->PreferredSize().height + 40);
    CenterOnScreen();
}

void ExtractionWindow::UpdateInputLabels()
{
    fMeasureLbl->SetText(fUseBrix ? "Brix:" : "TDS (%):");
    fLiquidLbl->SetText(fIsPercolation
        ? "Brew weight (g):"
        : "Water used (g):");
    UpdateToggleStatus();
}

void ExtractionWindow::UpdateToggleStatus()
{
    const char* measMode   = fUseBrix       ? "Brix"        : "TDS";
    const char* brewMode   = fIsPercolation ? "Percolation" : "Immersion";
    const char* idealRange = fIsPercolation ? "18-22%"      : "18-24%";
    char buf[128];
    snprintf(buf, sizeof(buf), "Mode: %s  |  %s  (ideal %s)",
             measMode, brewMode, idealRange);
    fStatusView->SetText(buf);
}

void ExtractionWindow::Calculate()
{
    float measure = atof(fMeasureCtl->Text());
    float liquid  = atof(fLiquidCtl->Text());
    float dose    = atof(fDoseCtl->Text());

    if (measure <= 0.0f || liquid <= 0.0f || dose <= 0.0f) {
        fTipsView->SetText("Please fill in all fields with positive values.");
        {
            rgb_color tc = CoffeeSettings::Get()->ThemeTextColor();
            fTipsView->SetFontAndColor(0, fTipsView->TextLength(),
                                       be_plain_font, B_FONT_ALL, &tc);
        }
        fBarView->SetExtraction(-1.0f, 18.0f, 22.0f);
        return;
    }

    float tds  = fUseBrix ? measure * 0.85f : measure;
    float ey   = (tds * liquid) / dose;
    float idealLo = 18.0f;
    float idealHi = fIsPercolation ? 22.0f : 24.0f;

    fBarView->SetExtraction(ey, idealLo, idealHi);

    char buf[2048];
    const char* brewType = fIsPercolation ? "percolation" : "immersion";

    if (ey < idealLo) {
        float shortfall = idealLo - ey;
        snprintf(buf, sizeof(buf),
            "Result: %.2f%% extraction  (under-extracted for %s)\n\n"
            "Your coffee is under-extracted by %.1f%%."
            " It will likely taste sour, weak, or lacking sweetness.\n\n"
            "Tips to increase extraction:\n"
            "%s"
            "  * Grind finer to increase surface area.\n"
            "  * Raise your water temperature (target 90-%s deg C).\n"
            "  * Extend brew time.\n"
            "  * Use more water or less coffee.\n"
            "  * Ensure even saturation of grounds.",
            ey, brewType, shortfall,
            fIsPercolation
                ? "  * Pour more slowly for better agitation.\n"
                : "  * Stir gently during immersion.\n",
            fIsPercolation ? "96" : "94");
    } else if (ey > idealHi) {
        float excess = ey - idealHi;
        snprintf(buf, sizeof(buf),
            "Result: %.2f%% extraction  (over-extracted for %s)\n\n"
            "Your coffee is over-extracted by %.1f%%."
            " It will likely taste bitter, astringent, or hollow.\n\n"
            "Tips to decrease extraction:\n"
            "%s"
            "  * Grind coarser to reduce surface area.\n"
            "  * Lower your water temperature slightly.\n"
            "  * Shorten brew time.\n"
            "  * Use less water or more coffee.",
            ey, brewType, excess,
            fIsPercolation
                ? "  * Speed up your pour rate.\n"
                : "  * Reduce steep time.\n");
    } else {
        snprintf(buf, sizeof(buf),
            "Result: %.2f%% extraction  (in the ideal zone for %s)\n\n"
            "Your extraction is dialled in!"
            " Ideal %s range is %.0f-%.0f%%.\n\n"
            "Fine-tuning ideas:\n"
            "  * If the cup tastes bright/acidic, nudge extraction up\n"
            "    by grinding slightly finer or brewing a little longer.\n"
            "  * If the cup tastes slightly bitter, nudge extraction\n"
            "    down by grinding a touch coarser.\n"
            "  * Keep notes of grind setting, dose, and water\n"
            "    temperature so you can reproduce this result.",
            ey, brewType, brewType, idealLo, idealHi);
    }

    fTipsView->SetText(buf);
    {
        rgb_color tc = CoffeeSettings::Get()->ThemeTextColor();
        fTipsView->SetFontAndColor(0, fTipsView->TextLength(),
                                   be_plain_font, B_FONT_ALL, &tc);
    }
}

void ExtractionWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case MSG_USE_TDS:
            fUseBrix = false;
            UpdateInputLabels();
            break;
        case MSG_USE_BRIX:
            fUseBrix = true;
            UpdateInputLabels();
            break;
        case MSG_BREW_PERC:
            fIsPercolation = true;
            UpdateInputLabels();
            break;
        case MSG_BREW_IMMS:
            fIsPercolation = false;
            UpdateInputLabels();
            break;
        case MSG_EXT_CALC:
            Calculate();
            break;
        case MSG_THEME_CHANGED: {
            CoffeeSettings* s = CoffeeSettings::Get();
            rgb_color bg = s->ThemePanelBg();
            rgb_color tc = s->ThemeTextColor();
            fTipsView->SetViewColor(bg);
            fTipsView->SetFontAndColor(0, fTipsView->TextLength(),
                                       be_plain_font, B_FONT_ALL, &tc);
            fTipsView->Invalidate();
            fBarView->Invalidate();
            break;
        }
        default:
            if (CoffeeSettings::Get()->HandleSettingsMessage(msg))
                break;
            BWindow::MessageReceived(msg);
    }
}
