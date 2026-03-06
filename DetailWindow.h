#pragma once
/*  DetailWindow.h – Coffee Toolkit
 *
 *  Particle Size Analyzer — three analysis modes:
 *    1. Photo Estimate  : local contrast map on a single photo
 *    2. Sieve Cascade   : accumulate fractions from sieve photos
 *    3. Calibrated Sheet: per-particle sizing with a px/mm calibration
 */

#include <Window.h>
#include <Bitmap.h>
#include <Button.h>
#include <FilePanel.h>
#include <GroupView.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Rect.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <TextControl.h>
#include <View.h>

#include <vector>

// -------------------------------------------------------
// ParticleGaugeView
//
//  Horizontal bar from Extra-Fine (left) to Extra-Coarse
//  (right).  A pointer triangle marks the estimated score
//  (0.0 = extra fine, 1.0 = extra coarse, -1 = no reading).
// -------------------------------------------------------
class ParticleGaugeView : public BView {
public:
    ParticleGaugeView(BRect frame);
    void Draw(BRect updateRect) override;
    void SetScore(float score);  // -1 = no reading

private:
    float fScore;
};

// -------------------------------------------------------
// SieveDistView
//
//  Bar chart of accumulated sieve fractions.
//  Each entry holds a sieve size (µm) and relative area.
// -------------------------------------------------------
struct SieveFraction {
    int   sizeUm;
    float relArea;
};

class SieveDistView : public BView {
public:
    SieveDistView(BRect frame);
    void Draw(BRect updateRect) override;
    void SetFractions(const std::vector<SieveFraction>& fracs);

private:
    std::vector<SieveFraction> fFractions;
};

// -------------------------------------------------------
// CalHistView
//
//  Histogram of particle diameters (mm) from the
//  calibrated-sheet analysis, binned at 0.1 mm.
// -------------------------------------------------------
class CalHistView : public BView {
public:
    CalHistView(BRect frame);
    void Draw(BRect updateRect) override;
    void SetDiameters(const std::vector<float>& diameters_mm);

private:
    std::vector<float> fDiameters;
};

// -------------------------------------------------------
// DetailWindow  (Particle Analyzer)
// -------------------------------------------------------
class DetailWindow : public BWindow {
public:
    DetailWindow();
    ~DetailWindow();
    void MessageReceived(BMessage* msg) override;

private:
    enum { kModePhoto = 0, kModeSieve, kModeCal };

    void SwitchMode(int mode);

    // ---- Mode 1: Photo Estimate ----
    void  LoadPhotoImage(const entry_ref& ref);
    void  AnalysePhoto();
    float ComputeGrindScore(BBitmap* bmp, BRect normSel);
    const char* GrindName(float score);
    void  BuildPhotoTips(float score, char* buf, size_t sz);

    // ---- Mode 2: Sieve Cascade ----
    void  LoadSieveFraction(const entry_ref& ref);
    float ComputeFractionArea(BBitmap* bmp, BRect normSel);
    void  UpdateSieveChart();

    // ---- Mode 3: Calibrated Sheet ----
    void  LoadCalImage(const entry_ref& ref);
    void  AnalyseCal();

    // ---- Shared thumbnail helper ----
    BBitmap* ScaleBitmap(BBitmap* src, int tw, int th);

    // ---- Mode state ----
    int fMode;

    // ---- Mode radio buttons & panels ----
    BGroupView* fModeBar;
    BRadioButton* fPhotoRadio;
    BRadioButton* fSieveRadio;
    BRadioButton* fCalRadio;

    BGroupView* fPhotoPanel;
    BGroupView* fSievePanel;
    BGroupView* fCalPanel;

    // ---- Mode 1 widgets ----
    BButton*          fPhotoOpenBtn;
    BStringView*      fPhotoFileView;
    BFilePanel*       fPhotoFilePanel;
    BView*            fPhotoThumb;   // ThumbView* — forward-declared as BView*
    BButton*          fPhotoClearBtn;
    BStringView*      fPhotoHintView;
    BBitmap*          fPhotoSource;
    BBitmap*          fPhotoThumbBmp;
    ParticleGaugeView* fPhotoGauge;
    BStringView*      fPhotoResultView;
    BTextView*        fPhotoTipsView;
    BScrollView*      fPhotoTipsScroll;

    // ---- Mode 2 widgets ----
    BPopUpMenu*       fSieveSizeMenu;
    BMenuField*       fSieveSizeField;
    BButton*          fSieveOpenBtn;
    BStringView*      fSieveFileView;
    BFilePanel*       fSieveFilePanel;
    BView*            fSieveThumb;   // ThumbView*
    BButton*          fSieveClearBtn;
    BStringView*      fSieveHintView;
    BBitmap*          fSieveSource;
    BBitmap*          fSieveThumbBmp;
    BButton*          fSieveAddBtn;
    BButton*          fSieveResetBtn;
    SieveDistView*    fSieveDist;
    BStringView*      fSieveSummary;
    std::vector<SieveFraction> fSieveFractions;

    // ---- Mode 3 widgets ----
    BButton*          fCalOpenBtn;
    BStringView*      fCalFileView;
    BFilePanel*       fCalFilePanel;
    BView*            fCalThumb;     // ThumbView*
    BButton*          fCalClearBtn;
    BStringView*      fCalHintView;
    BBitmap*          fCalSource;
    BBitmap*          fCalThumbBmp;
    BTextControl*     fCalPxPerMmCtl;
    BButton*          fCalAnalyseBtn;
    CalHistView*      fCalHist;
    BStringView*      fCalResultView;
};
