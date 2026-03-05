#pragma once
/*  BrewRatioWindow.h – Coffee Toolkit
 *
 *  Calculates coffee-to-water ratio from a preset or custom
 *  brew ratio and a water volume input.
 */

#include <Window.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TextControl.h>

class BrewRatioWindow : public BWindow {
public:
    BrewRatioWindow();
    void MessageReceived(BMessage* msg) override;

private:
    void  Calculate();
    float CurrentRatio() const;

    BPopUpMenu*   fRatioMenu;
    BMenuField*   fRatioField;
    BStringView*  fCustomRatioLbl;
    BTextControl* fCustomRatioCtl;
    BStringView*  fWaterLbl;
    BTextControl* fWaterCtl;
    BButton*      fCalcBtn;
    BStringView*  fResultView;
    bool          fCustomMode;
    float         fCurrentRatio;
};
