#pragma once
/*  DetailWindow.h – Coffee Toolkit
 *
 *  Generic placeholder window used for tools not yet implemented.
 *  Will be replaced by the Particle Analyzer.
 */

#include <Window.h>

class DetailWindow : public BWindow {
public:
    explicit DetailWindow(const char* title);
    void MessageReceived(BMessage* msg) override;
};
