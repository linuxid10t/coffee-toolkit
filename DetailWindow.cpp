/*  DetailWindow.cpp – Coffee Toolkit
 *
 *  Particle Size Analyzer — three modes:
 *    1. Photo Estimate  (local contrast map)
 *    2. Sieve Cascade   (threshold + accumulate per sieve fraction)
 *    3. Calibrated Sheet (flood-fill blobs + px/mm calibration)
 */

#include "DetailWindow.h"
#include "RoastColorWindow.h"   // reuse ThumbView + MSG_SELECTION_CHANGED
#include "Constants.h"
#include "Settings.h"

#include <Application.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>
#include <stack>

// ===================================================================
// ParticleGaugeView
// ===================================================================

// Score→colour: Extra-Fine = pale tan, Extra-Coarse = dark brown
static const float kScoreMin = 0.0f;
static const float kScoreMax = 1.0f;

struct GrindBand { float lo; float hi; const char* name; };
static const GrindBand kBands[] = {
    { 0.00f, 0.15f, "X-Fine"  },
    { 0.15f, 0.28f, "Fine"    },
    { 0.28f, 0.42f, "Med-Fine"},
    { 0.42f, 0.58f, "Medium"  },
    { 0.58f, 0.72f, "Med-Crs" },
    { 0.72f, 0.86f, "Coarse"  },
    { 0.86f, 1.00f, "X-Coarse"},
};
static const int kNBands = 7;

ParticleGaugeView::ParticleGaugeView(BRect frame)
    : BView(frame, "particle_gauge", B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
            B_WILL_DRAW),
      fScore(-1.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void ParticleGaugeView::SetScore(float score)
{
    fScore = score;
    Invalidate();
}

void ParticleGaugeView::Draw(BRect /*updateRect*/)
{
    CoffeeSettings* s = CoffeeSettings::Get();
    rgb_color bg      = s->ThemePanelBg();
    rgb_color dim     = s->ThemeDimTextColor();
    rgb_color outline = s->ThemeOutlineColor();
    rgb_color text    = s->ThemeTextColor();

    BRect b = Bounds();
    float barTop    = 18.0f;
    float barBottom = 44.0f;
    float barH      = barBottom - barTop;
    float barLeft   = b.left  + 2.0f;
    float barRight  = b.right - 2.0f;
    float barWidth  = barRight - barLeft;

    // Clear background
    SetHighColor(bg);
    FillRect(b);

    auto scoreToX = [&](float s) -> float {
        return barLeft + s * barWidth;
    };

    // Gradient: pale tan (fine) → dark brown (coarse)
    struct Stop { float pos; uint8 r, g, b; };
    Stop stops[] = {
        { 0.00f, 230, 210, 175 },  // very pale / fine powder
        { 0.20f, 190, 160, 105 },
        { 0.40f, 150, 110,  60 },
        { 0.60f, 110,  70,  28 },
        { 0.80f,  65,  35,  12 },
        { 1.00f,  25,  12,   4 },  // very dark / coarse chunks
    };
    const int nStops = 6;

    for (int i = 0; i < nStops - 1; i++) {
        float x0 = barLeft + stops[i].pos   * barWidth;
        float x1 = barLeft + stops[i+1].pos * barWidth;
        int steps = (int)(x1 - x0);
        if (steps < 1) steps = 1;
        for (int s = 0; s < steps; s++) {
            float t  = (float)s / steps;
            uint8 r  = (uint8)(stops[i].r + t*(stops[i+1].r - stops[i].r));
            uint8 g  = (uint8)(stops[i].g + t*(stops[i+1].g - stops[i].g));
            uint8 bv = (uint8)(stops[i].b + t*(stops[i+1].b - stops[i].b));
            SetHighColor(r, g, bv);
            float sx = x0 + s;
            StrokeLine(BPoint(sx, barTop), BPoint(sx, barBottom));
        }
    }

    // Outline
    SetHighColor(outline);
    StrokeRect(BRect(barLeft, barTop, barRight, barBottom));

    // Band dividers and labels
    BFont tiny; tiny.SetSize(8.0f); SetFont(&tiny);
    for (int i = 0; i < kNBands; i++) {
        float xL = scoreToX(kBands[i].lo);
        float xR = scoreToX(kBands[i].hi);
        if (i > 0) {
            SetHighColor(outline);
            StrokeLine(BPoint(xL, barTop), BPoint(xL, barBottom));
        }
        // label colour: light on dark right side, dark on light left side
        if (kBands[i].lo >= 0.5f)
            SetHighColor(220, 200, 160);
        else
            SetHighColor(40, 20, 8);
        DrawString(kBands[i].name,
                   BPoint((xL+xR)/2.0f - 14, barTop + barH*0.65f));
    }

    // Scale ticks
    BFont small; small.SetSize(9.0f); SetFont(&small);
    SetHighColor(dim);
    for (int i = 0; i <= kNBands; i++) {
        float sc = (float)i / kNBands;
        float x = scoreToX(sc);
        StrokeLine(BPoint(x, barBottom), BPoint(x, barBottom+4));
    }

    // Pointer
    if (fScore >= 0.0f && fScore <= 1.0f) {
        float x = scoreToX(fScore);
        SetHighColor(text);
        FillTriangle(BPoint(x, barTop-1),
                     BPoint(x-5, barTop-9),
                     BPoint(x+5, barTop-9));
        SetHighColor(outline);
        StrokeTriangle(BPoint(x, barTop-1),
                       BPoint(x-5, barTop-9),
                       BPoint(x+5, barTop-9));
        SetHighColor(text);
        SetDrawingMode(B_OP_OVER);
        StrokeLine(BPoint(x, barTop), BPoint(x, barBottom));
        SetDrawingMode(B_OP_COPY);
    }
}

// ===================================================================
// SieveDistView
// ===================================================================

SieveDistView::SieveDistView(BRect frame)
    : BView(frame, "sieve_dist", B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
            B_WILL_DRAW)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void SieveDistView::SetFractions(const std::vector<SieveFraction>& fracs)
{
    fFractions = fracs;
    // Sort by sieve size ascending for display
    std::sort(fFractions.begin(), fFractions.end(),
              [](const SieveFraction& a, const SieveFraction& b){
                  return a.sizeUm < b.sizeUm;
              });
    Invalidate();
}

void SieveDistView::Draw(BRect /*updateRect*/)
{
    CoffeeSettings* s = CoffeeSettings::Get();
    rgb_color bg      = s->ThemePanelBg();
    rgb_color text    = s->ThemeTextColor();
    rgb_color dim     = s->ThemeDimTextColor();

    BRect b = Bounds();
    SetHighColor(bg);
    FillRect(b);

    if (fFractions.empty()) {
        SetHighColor(dim);
        BFont f; f.SetSize(11); SetFont(&f);
        DrawString("No fractions loaded yet.",
                   BPoint(10, b.Height()/2));
        return;
    }

    // Normalise relative areas to percentages
    float total = 0.0f;
    for (const auto& fr : fFractions) total += fr.relArea;
    if (total <= 0.0f) return;

    float margin  = 30.0f;
    float areaW   = b.Width()  - margin * 2.0f;
    float areaH   = b.Height() - 30.0f;
    float barW    = areaW / (float)fFractions.size();

    BFont tiny; tiny.SetSize(8.5f); SetFont(&tiny);

    for (int i = 0; i < (int)fFractions.size(); i++) {
        float pct   = fFractions[i].relArea / total;
        float bh    = pct * areaH;
        float bx    = margin + i * barW + 2.0f;
        float bxe   = bx + barW - 4.0f;
        float by    = b.Height() - 20.0f - bh;
        float bye   = b.Height() - 20.0f;

        // Bar fill — coffee brown gradient tinted by size
        uint8 r = 120, g = 70, bv = 25;
        SetHighColor(r, g, bv);
        FillRect(BRect(bx, by, bxe, bye));
        SetHighColor(60, 35, 10);
        StrokeRect(BRect(bx, by, bxe, bye));

        // Percentage label above bar
        char pctStr[8]; snprintf(pctStr, sizeof(pctStr), "%.0f%%", pct*100.0f);
        SetHighColor(text);
        DrawString(pctStr, BPoint(bx, by - 2));

        // Sieve size label below bar
        char szStr[16];
        if (fFractions[i].sizeUm < 1000)
            snprintf(szStr, sizeof(szStr), "%dµm", fFractions[i].sizeUm);
        else
            snprintf(szStr, sizeof(szStr), "%.1fmm",
                     fFractions[i].sizeUm / 1000.0f);
        SetHighColor(dim);
        DrawString(szStr, BPoint(bx, bye + 12));
    }
}

// ===================================================================
// CalHistView
// ===================================================================

CalHistView::CalHistView(BRect frame)
    : BView(frame, "cal_hist", B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
            B_WILL_DRAW)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void CalHistView::SetDiameters(const std::vector<float>& diameters_mm)
{
    fDiameters = diameters_mm;
    Invalidate();
}

void CalHistView::Draw(BRect /*updateRect*/)
{
    CoffeeSettings* s = CoffeeSettings::Get();
    rgb_color bg      = s->ThemePanelBg();
    rgb_color dim     = s->ThemeDimTextColor();

    BRect b = Bounds();
    SetHighColor(bg);
    FillRect(b);

    if (fDiameters.empty()) {
        SetHighColor(dim);
        BFont f; f.SetSize(11); SetFont(&f);
        DrawString("No analysis run yet.", BPoint(10, b.Height()/2));
        return;
    }

    // Build histogram with 0.1 mm bins, range 0..4 mm
    const float binW  = 0.1f;
    const int   nBins = 40;   // 0.0 to 4.0 mm
    int bins[nBins] = {};
    int maxCount = 0;

    for (float d : fDiameters) {
        int bin = (int)(d / binW);
        if (bin < 0)     bin = 0;
        if (bin >= nBins) bin = nBins - 1;
        bins[bin]++;
        if (bins[bin] > maxCount) maxCount = bins[bin];
    }

    if (maxCount == 0) return;

    float margin  = 30.0f;
    float areaW   = b.Width()  - margin * 2.0f;
    float areaH   = b.Height() - 30.0f;
    float bw      = areaW / nBins;

    BFont tiny; tiny.SetSize(8.0f); SetFont(&tiny);

    for (int i = 0; i < nBins; i++) {
        if (bins[i] == 0) continue;
        float bh  = ((float)bins[i] / maxCount) * areaH;
        float bx  = margin + i * bw;
        float bxe = bx + bw - 1.0f;
        float by  = b.Height() - 20.0f - bh;
        float bye = b.Height() - 20.0f;

        SetHighColor(110, 68, 25);
        FillRect(BRect(bx, by, bxe, bye));
        SetHighColor(60, 35, 10);
        StrokeRect(BRect(bx, by, bxe, bye));
    }

    // X-axis tick labels every 0.5 mm (or equivalent in inches)
    bool metric = CoffeeSettings::Get()->UseMetric();
    SetHighColor(dim);
    for (int i = 0; i <= nBins; i += 5) {
        float x = margin + i * bw;
        StrokeLine(BPoint(x, b.Height()-20.0f),
                   BPoint(x, b.Height()-16.0f));
        float val = metric ? (i * binW) : (i * binW / 25.4f);
        char lbl[10];
        snprintf(lbl, sizeof(lbl), metric ? "%.1f" : "%.3f", val);
        DrawString(lbl, BPoint(x-8, b.Height()-6));
    }

    // Unit label
    SetHighColor(dim);
    DrawString(metric ? "mm" : "in", BPoint(b.Width()-22, b.Height()-6));
}

// ===================================================================
// Shared helpers
// ===================================================================

// sRGB linearisation (same as RoastColorWindow)
static inline float lin(float c)
{
    return c <= 0.04045f ? c / 12.92f
                         : powf((c + 0.055f) / 1.055f, 2.4f);
}

static float pixelLuminance(const uint8* bits, int bpr,
                             color_space cs, int x, int y)
{
    float rf = 0, gf = 0, bf = 0;
    if (cs == B_RGB32 || cs == B_RGBA32) {
        const uint8* p = bits + y*bpr + x*4;
        bf = p[0]/255.0f; gf = p[1]/255.0f; rf = p[2]/255.0f;
    } else if (cs == B_RGB24) {
        const uint8* p = bits + y*bpr + x*3;
        bf = p[0]/255.0f; gf = p[1]/255.0f; rf = p[2]/255.0f;
    }
    return 0.2126f*lin(rf) + 0.7152f*lin(gf) + 0.0722f*lin(bf);
}

// Scale a bitmap to tw×th using nearest-neighbour
BBitmap* DetailWindow::ScaleBitmap(BBitmap* src, int tw, int th)
{
    BRect sr = src->Bounds();
    float sw = sr.Width() + 1.0f;
    float sh = sr.Height() + 1.0f;
    const uint8* sbits = (const uint8*)src->Bits();
    int   sbpr  = src->BytesPerRow();
    color_space cs = src->ColorSpace();

    BBitmap* dst = new BBitmap(BRect(0, 0, tw-1, th-1), B_RGB32);
    uint8* dbits = (uint8*)dst->Bits();
    int    dbpr  = dst->BytesPerRow();

    for (int dy = 0; dy < th; dy++) {
        int sy = (int)(dy * sh / th);
        for (int dx = 0; dx < tw; dx++) {
            int sx = (int)(dx * sw / tw);
            uint8 r = 128, g = 128, bv = 128;
            if (cs == B_RGB32 || cs == B_RGBA32) {
                const uint8* p = sbits + sy*sbpr + sx*4;
                bv = p[0]; g = p[1]; r = p[2];
            } else if (cs == B_RGB24) {
                const uint8* p = sbits + sy*sbpr + sx*3;
                bv = p[0]; g = p[1]; r = p[2];
            }
            uint8* d = dbits + dy*dbpr + dx*4;
            d[0] = bv; d[1] = g; d[2] = r; d[3] = 255;
        }
    }
    return dst;
}

// ===================================================================
// DetailWindow — constructor
// ===================================================================

DetailWindow::DetailWindow()
    : BWindow(BRect(160, 100, 700, 840), "Particle Analyzer",
              B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fMode(kModePhoto),
      fPhotoFilePanel(nullptr),
      fPhotoSource(nullptr),
      fPhotoThumbBmp(nullptr),
      fSieveFilePanel(nullptr),
      fSieveSource(nullptr),
      fSieveThumbBmp(nullptr),
      fCalFilePanel(nullptr),
      fCalSource(nullptr),
      fCalThumbBmp(nullptr)
{
    BMessenger me(this);

    // ---- Menu bar ----
    BMenuBar* menuBar = new BMenuBar("menubar");
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
                                   new BMessage(B_ABOUT_REQUESTED)));
    menuBar->AddItem(helpMenu);
    CoffeeSettings::BuildSettingsMenu(menuBar);

    // ---- Mode selector (radio buttons in one BGroupView) ----
    fPhotoRadio = new BRadioButton("mode_photo", "Photo Estimate",
                                   new BMessage(MSG_PART_MODE_PHOTO));
    fSieveRadio = new BRadioButton("mode_sieve", "Sieve Cascade",
                                   new BMessage(MSG_PART_MODE_SIEVE));
    fCalRadio   = new BRadioButton("mode_cal",   "Calibrated Sheet",
                                   new BMessage(MSG_PART_MODE_CAL));
    fPhotoRadio->SetValue(B_CONTROL_ON);

    fModeBar = new BGroupView(B_HORIZONTAL, kPad*2);
    fModeBar->AddChild(fPhotoRadio);
    fModeBar->AddChild(fSieveRadio);
    fModeBar->AddChild(fCalRadio);

    // ===== PANEL 1: Photo Estimate =====
    fPhotoOpenBtn  = new BButton("ph_open", "Load Image...",
                                 new BMessage(MSG_PART_OPEN));
    fPhotoFileView = new BStringView("ph_file", "No image loaded.");
    fPhotoFileView->SetExplicitAlignment(
        BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

    ThumbView* phThumb = new ThumbView(BRect(0, 0, 239, 179), me);
    phThumb->SetExplicitMinSize(BSize(240, 180));
    phThumb->SetExplicitMaxSize(BSize(240, 180));
    fPhotoThumb = phThumb;

    fPhotoHintView = new BStringView("ph_hint",
        "Drag on the image to select a sample region.");
    BFont hintFont(be_plain_font);
    hintFont.SetFace(B_ITALIC_FACE);
    fPhotoHintView->SetFont(&hintFont);

    fPhotoClearBtn = new BButton("ph_clear", "Clear Selection",
                                 new BMessage(MSG_CLEAR_SELECTION));
    fPhotoClearBtn->SetEnabled(false);

    fPhotoGauge = new ParticleGaugeView(BRect(0, 0, 460, 55));
    fPhotoGauge->SetExplicitMinSize(BSize(460, 55));
    fPhotoGauge->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 55));

    fPhotoResultView = new BStringView("ph_result", "");
    fPhotoResultView->SetFont(be_bold_font);
    fPhotoResultView->SetExplicitAlignment(
        BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    BRect tvFrame(0, 0, 440, 130);
    fPhotoTipsView = new BTextView(tvFrame, "ph_tips",
                                   tvFrame.OffsetToCopy(B_ORIGIN),
                                   B_FOLLOW_ALL, B_WILL_DRAW);
    fPhotoTipsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fPhotoTipsView->MakeEditable(false);
    fPhotoTipsView->MakeSelectable(false);
    fPhotoTipsView->SetWordWrap(true);
    fPhotoTipsView->SetText(
        "Load a photograph of your coffee grounds on a uniform background.\n\n"
        "For best results:\n"
        "  * Spread grounds in a thin, even layer\n"
        "  * Use a neutral grey or white background\n"
        "  * Diffuse, even lighting — avoid direct sunlight\n"
        "  * Fill the frame with grounds\n\n"
        "Drag a rectangle on the preview to sample a specific region.");
    {
        CoffeeSettings* s = CoffeeSettings::Get();
        fPhotoTipsView->SetViewColor(s->ThemePanelBg());
        rgb_color tc = s->ThemeTextColor();
        fPhotoTipsView->SetFontAndColor(0, fPhotoTipsView->TextLength(),
                                        be_plain_font, B_FONT_ALL, &tc);
    }
    fPhotoTipsScroll = new BScrollView("ph_tips_scroll", fPhotoTipsView,
                                       B_FOLLOW_ALL, 0, false, true);
    fPhotoTipsScroll->SetExplicitMinSize(BSize(460, 75));
    fPhotoTipsScroll->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 130));

    fPhotoPanel = new BGroupView(B_VERTICAL, kPad);
    BLayoutBuilder::Group<>(fPhotoPanel, B_VERTICAL, kPad)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fPhotoOpenBtn)
            .Add(fPhotoFileView)
            .End()
        .Add(fPhotoThumb)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fPhotoHintView)
            .AddGlue()
            .Add(fPhotoClearBtn)
            .End()
        .Add(new BStringView("ph_glbl", "Grind size estimate:"))
        .Add(fPhotoGauge)
        .Add(fPhotoResultView)
        .Add(new BStringView("ph_tlbl", "Grind notes & guidance:"))
        .Add(fPhotoTipsScroll);

    // ===== PANEL 2: Sieve Cascade =====
    fSieveSizeMenu = new BPopUpMenu("600 µm");
    const int sieveSizes[] = { 212, 300, 425, 600, 850, 1200, 1700 };
    for (int sz : sieveSizes) {
        char lbl[32];
        if (sz < 1000)
            snprintf(lbl, sizeof(lbl), "%d µm", sz);
        else
            snprintf(lbl, sizeof(lbl), "%.1f mm", sz / 1000.0f);
        BMessage* pickMsg = new BMessage('SVSZ');
        pickMsg->AddInt32("size_um", sz);
        fSieveSizeMenu->AddItem(new BMenuItem(lbl, pickMsg));
    }
    // Default to 600 µm
    fSieveSizeMenu->ItemAt(3)->SetMarked(true);
    fSieveSizeField = new BMenuField("sv_szfield", "Sieve size:", fSieveSizeMenu);

    fSieveOpenBtn  = new BButton("sv_open", "Load Fraction Image...",
                                 new BMessage(MSG_SIEVE_OPEN));
    fSieveFileView = new BStringView("sv_file", "No image loaded.");
    fSieveFileView->SetExplicitAlignment(
        BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

    ThumbView* svThumb = new ThumbView(BRect(0, 0, 239, 179), me);
    svThumb->SetExplicitMinSize(BSize(240, 180));
    svThumb->SetExplicitMaxSize(BSize(240, 180));
    fSieveThumb = svThumb;

    fSieveHintView = new BStringView("sv_hint",
        "Drag on the image to select the sieve fraction region.");
    fSieveHintView->SetFont(&hintFont);

    fSieveClearBtn = new BButton("sv_clear", "Clear Selection",
                                 new BMessage(MSG_CLEAR_SELECTION));
    fSieveClearBtn->SetEnabled(false);

    fSieveAddBtn   = new BButton("sv_add",   "Add Fraction",
                                 new BMessage(MSG_SIEVE_ADD));
    fSieveAddBtn->SetEnabled(false);
    fSieveResetBtn = new BButton("sv_reset", "Reset Distribution",
                                 new BMessage(MSG_SIEVE_RESET));

    fSieveDist = new SieveDistView(BRect(0, 0, 460, 100));
    fSieveDist->SetExplicitMinSize(BSize(460, 100));
    fSieveDist->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 150));

    fSieveSummary = new BStringView("sv_summary", "Load fraction images and click \"Add Fraction\".");
    fSieveSummary->SetExplicitAlignment(
        BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    fSievePanel = new BGroupView(B_VERTICAL, kPad);
    BLayoutBuilder::Group<>(fSievePanel, B_VERTICAL, kPad)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fSieveSizeField)
            .Add(fSieveOpenBtn)
            .Add(fSieveFileView)
            .End()
        .Add(fSieveThumb)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fSieveHintView)
            .AddGlue()
            .Add(fSieveClearBtn)
            .End()
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fSieveAddBtn)
            .AddGlue()
            .Add(fSieveResetBtn)
            .End()
        .Add(new BStringView("sv_dlbl", "Particle size distribution:"))
        .Add(fSieveDist)
        .Add(fSieveSummary);

    // ===== PANEL 3: Calibrated Sheet =====
    fCalOpenBtn  = new BButton("cal_open", "Load Image...",
                               new BMessage(MSG_CAL_OPEN));
    fCalFileView = new BStringView("cal_file", "No image loaded.");
    fCalFileView->SetExplicitAlignment(
        BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

    ThumbView* calThumb = new ThumbView(BRect(0, 0, 239, 179), me);
    calThumb->SetExplicitMinSize(BSize(240, 180));
    calThumb->SetExplicitMaxSize(BSize(240, 180));
    fCalThumb = calThumb;

    fCalHintView = new BStringView("cal_hint",
        "Drag on the image to select the coffee region.");
    fCalHintView->SetFont(&hintFont);

    fCalClearBtn = new BButton("cal_clear", "Clear Selection",
                               new BMessage(MSG_CLEAR_SELECTION));
    fCalClearBtn->SetEnabled(false);

    fCalSubCircle = new BRadioButton("cal_circ", "Circle Reference",
                                     new BMessage(MSG_CAL_SUB_CIRCLE));
    fCalSubLines  = new BRadioButton("cal_lns",  "Notebook Lines",
                                     new BMessage(MSG_CAL_SUB_LINES));
    fCalSubCircle->SetValue(B_CONTROL_ON);

    fCalSubRadioGrp = new BGroupView(B_HORIZONTAL, kPad);
    fCalSubRadioGrp->AddChild(fCalSubCircle);
    fCalSubRadioGrp->AddChild(fCalSubLines);

    fCalRefSizeCtl = new BTextControl("cal_ref", "Circle diameter (mm):",
                                      "50.0", nullptr);
    fCalRefSizeCtl->SetModificationMessage(nullptr);

    fCalCalibrateBtn = new BButton("cal_cal", "Calibrate from Selection",
                                   new BMessage(MSG_CAL_CALIBRATE));
    fCalCalibrateBtn->SetEnabled(false);

    fCalPxMmLabel = new BStringView("cal_pxmm_lbl", "Calibration: not set");
    fCalPxMmLabel->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

    fDerivedPxPerMm = 0.0f;

    fCalAnalyseBtn = new BButton("cal_analyse", "Analyse",
                                 new BMessage(MSG_CAL_ANALYSE));
    fCalAnalyseBtn->SetEnabled(false);

    fCalHist = new CalHistView(BRect(0, 0, 460, 100));
    fCalHist->SetExplicitMinSize(BSize(460, 100));
    fCalHist->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 150));

    fCalResultView = new BStringView("cal_result", "");
    fCalResultView->SetFont(be_bold_font);
    fCalResultView->SetExplicitAlignment(
        BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    fCalPanel = new BGroupView(B_VERTICAL, kPad);
    BLayoutBuilder::Group<>(fCalPanel, B_VERTICAL, kPad)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fCalOpenBtn)
            .Add(fCalFileView)
            .End()
        .Add(fCalThumb)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fCalHintView)
            .AddGlue()
            .Add(fCalClearBtn)
            .End()
        .Add(fCalSubRadioGrp)
        .AddGroup(B_HORIZONTAL, kPad)
            .Add(fCalRefSizeCtl)
            .AddGlue()
            .Add(fCalCalibrateBtn)
            .End()
        .Add(fCalPxMmLabel)
        .AddGroup(B_HORIZONTAL, kPad)
            .AddGlue()
            .Add(fCalAnalyseBtn)
            .End()
        .Add(new BStringView("cal_hlbl", "Particle diameter histogram (mm):"))
        .Add(fCalHist)
        .Add(fCalResultView);

    // ===== Main layout =====
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .AddGroup(B_VERTICAL, kPad)
            .SetInsets(kPad*2, kPad, kPad*2, kPad)
            .Add(fModeBar)
            .Add(fPhotoPanel)
            .Add(fSievePanel)
            .Add(fCalPanel)
            .End();

    // Hide non-default panels
    fSievePanel->Hide();
    fCalPanel->Hide();

    ResizeTo(GetLayout()->PreferredSize().width  + 40,
             GetLayout()->PreferredSize().height + 40);
    CenterOnScreen();
}

DetailWindow::~DetailWindow()
{
    delete fPhotoSource;
    delete fPhotoThumbBmp;
    delete fPhotoFilePanel;
    delete fSieveSource;
    delete fSieveThumbBmp;
    delete fSieveFilePanel;
    delete fCalSource;
    delete fCalThumbBmp;
    delete fCalFilePanel;
}

// ===================================================================
// SwitchMode
// ===================================================================

void DetailWindow::SwitchMode(int mode)
{
    fMode = mode;
    BGroupView* panels[] = { fPhotoPanel, fSievePanel, fCalPanel };
    for (int i = 0; i < 3; i++) {
        bool shouldShow = (i == mode);
        if (shouldShow && panels[i]->IsHidden())
            panels[i]->Show();
        else if (!shouldShow && !panels[i]->IsHidden())
            panels[i]->Hide();
    }
}

// ===================================================================
// Mode 1: Photo Estimate
// ===================================================================

void DetailWindow::LoadPhotoImage(const entry_ref& ref)
{
    BBitmap* bmp = BTranslationUtils::GetBitmapFile(BPath(&ref).Path());
    if (!bmp || !bmp->IsValid()) {
        delete bmp;
        fPhotoFileView->SetText("Error: could not load image.");
        return;
    }

    delete fPhotoSource;
    fPhotoSource = bmp;

    BEntry entry(&ref);
    char name[B_FILE_NAME_LENGTH];
    entry.GetName(name);
    fPhotoFileView->SetText(name);

    BRect src = fPhotoSource->Bounds();
    float sw = src.Width() + 1, sh = src.Height() + 1;
    float scale = (sw/sh > 320.0f/240.0f) ? 320.0f/sw : 240.0f/sh;
    int tw = (int)(sw * scale), th = (int)(sh * scale);

    delete fPhotoThumbBmp;
    fPhotoThumbBmp = ScaleBitmap(fPhotoSource, tw, th);

    ((ThumbView*)fPhotoThumb)->SetBitmap(fPhotoThumbBmp);
    fPhotoClearBtn->SetEnabled(false);
    AnalysePhoto();
}

float DetailWindow::ComputeGrindScore(BBitmap* bmp, BRect normSel)
{
    BRect bounds = bmp->Bounds();
    int   w   = (int)(bounds.Width()  + 1);
    int   h   = (int)(bounds.Height() + 1);
    int   bpr = bmp->BytesPerRow();
    color_space cs = bmp->ColorSpace();
    const uint8* bits = (const uint8*)bmp->Bits();

    int x0, y0, x1, y1;
    if (normSel.IsValid() && normSel.Width() > 0 && normSel.Height() > 0) {
        x0 = (int)(normSel.left   * w);
        y0 = (int)(normSel.top    * h);
        x1 = (int)(normSel.right  * w);
        y1 = (int)(normSel.bottom * h);
    } else {
        x0 = w/4;   y0 = h/4;
        x1 = w*3/4; y1 = h*3/4;
    }
    if (x0 < 0) x0 = 0;
    if (x1 > w) x1 = w;
    if (y0 < 0) y0 = 0;
    if (y1 > h) y1 = h;
    if (x0 >= x1 || y0 >= y1) return 0.5f;

    // Local contrast map: 8×8 cells
    const int cellSz = 8;
    double sumVar = 0.0;
    int    nCells = 0;

    for (int cy = y0; cy < y1; cy += cellSz) {
        int cye = cy + cellSz; if (cye > y1) cye = y1;
        for (int cx = x0; cx < x1; cx += cellSz) {
            int cxe = cx + cellSz; if (cxe > x1) cxe = x1;
            double sum = 0.0;
            int    cnt = 0;
            for (int py = cy; py < cye; py++)
                for (int px = cx; px < cxe; px++) {
                    sum += pixelLuminance(bits, bpr, cs, px, py);
                    cnt++;
                }
            if (cnt == 0) continue;
            double mean = sum / cnt;
            double var  = 0.0;
            for (int py = cy; py < cye; py++)
                for (int px = cx; px < cxe; px++) {
                    double d = pixelLuminance(bits, bpr, cs, px, py) - mean;
                    var += d * d;
                }
            sumVar += var / cnt;
            nCells++;
        }
    }

    if (nCells == 0) return 0.5f;
    float avgVar = (float)(sumVar / nCells);
    // Calibrate: typical max local variance for a coarse grind ≈ 0.05
    float score  = avgVar / 0.05f;
    if (score > 1.0f) score = 1.0f;
    return score;
}

const char* DetailWindow::GrindName(float score)
{
    for (int i = 0; i < kNBands; i++)
        if (score < kBands[i].hi)
            return kBands[i].name;
    return kBands[kNBands-1].name;
}

void DetailWindow::BuildPhotoTips(float score, char* buf, size_t sz)
{
    if (score < 0.15f) {
        snprintf(buf, sz,
            "Grind: Extra Fine (<250 µm)\n\n"
            "Suitable for: Turkish coffee only.\n\n"
            "Notes: Very fine powder almost like flour. Over-extraction is "
            "likely in most brew methods.\n\n"
            "Brew tips:\n"
            "  * Use immediately — moisture absorption degrades quickly.\n"
            "  * Turkish: bring to boil 3 times, do not filter.\n"
            "  * Not suitable for espresso — will clog baskets.");
    } else if (score < 0.28f) {
        snprintf(buf, sz,
            "Grind: Fine (250–400 µm)\n\n"
            "Suitable for: Espresso, stovetop moka pot.\n\n"
            "Notes: Dense puck, high extraction rate. Ideal for short, "
            "concentrated shots.\n\n"
            "Brew tips:\n"
            "  * Espresso: target 9 bar, 25–30 s shot time.\n"
            "  * Adjust dose ±0.5 g to dial in shot.\n"
            "  * Moka pot: medium heat, remove before hissing.");
    } else if (score < 0.42f) {
        snprintf(buf, sz,
            "Grind: Medium-Fine (400–600 µm)\n\n"
            "Suitable for: AeroPress, moka pot, siphon.\n\n"
            "Notes: Good balance of extraction speed and clarity.\n\n"
            "Brew tips:\n"
            "  * AeroPress: 2–3 min brew, inverted or standard method.\n"
            "  * Water temperature 90–95 deg C.\n"
            "  * Siphon: stir once at start, once mid-brew.");
    } else if (score < 0.58f) {
        snprintf(buf, sz,
            "Grind: Medium (600–800 µm)\n\n"
            "Suitable for: Pour-over (V60, Kalita), drip machine.\n\n"
            "Notes: Most versatile grind — the starting point for "
            "filter brewing.\n\n"
            "Brew tips:\n"
            "  * Pour-over: 3 min total brew, 30 s bloom.\n"
            "  * Water temperature 92–94 deg C.\n"
            "  * Aim for 60 g/litre ratio as a baseline.");
    } else if (score < 0.72f) {
        snprintf(buf, sz,
            "Grind: Medium-Coarse (800–1000 µm)\n\n"
            "Suitable for: Chemex, Café Solo, Clever Dripper.\n\n"
            "Notes: Slower flow rate; cleaner cup with the right filter.\n\n"
            "Brew tips:\n"
            "  * Chemex: pre-wet filter, 4 min total brew.\n"
            "  * Water temperature 93–96 deg C.\n"
            "  * Use the thick Chemex filter for best clarity.");
    } else if (score < 0.86f) {
        snprintf(buf, sz,
            "Grind: Coarse (1000–1200 µm)\n\n"
            "Suitable for: French press, percolator.\n\n"
            "Notes: Large particles prevent sediment clogging the press "
            "mesh. Under-extraction risk if too coarse.\n\n"
            "Brew tips:\n"
            "  * French press: 4 min steep, plunge slowly.\n"
            "  * Water temperature 95 deg C.\n"
            "  * Let settle 1 min before pouring.");
    } else {
        snprintf(buf, sz,
            "Grind: Extra Coarse (>1200 µm)\n\n"
            "Suitable for: Cold brew, cowboy coffee.\n\n"
            "Notes: Very long extraction needed. Minimal extraction in "
            "standard hot brew methods.\n\n"
            "Brew tips:\n"
            "  * Cold brew: steep 16–24 h in cold water.\n"
            "  * Use a high coffee:water ratio (1:5 to 1:7).\n"
            "  * Strain through fine filter to remove sediment.");
    }
}

void DetailWindow::AnalysePhoto()
{
    if (!fPhotoSource || !fPhotoSource->IsValid()) return;

    BRect normSel;
    ThumbView* tv = (ThumbView*)fPhotoThumb;
    if (tv->HasSelection())
        normSel = tv->NormalisedSelection();

    float score = ComputeGrindScore(fPhotoSource, normSel);
    fPhotoGauge->SetScore(score);

    // Find band size string
    const char* sizeStr = "";
    if (score < 0.15f)      sizeStr = "<250 µm";
    else if (score < 0.28f) sizeStr = "250–400 µm";
    else if (score < 0.42f) sizeStr = "400–600 µm";
    else if (score < 0.58f) sizeStr = "600–800 µm";
    else if (score < 0.72f) sizeStr = "800–1000 µm";
    else if (score < 0.86f) sizeStr = "1000–1200 µm";
    else                    sizeStr = ">1200 µm";

    char label[256];
    snprintf(label, sizeof(label),
             "%s  (%s)  [%s]",
             GrindName(score), sizeStr,
             tv->HasSelection() ? "custom selection" : "centre region");
    fPhotoResultView->SetText(label);

    char tips[2048];
    BuildPhotoTips(score, tips, sizeof(tips));
    fPhotoTipsView->SetText(tips);
}

// ===================================================================
// Mode 2: Sieve Cascade
// ===================================================================

void DetailWindow::LoadSieveFraction(const entry_ref& ref)
{
    BBitmap* bmp = BTranslationUtils::GetBitmapFile(BPath(&ref).Path());
    if (!bmp || !bmp->IsValid()) {
        delete bmp;
        fSieveFileView->SetText("Error: could not load image.");
        return;
    }

    delete fSieveSource;
    fSieveSource = bmp;

    BEntry entry(&ref);
    char name[B_FILE_NAME_LENGTH];
    entry.GetName(name);
    fSieveFileView->SetText(name);

    BRect src = fSieveSource->Bounds();
    float sw = src.Width() + 1, sh = src.Height() + 1;
    float scale = (sw/sh > 320.0f/240.0f) ? 320.0f/sw : 240.0f/sh;
    int tw = (int)(sw * scale), th = (int)(sh * scale);

    delete fSieveThumbBmp;
    fSieveThumbBmp = ScaleBitmap(fSieveSource, tw, th);

    ((ThumbView*)fSieveThumb)->SetBitmap(fSieveThumbBmp);
    fSieveClearBtn->SetEnabled(false);
    fSieveAddBtn->SetEnabled(true);
}

float DetailWindow::ComputeFractionArea(BBitmap* bmp, BRect normSel)
{
    BRect bounds = bmp->Bounds();
    int   w   = (int)(bounds.Width()  + 1);
    int   h   = (int)(bounds.Height() + 1);
    int   bpr = bmp->BytesPerRow();
    color_space cs = bmp->ColorSpace();
    const uint8* bits = (const uint8*)bmp->Bits();

    int x0, y0, x1, y1;
    if (normSel.IsValid() && normSel.Width() > 0 && normSel.Height() > 0) {
        x0 = (int)(normSel.left   * w); y0 = (int)(normSel.top    * h);
        x1 = (int)(normSel.right  * w); y1 = (int)(normSel.bottom * h);
    } else {
        x0 = 0; y0 = 0; x1 = w; y1 = h;
    }
    if (x0 < 0) x0 = 0;
    if (x1 > w) x1 = w;
    if (y0 < 0) y0 = 0;
    if (y1 > h) y1 = h;
    if (x0 >= x1 || y0 >= y1) return 0.0f;

    int dark = 0, total = 0;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float L = pixelLuminance(bits, bpr, cs, x, y);
            if (L < 0.7f) dark++;
            total++;
        }
    }
    return (total > 0) ? (float)dark / total : 0.0f;
}

void DetailWindow::UpdateSieveChart()
{
    fSieveDist->SetFractions(fSieveFractions);

    if (fSieveFractions.size() < 2) {
        fSieveSummary->SetText("Add more fractions to compute distribution.");
        return;
    }

    // Estimate D50: sieve size where cumulative % passes 50%
    // Sort by size ascending and find midpoint
    std::vector<SieveFraction> sorted = fSieveFractions;
    std::sort(sorted.begin(), sorted.end(),
              [](const SieveFraction& a, const SieveFraction& b){
                  return a.sizeUm < b.sizeUm;
              });
    float total = 0.0f;
    for (const auto& f : sorted) total += f.relArea;

    float cumul = 0.0f;
    int   d50   = sorted.back().sizeUm;
    for (const auto& f : sorted) {
        cumul += f.relArea;
        if (cumul / total >= 0.5f) {
            d50 = f.sizeUm;
            break;
        }
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "D50 ≈ %d µm  |  %d fraction%s loaded",
             d50,
             (int)fSieveFractions.size(),
             fSieveFractions.size() == 1 ? "" : "s");
    fSieveSummary->SetText(buf);
}

// ===================================================================
// Mode 3: Calibrated Sheet
// ===================================================================

void DetailWindow::LoadCalImage(const entry_ref& ref)
{
    BBitmap* bmp = BTranslationUtils::GetBitmapFile(BPath(&ref).Path());
    if (!bmp || !bmp->IsValid()) {
        delete bmp;
        fCalFileView->SetText("Error: could not load image.");
        return;
    }

    delete fCalSource;
    fCalSource = bmp;

    BEntry entry(&ref);
    char name[B_FILE_NAME_LENGTH];
    entry.GetName(name);
    fCalFileView->SetText(name);

    BRect src = fCalSource->Bounds();
    float sw = src.Width() + 1, sh = src.Height() + 1;
    float scale = (sw/sh > 320.0f/240.0f) ? 320.0f/sw : 240.0f/sh;
    int tw = (int)(sw * scale), th = (int)(sh * scale);

    delete fCalThumbBmp;
    fCalThumbBmp = ScaleBitmap(fCalSource, tw, th);

    ((ThumbView*)fCalThumb)->SetBitmap(fCalThumbBmp);
    fCalClearBtn->SetEnabled(false);
    fCalCalibrateBtn->SetEnabled(true);
    fDerivedPxPerMm = 0.0f;
    fCalPxMmLabel->SetText("Calibration: not set");
    fCalAnalyseBtn->SetEnabled(false);
}

void DetailWindow::AnalyseCal()
{
    if (!fCalSource || !fCalSource->IsValid()) return;

    if (fDerivedPxPerMm <= 0.0f) {
        fCalResultView->SetText("Calibrate first using the sub-method above.");
        return;
    }
    float pxPerMm = fDerivedPxPerMm;

    BRect bounds = fCalSource->Bounds();
    int   w   = (int)(bounds.Width()  + 1);
    int   h   = (int)(bounds.Height() + 1);
    int   bpr = fCalSource->BytesPerRow();
    color_space cs = fCalSource->ColorSpace();
    const uint8* bits = (const uint8*)fCalSource->Bits();

    // Determine analysis region
    ThumbView* tv = (ThumbView*)fCalThumb;
    int x0, y0, x1, y1;
    if (tv->HasSelection()) {
        BRect n = tv->NormalisedSelection();
        // Map normalised thumb coords back to source image coords
        BRect tbnd = fCalThumbBmp->Bounds();
        float tw_f = tbnd.Width() + 1.0f;
        float th_f = tbnd.Height() + 1.0f;
        float sx = (float)w / tw_f;
        float sy = (float)h / th_f;
        x0 = (int)(n.left   * tw_f * sx);
        y0 = (int)(n.top    * th_f * sy);
        x1 = (int)(n.right  * tw_f * sx);
        y1 = (int)(n.bottom * th_f * sy);
    } else {
        x0 = 0; y0 = 0; x1 = w; y1 = h;
    }
    if (x0 < 0) x0 = 0;
    if (x1 > w) x1 = w;
    if (y0 < 0) y0 = 0;
    if (y1 > h) y1 = h;

    // Threshold: luminance < 0.7 → particle
    // Use a flat array of bool for visited flags
    int rw = x1 - x0, rh = y1 - y0;
    if (rw <= 0 || rh <= 0) return;

    std::vector<bool> visited(rw * rh, false);
    std::vector<float> diameters;

    auto idx = [&](int x, int y) { return (y - y0)*rw + (x - x0); };

    for (int gy = y0; gy < y1; gy++) {
        for (int gx = x0; gx < x1; gx++) {
            if (visited[idx(gx, gy)]) continue;
            float L = pixelLuminance(bits, bpr, cs, gx, gy);
            if (L >= 0.7f) { visited[idx(gx, gy)] = true; continue; }

            // BFS flood fill
            std::stack<std::pair<int,int>> stk;
            stk.push({gx, gy});
            visited[idx(gx, gy)] = true;
            int area = 0;

            while (!stk.empty()) {
                auto [cx, cy] = stk.top(); stk.pop();
                area++;
                const int dx4[] = {1,-1,0,0};
                const int dy4[] = {0,0,1,-1};
                for (int d = 0; d < 4; d++) {
                    int nx = cx + dx4[d];
                    int ny = cy + dy4[d];
                    if (nx < x0 || nx >= x1 || ny < y0 || ny >= y1) continue;
                    if (visited[idx(nx, ny)]) continue;
                    visited[idx(nx, ny)] = true;
                    float Ln = pixelLuminance(bits, bpr, cs, nx, ny);
                    if (Ln < 0.7f)
                        stk.push({nx, ny});
                }
            }

            if (area < 2) continue; // discard noise
            float diam_px = 2.0f * sqrtf((float)area / 3.14159f);
            float diam_mm = diam_px / pxPerMm;
            diameters.push_back(diam_mm);
        }
    }

    fCalHist->SetDiameters(diameters);

    if (diameters.empty()) {
        fCalResultView->SetText("No particles found — try adjusting the threshold or calibration.");
        return;
    }

    std::sort(diameters.begin(), diameters.end());
    int n = (int)diameters.size();
    float d50 = diameters[n/2];
    float d90 = diameters[(int)(n * 0.90f)];

    bool metric = CoffeeSettings::Get()->UseMetric();
    const char* unit = metric ? "mm" : "in";
    float displayD50 = metric ? d50 : d50 / 25.4f;
    float displayD90 = metric ? d90 : d90 / 25.4f;
    char buf[192];
    snprintf(buf, sizeof(buf),
             "D50: %.3f %s  |  D90: %.3f %s  |  %d particles",
             displayD50, unit, displayD90, unit, n);
    fCalResultView->SetText(buf);
}

void DetailWindow::UpdateCalSubMethod()
{
    bool isCircle = (fCalSubCircle->Value() == B_CONTROL_ON);
    const char* unit = CoffeeSettings::Get()->UseMetric() ? "mm" : "in";
    char lbl[64];
    snprintf(lbl, sizeof(lbl), "%s (%s):",
        isCircle ? "Circle diameter" : "Line spacing", unit);
    fCalRefSizeCtl->SetLabel(lbl);
    fDerivedPxPerMm = 0.0f;
    fCalPxMmLabel->SetText("Calibration: not set");
    fCalAnalyseBtn->SetEnabled(false);
}

void DetailWindow::RunCalibrate()
{
    if (!fCalSource || !fCalSource->IsValid()) return;

    float refSize = atof(fCalRefSizeCtl->Text());
    if (refSize <= 0.0f) {
        fCalPxMmLabel->SetText("Error: invalid reference size.");
        return;
    }

    // Internal calibration is always px/mm; convert inches input if needed
    bool metric = CoffeeSettings::Get()->UseMetric();
    float refSizeMm = metric ? refSize : refSize * 25.4f;

    ThumbView* tv = (ThumbView*)fCalThumb;
    BRect normSel(0.0f, 0.0f, 1.0f, 1.0f);
    if (tv->HasSelection())
        normSel = tv->NormalisedSelection();

    bool isCircle = (fCalSubCircle->Value() == B_CONTROL_ON);
    float pxmm = isCircle
        ? DetectCirclePxMm(fCalSource, normSel, refSizeMm)
        : DetectLinesPxMm(fCalSource, normSel, refSizeMm);

    if (pxmm <= 0.0f) {
        fCalPxMmLabel->SetText(isCircle
            ? "Detection failed — select a region containing the circle."
            : "Detection failed — select a region with clear horizontal lines.");
        return;
    }

    fDerivedPxPerMm = pxmm;
    char buf[64];
    snprintf(buf, sizeof(buf), "Calibration: %.2f px/mm", fDerivedPxPerMm);
    fCalPxMmLabel->SetText(buf);
    fCalAnalyseBtn->SetEnabled(true);
}

float DetailWindow::DetectCirclePxMm(BBitmap* bmp, BRect normSel, float diamMm)
{
    if (!bmp || !bmp->IsValid()) return 0.0f;

    BRect bounds = bmp->Bounds();
    int w   = (int)(bounds.Width()  + 1);
    int h   = (int)(bounds.Height() + 1);
    int bpr = bmp->BytesPerRow();
    color_space cs = bmp->ColorSpace();
    const uint8* bits = (const uint8*)bmp->Bits();

    int x0 = (int)(normSel.left   * w);
    int y0 = (int)(normSel.top    * h);
    int x1 = (int)(normSel.right  * w);
    int y1 = (int)(normSel.bottom * h);
    if (x0 < 0) x0 = 0;
    if (x1 > w) x1 = w;
    if (y0 < 0) y0 = 0;
    if (y1 > h) y1 = h;

    int rw = x1 - x0, rh = y1 - y0;
    if (rw <= 0 || rh <= 0) return 0.0f;

    std::vector<bool> visited(rw * rh, false);
    auto idx = [&](int x, int y) { return (y - y0)*rw + (x - x0); };

    int maxArea = 0;

    for (int gy = y0; gy < y1; gy++) {
        for (int gx = x0; gx < x1; gx++) {
            if (visited[idx(gx, gy)]) continue;
            float L = pixelLuminance(bits, bpr, cs, gx, gy);
            if (L < 0.7f) { visited[idx(gx, gy)] = true; continue; }

            std::stack<std::pair<int,int>> stk;
            stk.push({gx, gy});
            visited[idx(gx, gy)] = true;
            int area = 0;

            while (!stk.empty()) {
                auto [cx, cy] = stk.top(); stk.pop();
                area++;
                const int dx4[] = {1,-1,0,0};
                const int dy4[] = {0,0,1,-1};
                for (int d = 0; d < 4; d++) {
                    int nx = cx + dx4[d];
                    int ny = cy + dy4[d];
                    if (nx < x0 || nx >= x1 || ny < y0 || ny >= y1) continue;
                    if (visited[idx(nx, ny)]) continue;
                    visited[idx(nx, ny)] = true;
                    if (pixelLuminance(bits, bpr, cs, nx, ny) >= 0.7f)
                        stk.push({nx, ny});
                }
            }

            if (area > maxArea) maxArea = area;
        }
    }

    if (maxArea <= 0) return 0.0f;
    float diam_px = 2.0f * sqrtf((float)maxArea / 3.14159f);
    return diam_px / diamMm;
}

float DetailWindow::DetectLinesPxMm(BBitmap* bmp, BRect normSel, float spacingMm)
{
    if (!bmp || !bmp->IsValid()) return 0.0f;

    BRect bounds = bmp->Bounds();
    int w   = (int)(bounds.Width()  + 1);
    int h   = (int)(bounds.Height() + 1);
    int bpr = bmp->BytesPerRow();
    color_space cs = bmp->ColorSpace();
    const uint8* bits = (const uint8*)bmp->Bits();

    int x0 = (int)(normSel.left   * w);
    int y0 = (int)(normSel.top    * h);
    int x1 = (int)(normSel.right  * w);
    int y1 = (int)(normSel.bottom * h);
    if (x0 < 0) x0 = 0;
    if (x1 > w) x1 = w;
    if (y0 < 0) y0 = 0;
    if (y1 > h) y1 = h;

    int rw = x1 - x0, rh = y1 - y0;
    if (rw <= 0 || rh <= 0) return 0.0f;

    // Per-row average luminance
    std::vector<float> rowLum(rh, 0.0f);
    for (int gy = y0; gy < y1; gy++) {
        float sum = 0.0f;
        for (int gx = x0; gx < x1; gx++)
            sum += pixelLuminance(bits, bpr, cs, gx, gy);
        rowLum[gy - y0] = sum / rw;
    }

    // 5-row moving average smooth
    std::vector<float> smooth(rh, 0.0f);
    for (int i = 0; i < rh; i++) {
        float s = 0.0f; int cnt = 0;
        for (int j = std::max(0, i-2); j <= std::min(rh-1, i+2); j++) {
            s += rowLum[j]; cnt++;
        }
        smooth[i] = s / cnt;
    }

    // Global mean
    float gmean = 0.0f;
    for (float v : smooth) gmean += v;
    gmean /= rh;

    // Local maxima above global mean
    std::vector<int> peaks;
    for (int i = 1; i < rh - 1; i++) {
        if (smooth[i] > smooth[i-1] && smooth[i] > smooth[i+1] && smooth[i] > gmean)
            peaks.push_back(i);
    }

    if ((int)peaks.size() < 2) return 0.0f;

    float totalSpacing = 0.0f;
    for (int i = 1; i < (int)peaks.size(); i++)
        totalSpacing += peaks[i] - peaks[i-1];
    float avgSpacing = totalSpacing / ((int)peaks.size() - 1);

    return avgSpacing / spacingMm;
}

// ===================================================================
// UpdateUnits
// Called when the mm/inches setting changes.  Updates all unit-dependent
// labels in Mode 3 (Calibrated Sheet) and redraws the histogram.

void DetailWindow::UpdateUnits()
{
    const char* unit = CoffeeSettings::Get()->UseMetric() ? "mm" : "in";

    // Histogram panel label (static view found by name)
    BStringView* histLbl = (BStringView*)FindView("cal_hlbl");
    if (histLbl) {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "Particle diameter histogram (%s):", unit);
        histLbl->SetText(lbl);
    }

    // Calibration reference-size label and reset calibration state,
    // since the stored px/mm was derived from a value in the old unit.
    UpdateCalSubMethod();
    fCalResultView->SetText("");
    fCalHist->Invalidate();
}

// MessageReceived
// ===================================================================

void DetailWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {

        // ---- Mode switching ----
        case MSG_PART_MODE_PHOTO:
            SwitchMode(kModePhoto);
            break;
        case MSG_PART_MODE_SIEVE:
            SwitchMode(kModeSieve);
            break;
        case MSG_PART_MODE_CAL:
            SwitchMode(kModeCal);
            break;

        // ---- Photo Estimate ----
        case MSG_PART_OPEN: {
            if (!fPhotoFilePanel) {
                BMessenger me(this);
                fPhotoFilePanel = new BFilePanel(B_OPEN_PANEL, &me, nullptr,
                                                 B_FILE_NODE, false,
                                                 new BMessage(MSG_PART_REFS));
            }
            fPhotoFilePanel->Show();
            break;
        }
        case MSG_PART_REFS: {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK)
                LoadPhotoImage(ref);
            break;
        }

        // ---- Sieve Cascade ----
        case MSG_SIEVE_OPEN: {
            if (!fSieveFilePanel) {
                BMessenger me(this);
                fSieveFilePanel = new BFilePanel(B_OPEN_PANEL, &me, nullptr,
                                                 B_FILE_NODE, false,
                                                 new BMessage(MSG_SIEVE_REFS));
            }
            fSieveFilePanel->Show();
            break;
        }
        case MSG_SIEVE_REFS: {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK)
                LoadSieveFraction(ref);
            break;
        }
        case MSG_SIEVE_ADD: {
            if (!fSieveSource) break;
            // Get current sieve size from menu
            BMenuItem* marked = fSieveSizeMenu->FindMarked();
            int sizeUm = 600;
            if (marked) {
                BMessage* bmsg = marked->Message();
                if (bmsg) bmsg->FindInt32("size_um", (int32*)&sizeUm);
            }
            ThumbView* tv = (ThumbView*)fSieveThumb;
            BRect normSel;
            if (tv->HasSelection()) normSel = tv->NormalisedSelection();
            float area = ComputeFractionArea(fSieveSource, normSel);

            // Replace existing entry for this size, or add new
            bool found = false;
            for (auto& f : fSieveFractions) {
                if (f.sizeUm == sizeUm) {
                    f.relArea = area;
                    found = true;
                    break;
                }
            }
            if (!found)
                fSieveFractions.push_back({ sizeUm, area });

            UpdateSieveChart();
            break;
        }
        case MSG_SIEVE_RESET: {
            fSieveFractions.clear();
            UpdateSieveChart();
            fSieveSummary->SetText("Distribution reset.");
            break;
        }

        // ---- Calibrated Sheet ----
        case MSG_CAL_OPEN: {
            if (!fCalFilePanel) {
                BMessenger me(this);
                fCalFilePanel = new BFilePanel(B_OPEN_PANEL, &me, nullptr,
                                               B_FILE_NODE, false,
                                               new BMessage(MSG_CAL_REFS));
            }
            fCalFilePanel->Show();
            break;
        }
        case MSG_CAL_REFS: {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK)
                LoadCalImage(ref);
            break;
        }
        case MSG_CAL_SUB_CIRCLE:
        case MSG_CAL_SUB_LINES:
            UpdateCalSubMethod();
            break;
        case MSG_CAL_CALIBRATE:
            RunCalibrate();
            break;
        case MSG_CAL_ANALYSE:
            AnalyseCal();
            break;

        // ---- Shared: ThumbView selection (route to active mode) ----
        case MSG_SELECTION_CHANGED: {
            bool hasSel = msg->HasFloat("x0");
            if (fMode == kModePhoto) {
                fPhotoClearBtn->SetEnabled(hasSel);
                if (fPhotoSource) AnalysePhoto();
            } else if (fMode == kModeSieve) {
                fSieveClearBtn->SetEnabled(hasSel);
            } else {
                fCalClearBtn->SetEnabled(hasSel);
            }
            break;
        }
        case MSG_CLEAR_SELECTION: {
            if (fMode == kModePhoto) {
                ((ThumbView*)fPhotoThumb)->ClearSelection();
                fPhotoClearBtn->SetEnabled(false);
                if (fPhotoSource) AnalysePhoto();
            } else if (fMode == kModeSieve) {
                ((ThumbView*)fSieveThumb)->ClearSelection();
                fSieveClearBtn->SetEnabled(false);
            } else {
                ((ThumbView*)fCalThumb)->ClearSelection();
                fCalClearBtn->SetEnabled(false);
            }
            break;
        }

        case MSG_SET_UNIT_MM:
        case MSG_SET_UNIT_INCHES:
            if (CoffeeSettings::Get()->HandleSettingsMessage(msg))
                UpdateUnits();
            break;

        case MSG_THEME_CHANGED: {
            CoffeeSettings* s = CoffeeSettings::Get();
            rgb_color bg = s->ThemePanelBg();
            rgb_color tc = s->ThemeTextColor();
            fPhotoTipsView->SetViewColor(bg);
            fPhotoTipsView->SetFontAndColor(0, fPhotoTipsView->TextLength(),
                                            be_plain_font, B_FONT_ALL, &tc);
            fPhotoTipsView->Invalidate();
            fPhotoGauge->Invalidate();
            fSieveDist->Invalidate();
            fCalHist->Invalidate();
            break;
        }

        default:
            if (CoffeeSettings::Get()->HandleSettingsMessage(msg))
                break;
            BWindow::MessageReceived(msg);
    }
}
