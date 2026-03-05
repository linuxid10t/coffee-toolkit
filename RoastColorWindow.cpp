/*  RoastColorWindow.cpp – Coffee Toolkit */

#include "RoastColorWindow.h"
#include "Constants.h"

#include <Application.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Path.h>
#include <shared/ToolBar.h>
#include <TranslationUtils.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// -------------------------------------------------------
// RoastGaugeView
// -------------------------------------------------------
RoastGaugeView::RoastGaugeView(BRect frame)
    : BView(frame, "roast_gauge", B_FOLLOW_H_CENTER | B_FOLLOW_TOP,
            B_WILL_DRAW),
      fAgtron(-1.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void RoastGaugeView::SetAgtron(float agtron)
{
    fAgtron = agtron;
    Invalidate();
}

void RoastGaugeView::Draw(BRect /*updateRect*/)
{
    BRect b = Bounds();
    float barTop    = 18.0f;
    float barBottom = 44.0f;
    float barH      = barBottom - barTop;
    float barLeft   = b.left  + 2.0f;
    float barRight  = b.right - 2.0f;
    float barWidth  = barRight - barLeft;

    auto agtronToX = [&](float ag) -> float {
        float t = (ag - kAgtronMin) / (kAgtronMax - kAgtronMin);
        return barLeft + t * barWidth;
    };

    // Gradient bar – 7 colour stops from very dark to very light roast
    struct Stop { float pos; uint8 r, g, b; };
    Stop stops[] = {
        { 0.00f,  10,  5,  2 },
        { 0.17f,  40, 18,  8 },
        { 0.33f,  80, 38, 15 },
        { 0.50f, 130, 72, 30 },
        { 0.67f, 175,115, 55 },
        { 0.83f, 210,165,100 },
        { 1.00f, 235,210,165 },
    };
    const int nStops = 7;

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
    SetHighColor(60, 60, 60);
    StrokeRect(BRect(barLeft, barTop, barRight, barBottom));

    // Roast level band dividers and labels
    struct Band { float ag; const char* name; };
    Band bands[] = {
        { 25, "V.Dark"  },
        { 35, "Dark"    },
        { 45, "M-Dark"  },
        { 55, "Medium"  },
        { 65, "Light"   },
        { 75, "V.Light" },
        { 95, nullptr   },
    };
    BFont tiny; tiny.SetSize(8.5f); SetFont(&tiny);
    for (int i = 0; i < 6; i++) {
        if (i < 5) {
            SetHighColor(100, 100, 100);
            float xd = agtronToX(bands[i+1].ag);
            StrokeLine(BPoint(xd, barTop), BPoint(xd, barBottom));
        }
        float xL  = agtronToX(bands[i].ag);
        float xR  = agtronToX(bands[i+1].ag);
        SetHighColor(220, 220, 200);
        DrawString(bands[i].name,
                   BPoint((xL+xR)/2.0f - 14, barTop + barH*0.65f));
    }

    // Scale ticks
    BFont small; small.SetSize(9.0f); SetFont(&small);
    SetHighColor(60, 60, 60);
    for (int ag = 25; ag <= 95; ag += 10) {
        float x = agtronToX((float)ag);
        StrokeLine(BPoint(x, barBottom), BPoint(x, barBottom+4));
        char lbl[8]; snprintf(lbl, sizeof(lbl), "%d", ag);
        DrawString(lbl, BPoint(x-5, barBottom+14));
    }

    // Pointer
    if (fAgtron >= kAgtronMin && fAgtron <= kAgtronMax + 1.0f) {
        float x = agtronToX(fAgtron);
        SetHighColor(255, 255, 255);
        FillTriangle(BPoint(x, barTop-1),
                     BPoint(x-5, barTop-9),
                     BPoint(x+5, barTop-9));
        SetHighColor(30, 30, 30);
        StrokeTriangle(BPoint(x, barTop-1),
                       BPoint(x-5, barTop-9),
                       BPoint(x+5, barTop-9));
        SetHighColor(255, 255, 255);
        SetDrawingMode(B_OP_OVER);
        StrokeLine(BPoint(x, barTop), BPoint(x, barBottom));
        SetDrawingMode(B_OP_COPY);
        char val[16]; snprintf(val, sizeof(val), "%.0f", fAgtron);
        BFont norm; norm.SetSize(10.0f); SetFont(&norm);
        SetHighColor(10, 10, 10);
        DrawString(val, BPoint(x-8, barTop-11));
    }
}

// -------------------------------------------------------
// ThumbView
// -------------------------------------------------------
ThumbView::ThumbView(BRect frame, BMessenger target)
    : BView(frame, "thumb", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fBitmap(nullptr),
      fTarget(target),
      fHasSelection(false),
      fDragging(false)
{
    SetViewColor(40, 40, 40);
}

void ThumbView::SetBitmap(BBitmap* bmp)
{
    fBitmap = bmp;
    ClearSelection();
    Invalidate();
}

void ThumbView::ClearSelection()
{
    fHasSelection = false;
    fDragging     = false;
    NotifyTarget();
    Invalidate();
}

BRect ThumbView::NormalisedSelection() const
{
    if (!fHasSelection) return BRect();
    BRect b = Bounds();
    float w = b.Width()  + 1.0f;
    float h = b.Height() + 1.0f;
    return BRect(fSelection.left   / w,
                 fSelection.top    / h,
                 fSelection.right  / w,
                 fSelection.bottom / h);
}

BRect ThumbView::OrderedRect(BPoint a, BPoint b) const
{
    BRect bnd = Bounds();
    auto clampX = [&](float x) {
        return x < bnd.left ? bnd.left
             : x > bnd.right ? bnd.right : x;
    };
    auto clampY = [&](float y) {
        return y < bnd.top ? bnd.top
             : y > bnd.bottom ? bnd.bottom : y;
    };
    return BRect(clampX(a.x < b.x ? a.x : b.x),
                 clampY(a.y < b.y ? a.y : b.y),
                 clampX(a.x > b.x ? a.x : b.x),
                 clampY(a.y > b.y ? a.y : b.y));
}

void ThumbView::NotifyTarget()
{
    BMessage msg(MSG_SELECTION_CHANGED);
    if (fHasSelection) {
        BRect n = NormalisedSelection();
        msg.AddFloat("x0", n.left);
        msg.AddFloat("y0", n.top);
        msg.AddFloat("x1", n.right);
        msg.AddFloat("y1", n.bottom);
    }
    fTarget.SendMessage(&msg);
}

void ThumbView::MouseDown(BPoint where)
{
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
    fDragging     = true;
    fDragStart    = where;
    fDragCurrent  = where;
    fHasSelection = false;
    Invalidate();
}

void ThumbView::MouseMoved(BPoint where, uint32 /*transit*/,
                           const BMessage* /*drag*/)
{
    if (!fDragging) return;
    fDragCurrent = where;
    Invalidate();
}

void ThumbView::MouseUp(BPoint where)
{
    if (!fDragging) return;
    fDragging = false;
    BRect r = OrderedRect(fDragStart, where);
    if (r.Width() > 4 && r.Height() > 4) {
        fSelection    = r;
        fHasSelection = true;
    } else {
        fHasSelection = false;
    }
    NotifyTarget();
    Invalidate();
}

void ThumbView::Draw(BRect /*updateRect*/)
{
    BRect b = Bounds();

    if (fBitmap && fBitmap->IsValid()) {
        DrawBitmapAsync(fBitmap, fBitmap->Bounds(),
                        b, B_FILTER_BITMAP_BILINEAR);
    } else {
        SetHighColor(80, 80, 80);
        BFont f; f.SetSize(11); SetFont(&f);
        DrawString("No image loaded", BPoint(10, b.Height()/2));
        SetHighColor(60, 60, 60);
        DrawString("Click 'Load Image' above",
                   BPoint(10, b.Height()/2 + 16));
    }

    // Draw selection overlay
    BRect selRect;
    if (fDragging)
        selRect = OrderedRect(fDragStart, fDragCurrent);
    else if (fHasSelection)
        selRect = fSelection;
    else
        return;

    // Dashed white then offset black gives static marching-ants look
    const float dash = 6.0f;
    auto drawDashedRect = [&](rgb_color col, float offset) {
        SetHighColor(col);
        SetDrawingMode(B_OP_OVER);
        for (int edge = 0; edge < 2; edge++) {
            float y = (edge == 0) ? selRect.top : selRect.bottom;
            float x = selRect.left + offset;
            bool on = true;
            while (x < selRect.right) {
                float xe = x + dash;
                if (xe > selRect.right) xe = selRect.right;
                if (on) StrokeLine(BPoint(x, y), BPoint(xe, y));
                x = xe; on = !on;
            }
        }
        for (int edge = 0; edge < 2; edge++) {
            float x = (edge == 0) ? selRect.left : selRect.right;
            float y = selRect.top + offset;
            bool on = true;
            while (y < selRect.bottom) {
                float ye = y + dash;
                if (ye > selRect.bottom) ye = selRect.bottom;
                if (on) StrokeLine(BPoint(x, y), BPoint(x, ye));
                y = ye; on = !on;
            }
        }
        SetDrawingMode(B_OP_COPY);
    };
    drawDashedRect({255, 255, 255, 255}, 0.0f);
    drawDashedRect({0,   0,   0,   255}, dash);

    // Darken area outside selection
    SetHighColor(0, 0, 0, 80);
    SetDrawingMode(B_OP_ALPHA);
    if (selRect.top > b.top)
        FillRect(BRect(b.left, b.top, b.right, selRect.top - 1));
    if (selRect.bottom < b.bottom)
        FillRect(BRect(b.left, selRect.bottom + 1, b.right, b.bottom));
    if (selRect.left > b.left)
        FillRect(BRect(b.left, selRect.top, selRect.left - 1, selRect.bottom));
    if (selRect.right < b.right)
        FillRect(BRect(selRect.right+1, selRect.top, b.right, selRect.bottom));
    SetDrawingMode(B_OP_COPY);
}

// -------------------------------------------------------
// RoastColorWindow
// -------------------------------------------------------
RoastColorWindow::RoastColorWindow()
    : BWindow(BRect(180, 120, 720, 760), "Roast Color Analyzer",
              B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fFilePanel(nullptr),
      fThumbBitmap(nullptr),
      fSourceBitmap(nullptr)
{
    fOpenBtn = new BButton("open_btn", "Load Image...",
                           new BMessage(MSG_ROAST_OPEN));

    fFileNameView = new BStringView("fname", "No image loaded.");
    fFileNameView->SetExplicitAlignment(
        BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

    BMessenger me(this);
    fThumbView = new ThumbView(BRect(0, 0, 319, 239), me);
    fThumbView->SetExplicitMinSize(BSize(320, 240));
    fThumbView->SetExplicitMaxSize(BSize(320, 240));

    fSelHintView = new BStringView("sel_hint",
        "Drag on the image to select a sample region.");
    BFont hintFont(be_plain_font);
    hintFont.SetFace(B_ITALIC_FACE);
    fSelHintView->SetFont(&hintFont);

    fClearSelBtn = new BButton("clear_sel", "Clear Selection",
                               new BMessage(MSG_CLEAR_SELECTION));
    fClearSelBtn->SetEnabled(false);

    fGaugeView = new RoastGaugeView(BRect(0, 0, 460, 70));
    fGaugeView->SetExplicitMinSize(BSize(460, 70));
    fGaugeView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 70));

    fAgtronView = new BStringView("agtron_result", "");
    fAgtronView->SetFont(be_bold_font);
    fAgtronView->SetExplicitAlignment(
        BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));

    BRect tvFrame(0, 0, 440, 150);
    fTipsView = new BTextView(tvFrame, "roast_tips",
                              tvFrame.OffsetToCopy(B_ORIGIN),
                              B_FOLLOW_ALL, B_WILL_DRAW);
    fTipsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fTipsView->MakeEditable(false);
    fTipsView->MakeSelectable(false);
    fTipsView->SetWordWrap(true);
    fTipsView->SetText(
        "Load a photograph of your coffee grounds or whole beans.\n\n"
        "For best accuracy:\n"
        "  * Use ground coffee (consistent surface)\n"
        "  * Photograph on a neutral grey or white card\n"
        "  * Use even, diffuse lighting - avoid direct sunlight\n"
        "  * Fill most of the frame with coffee\n\n"
        "Drag a selection rectangle on the preview to sample a\n"
        "specific region instead of the default centre crop.");

    fTipsScroll = new BScrollView("roast_tips_scroll", fTipsView,
                                  B_FOLLOW_ALL, 0, false, true);
    fTipsScroll->SetExplicitMinSize(BSize(460, 130));
    fTipsScroll->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 200));

    BToolBar* toolbar = new BToolBar(B_HORIZONTAL);
    toolbar->AddAction(B_ABOUT_REQUESTED, be_app, nullptr, "About", "About");

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(toolbar)
        .AddGroup(B_VERTICAL, kPad)
            .SetInsets(kPad*2, kPad*2, kPad*2, kPad*2)
            .AddGroup(B_HORIZONTAL, kPad)
                .Add(fOpenBtn)
                .Add(fFileNameView)
                .End()
            .Add(fThumbView)
            .AddGroup(B_HORIZONTAL, kPad)
                .Add(fSelHintView)
                .AddGlue()
                .Add(fClearSelBtn)
                .End()
            .Add(new BStringView("sep1", ""))
            .Add(new BStringView("gauge_lbl", "Agtron roast scale:"))
            .Add(fGaugeView)
            .Add(fAgtronView)
            .Add(new BStringView("sep2", ""))
            .Add(new BStringView("tips_lbl", "Roast notes & guidance:"))
            .Add(fTipsScroll)
            .End();

    ResizeTo(GetLayout()->PreferredSize().width  + 40,
             GetLayout()->PreferredSize().height + 40);
    CenterOnScreen();
}

RoastColorWindow::~RoastColorWindow()
{
    delete fSourceBitmap;
    delete fThumbBitmap;
    delete fFilePanel;
}

void RoastColorWindow::LoadImage(const entry_ref& ref)
{
    BBitmap* bmp = BTranslationUtils::GetBitmapFile(BPath(&ref).Path());
    if (!bmp || !bmp->IsValid()) {
        delete bmp;
        fFileNameView->SetText("Error: could not load image.");
        return;
    }

    delete fSourceBitmap;
    fSourceBitmap = bmp;

    BEntry entry(&ref);
    char name[B_FILE_NAME_LENGTH];
    entry.GetName(name);
    fFileNameView->SetText(name);

    // Scale thumbnail to fit 320x240, preserving aspect ratio
    BRect src = fSourceBitmap->Bounds();
    float sw = src.Width() + 1;
    float sh = src.Height() + 1;
    float scale = (sw/sh > 320.0f/240.0f) ? 320.0f/sw : 240.0f/sh;
    int tw = (int)(sw * scale);
    int th = (int)(sh * scale);

    delete fThumbBitmap;
    fThumbBitmap = new BBitmap(BRect(0, 0, tw-1, th-1), B_RGB32);

    uint8* src_bits = (uint8*)fSourceBitmap->Bits();
    uint8* dst_bits = (uint8*)fThumbBitmap->Bits();
    int    src_bpr  = fSourceBitmap->BytesPerRow();
    int    dst_bpr  = fThumbBitmap->BytesPerRow();
    color_space cs  = fSourceBitmap->ColorSpace();

    for (int dy = 0; dy < th; dy++) {
        int sy = (int)(dy * sh / th);
        for (int dx = 0; dx < tw; dx++) {
            int sx = (int)(dx * sw / tw);
            uint8 r = 128, g = 128, bv = 128;
            if (cs == B_RGB32 || cs == B_RGBA32) {
                uint8* p = src_bits + sy*src_bpr + sx*4;
                bv = p[0]; g = p[1]; r = p[2];
            } else if (cs == B_RGB24) {
                uint8* p = src_bits + sy*src_bpr + sx*3;
                bv = p[0]; g = p[1]; r = p[2];
            }
            uint8* d = dst_bits + dy*dst_bpr + dx*4;
            d[0] = bv; d[1] = g; d[2] = r; d[3] = 255;
        }
    }

    fThumbView->SetBitmap(fThumbBitmap);
    fClearSelBtn->SetEnabled(false);
    Analyse();
}

float RoastColorWindow::ComputeAgtron(BBitmap* bmp, BRect normSel)
{
    BRect bounds = bmp->Bounds();
    int   w   = (int)(bounds.Width()  + 1);
    int   h   = (int)(bounds.Height() + 1);
    int   bpr = bmp->BytesPerRow();
    color_space cs = bmp->ColorSpace();
    uint8* bits = (uint8*)bmp->Bits();

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
    if (x0 < 0)  x0 = 0;
    if (x1 > w)  x1 = w;
    if (y0 < 0)  y0 = 0;
    if (y1 > h)  y1 = h;
    if (x0 >= x1 || y0 >= y1) return 50.0f;

    double sumL = 0.0;
    int    count = 0;

    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            float rf = 0, gf = 0, bf = 0;
            if (cs == B_RGB32 || cs == B_RGBA32) {
                uint8* p = bits + y*bpr + x*4;
                bf = p[0]/255.0f; gf = p[1]/255.0f; rf = p[2]/255.0f;
            } else if (cs == B_RGB24) {
                uint8* p = bits + y*bpr + x*3;
                bf = p[0]/255.0f; gf = p[1]/255.0f; rf = p[2]/255.0f;
            } else continue;

            auto lin = [](float c) -> float {
                return c <= 0.04045f ? c/12.92f
                                     : powf((c+0.055f)/1.055f, 2.4f);
            };
            float L = 0.2126f*lin(rf) + 0.7152f*lin(gf) + 0.0722f*lin(bf);
            sumL += L;
            count++;
        }
    }

    if (count == 0) return 50.0f;
    float avgL   = (float)(sumL / count);
    float agtron = kAgtronMin + avgL * (kAgtronMax - kAgtronMin);
    if (agtron < kAgtronMin) agtron = kAgtronMin;
    if (agtron > kAgtronMax) agtron = kAgtronMax;
    return agtron;
}

const char* RoastColorWindow::RoastName(float ag)
{
    if (ag >= 75) return "Very Light  (Cinnamon / White Roast)";
    if (ag >= 65) return "Light  (City)";
    if (ag >= 55) return "Medium  (City+)";
    if (ag >= 45) return "Medium-Dark  (Full City / Full City+)";
    if (ag >= 35) return "Dark  (Vienna / French)";
    return              "Very Dark  (Italian / Spanish)";
}

void RoastColorWindow::BuildTips(float ag, char* buf, size_t sz)
{
    const char* name = RoastName(ag);
    if (ag >= 75) {
        snprintf(buf, sz,
            "Roast level: %s\n\nVery lightly roasted coffee retains maximum "
            "origin character.\n\nFlavour notes: floral, fruity, tea-like, "
            "bright acidity.\nBrew tips:\n"
            "  * Use lower water temperature (85-90 deg C).\n"
            "  * Grind finer than you would for a medium roast.\n"
            "  * Well-suited to pour-over and Chemex methods.\n"
            "  * Avoid espresso - low solubility makes it difficult.", name);
    } else if (ag >= 65) {
        snprintf(buf, sz,
            "Roast level: %s\n\nA light roast with good clarity and origin "
            "expression.\n\nFlavour notes: stone fruit, citrus, caramel "
            "sweetness.\nBrew tips:\n"
            "  * Water temperature 88-93 deg C.\n"
            "  * Works well with V60, Kalita Wave, AeroPress.\n"
            "  * Can pull as espresso with extended pre-infusion.\n"
            "  * Rest 7-14 days post-roast for CO2 degassing.", name);
    } else if (ag >= 55) {
        snprintf(buf, sz,
            "Roast level: %s\n\nThe most versatile roast - balanced across "
            "all brew methods.\n\nFlavour notes: chocolate, nuts, mild fruit, "
            "balanced acidity.\nBrew tips:\n"
            "  * Water temperature 90-94 deg C.\n"
            "  * Excellent for espresso, filter, and immersion methods.\n"
            "  * Aim for 18-22%% extraction yield.\n"
            "  * Rest 5-10 days post-roast.", name);
    } else if (ag >= 45) {
        snprintf(buf, sz,
            "Roast level: %s\n\nA darker roast with developed body and "
            "reduced acidity.\n\nFlavour notes: dark chocolate, caramel, "
            "toasted nuts.\nBrew tips:\n"
            "  * Water temperature 90-93 deg C.\n"
            "  * Ideal for espresso and French press.\n"
            "  * Grind slightly coarser to prevent over-extraction.\n"
            "  * Milk-based drinks complement this roast level well.", name);
    } else if (ag >= 35) {
        snprintf(buf, sz,
            "Roast level: %s\n\nA dark roast with bold body and low origin "
            "character.\n\nFlavour notes: bittersweet chocolate, smoky.\n"
            "Brew tips:\n"
            "  * Water temperature 88-92 deg C.\n"
            "  * Well-suited to espresso and moka pot.\n"
            "  * Use a coarser grind to prevent bitterness.\n"
            "  * Can stand up to milk and sugar.", name);
    } else {
        snprintf(buf, sz,
            "Roast level: %s\n\nA very dark roast - roast character "
            "dominates entirely.\n\nFlavour notes: charred, smoky, ash.\n"
            "Brew tips:\n"
            "  * Water temperature 85-90 deg C.\n"
            "  * Best suited to espresso only.\n"
            "  * Use a coarser grind - oils clog finer settings.\n"
            "  * Origin flavour is largely absent at this level.", name);
    }
}

void RoastColorWindow::Analyse()
{
    if (!fSourceBitmap || !fSourceBitmap->IsValid()) return;

    BRect normSel;
    if (fThumbView->HasSelection())
        normSel = fThumbView->NormalisedSelection();

    float agtron = ComputeAgtron(fSourceBitmap, normSel);
    fGaugeView->SetAgtron(agtron);

    char label[192];
    snprintf(label, sizeof(label),
             "Agtron: %.0f   --   %s  [%s]",
             agtron, RoastName(agtron),
             fThumbView->HasSelection() ? "custom selection" : "centre region");
    fAgtronView->SetText(label);

    char tips[2048];
    BuildTips(agtron, tips, sizeof(tips));
    fTipsView->SetText(tips);
}

void RoastColorWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case MSG_ROAST_OPEN: {
            if (!fFilePanel) {
                BMessenger me(this);
                fFilePanel = new BFilePanel(B_OPEN_PANEL, &me, nullptr,
                                            B_FILE_NODE, false,
                                            new BMessage(MSG_ROAST_REFS));
            }
            fFilePanel->Show();
            break;
        }
        case MSG_ROAST_REFS: {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK)
                LoadImage(ref);
            break;
        }
        case MSG_SELECTION_CHANGED: {
            fClearSelBtn->SetEnabled(msg->HasFloat("x0"));
            if (fSourceBitmap)
                Analyse();
            break;
        }
        case MSG_CLEAR_SELECTION: {
            fThumbView->ClearSelection();
            fClearSelBtn->SetEnabled(false);
            if (fSourceBitmap)
                Analyse();
            break;
        }
        default:
            BWindow::MessageReceived(msg);
    }
}
