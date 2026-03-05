#pragma once
/*  RoastColorWindow.h – Coffee Toolkit
 *
 *  Analyses a photograph of coffee to estimate its Agtron
 *  roast score, using CIE luminance of the sampled region.
 *
 *  Agtron scale: 25 (very dark/Italian) … 95 (very light/green)
 */

#include <Window.h>
#include <Bitmap.h>
#include <Button.h>
#include <FilePanel.h>
#include <Messenger.h>
#include <Rect.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <View.h>

// Agtron scale limits — shared by RoastGaugeView and RoastColorWindow
static const float kAgtronMin = 25.0f;
static const float kAgtronMax = 95.0f;

// -------------------------------------------------------
// RoastGaugeView
//
//  Horizontal bar with a dark-to-light coffee colour gradient
//  and roast-level band labels.  A white triangle pointer
//  marks the measured Agtron value.
// -------------------------------------------------------
class RoastGaugeView : public BView {
public:
    RoastGaugeView(BRect frame);
    void Draw(BRect updateRect) override;
    void SetAgtron(float agtron);   // pass -1 for no reading

private:
    float fAgtron;
};

// -------------------------------------------------------
// ThumbView
//
//  Displays a BBitmap preview.  The user can click-drag to
//  select a rectangular sampling region drawn as a dashed
//  white/black overlay.  Sends MSG_SELECTION_CHANGED to the
//  target messenger whenever the selection changes.
// -------------------------------------------------------
class ThumbView : public BView {
public:
    ThumbView(BRect frame, BMessenger target);

    void SetBitmap(BBitmap* bmp);   // also clears selection
    void ClearSelection();
    bool  HasSelection() const { return fHasSelection; }
    BRect NormalisedSelection() const;   // 0..1 in each axis

    void Draw(BRect updateRect) override;
    void MouseDown(BPoint where) override;
    void MouseMoved(BPoint where, uint32 transit,
                    const BMessage* drag) override;
    void MouseUp(BPoint where) override;

private:
    void  NotifyTarget();
    BRect OrderedRect(BPoint a, BPoint b) const;

    BBitmap*   fBitmap;
    BMessenger fTarget;

    bool   fHasSelection;
    bool   fDragging;
    BPoint fDragStart;
    BPoint fDragCurrent;
    BRect  fSelection;
};

// -------------------------------------------------------
// RoastColorWindow
// -------------------------------------------------------
class RoastColorWindow : public BWindow {
public:
    RoastColorWindow();
    ~RoastColorWindow();
    void MessageReceived(BMessage* msg) override;

private:
    void        LoadImage(const entry_ref& ref);
    void        Analyse();
    float       ComputeAgtron(BBitmap* bmp, BRect normSel);
    const char* RoastName(float agtron);
    void        BuildTips(float agtron, char* buf, size_t bufSize);

    // File selection
    BButton*      fOpenBtn;
    BStringView*  fFileNameView;
    BFilePanel*   fFilePanel;

    // Preview
    ThumbView*    fThumbView;
    BButton*      fClearSelBtn;
    BStringView*  fSelHintView;
    BBitmap*      fThumbBitmap;
    BBitmap*      fSourceBitmap;

    // Results
    RoastGaugeView* fGaugeView;
    BStringView*    fAgtronView;
    BTextView*      fTipsView;
    BScrollView*    fTipsScroll;
};
