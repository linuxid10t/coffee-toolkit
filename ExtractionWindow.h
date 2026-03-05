#pragma once
/*  ExtractionWindow.h – Coffee Toolkit
 *
 *  Extraction yield calculator with TDS/Brix input,
 *  percolation/immersion toggle, a colour-coded gauge,
 *  and contextual brew tips.
 */

#include <Window.h>
#include <Button.h>
#include <GroupView.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

// -------------------------------------------------------
// ExtractionBarView
//
//  Horizontal bar spanning 0–35%.
//    under-extracted (< ideal_lo) → blue
//    ideal zone                   → green
//    over-extracted  (> ideal_hi) → red
//  A triangle pointer marks the current value.
// -------------------------------------------------------
class ExtractionBarView : public BView {
public:
    ExtractionBarView(BRect frame);
    void Draw(BRect updateRect) override;

    // Pass extraction = -1 to show an empty bar.
    void SetExtraction(float extraction, float idealLo, float idealHi);

private:
    float fExtraction;
    float fIdealLo;
    float fIdealHi;

    static const float kBarMax;
};

// -------------------------------------------------------
// ExtractionWindow
// -------------------------------------------------------
class ExtractionWindow : public BWindow {
public:
    ExtractionWindow();
    void MessageReceived(BMessage* msg) override;

private:
    void Calculate();
    void UpdateInputLabels();
    void UpdateToggleStatus();

    // Input mode radios
    BRadioButton* fTdsRadio;
    BRadioButton* fBrixRadio;
    BStringView*  fMeasureLbl;
    BTextControl* fMeasureCtl;

    // Brew type radios
    BRadioButton* fPercRadio;
    BRadioButton* fImmsRadio;

    // Status label
    BStringView*  fStatusView;

    // Inputs
    BStringView*  fLiquidLbl;
    BTextControl* fLiquidCtl;
    BStringView*  fDoseLbl;
    BTextControl* fDoseCtl;

    // Calculate button
    BButton*      fCalcBtn;

    // Results
    ExtractionBarView* fBarView;
    BTextView*         fTipsView;
    BScrollView*       fTipsScroll;

    // Radio group containers (isolate the two radio pairs)
    BGroupView* fMeasureGroup;
    BGroupView* fBrewGroup;

    // State
    bool fUseBrix;
    bool fIsPercolation;
};
