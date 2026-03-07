#pragma once
/*  Settings.h – Coffee Toolkit
 *
 *  Singleton settings manager with persistent storage
 *  and cross-window menu synchronisation.
 */

#include <InterfaceDefs.h>
#include <SupportDefs.h>

class BMenuBar;
class BMessage;

class CoffeeSettings {
public:
    static CoffeeSettings*  Get();

    void                    Load();
    void                    Save();

    static void             BuildSettingsMenu(BMenuBar* menuBar);
    bool                    HandleSettingsMessage(BMessage* msg);

    // Current values
    bool        UseCelsius() const      { return fCelsius; }
    bool        UseMetric() const       { return fMetric; }
    int32       DefaultRatio() const    { return fRatio; }
    int32       Theme() const           { return fTheme; }
    const char* Language() const        { return fLanguage; }

    // Theme color accessors
    rgb_color   ThemePanelBg() const;
    rgb_color   ThemeTextColor() const;
    rgb_color   ThemeDimTextColor() const;
    rgb_color   ThemeOutlineColor() const;

private:
                            CoffeeSettings();

    void                    SyncAllWindows();

    bool        fCelsius;
    bool        fMetric;    // true = mm, false = inches
    int32       fRatio;     // 15–18
    int32       fTheme;     // 0=System, 1=Light, 2=Dark
    char        fLanguage[4]; // "en", "es", "fr", "de", "ja"

    static CoffeeSettings*  sInstance;
};
