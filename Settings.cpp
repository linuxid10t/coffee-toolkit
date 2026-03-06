/*  Settings.cpp – Coffee Toolkit */

#include "Settings.h"
#include "Constants.h"

#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Window.h>
#include <string.h>

static const char* kSettingsFile = "CoffeeToolkit_settings";

CoffeeSettings* CoffeeSettings::sInstance = nullptr;

CoffeeSettings::CoffeeSettings()
    : fCelsius(true),
      fMetric(true),
      fRatio(15),
      fTheme(0)
{
    strlcpy(fLanguage, "en", sizeof(fLanguage));
    Load();
}

CoffeeSettings*
CoffeeSettings::Get()
{
    if (!sInstance)
        sInstance = new CoffeeSettings();
    return sInstance;
}

void
CoffeeSettings::Load()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
        return;
    path.Append(kSettingsFile);

    BFile file(path.Path(), B_READ_ONLY);
    if (file.InitCheck() != B_OK)
        return;

    BMessage archive;
    if (archive.Unflatten(&file) != B_OK)
        return;

    bool celsius;
    if (archive.FindBool("celsius", &celsius) == B_OK)
        fCelsius = celsius;

    bool metric;
    if (archive.FindBool("metric", &metric) == B_OK)
        fMetric = metric;

    int32 ratio;
    if (archive.FindInt32("ratio", &ratio) == B_OK
        && ratio >= 15 && ratio <= 18)
        fRatio = ratio;

    int32 theme;
    if (archive.FindInt32("theme", &theme) == B_OK
        && theme >= 0 && theme <= 2)
        fTheme = theme;

    const char* lang;
    if (archive.FindString("language", &lang) == B_OK)
        strlcpy(fLanguage, lang, sizeof(fLanguage));
}

void
CoffeeSettings::Save()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
        return;
    path.Append(kSettingsFile);

    BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
    if (file.InitCheck() != B_OK)
        return;

    BMessage archive;
    archive.AddBool("celsius", fCelsius);
    archive.AddBool("metric", fMetric);
    archive.AddInt32("ratio", fRatio);
    archive.AddInt32("theme", fTheme);
    archive.AddString("language", fLanguage);
    archive.Flatten(&file);
}

/* static */ void
CoffeeSettings::BuildSettingsMenu(BMenuBar* menuBar)
{
    CoffeeSettings* s = Get();
    BMenu* settingsMenu = new BMenu("Settings");

    // Temperature submenu
    BMenu* tempMenu = new BMenu("Temperature");
    tempMenu->SetRadioMode(true);
    BMenuItem* celsius = new BMenuItem("Celsius",
        new BMessage(MSG_SET_TEMP_CELSIUS));
    BMenuItem* fahrenheit = new BMenuItem("Fahrenheit",
        new BMessage(MSG_SET_TEMP_FAHRENHEIT));
    celsius->SetMarked(s->fCelsius);
    fahrenheit->SetMarked(!s->fCelsius);
    tempMenu->AddItem(celsius);
    tempMenu->AddItem(fahrenheit);
    settingsMenu->AddItem(tempMenu);

    // Default Brew Ratio submenu
    BMenu* ratioMenu = new BMenu("Default Brew Ratio");
    ratioMenu->SetRadioMode(true);
    struct { const char* label; uint32 msg; int32 val; } ratios[] = {
        { "1:15", MSG_SET_RATIO_15, 15 },
        { "1:16", MSG_SET_RATIO_16, 16 },
        { "1:17", MSG_SET_RATIO_17, 17 },
        { "1:18", MSG_SET_RATIO_18, 18 },
    };
    for (auto& r : ratios) {
        BMenuItem* item = new BMenuItem(r.label, new BMessage(r.msg));
        item->SetMarked(s->fRatio == r.val);
        ratioMenu->AddItem(item);
    }
    settingsMenu->AddItem(ratioMenu);

    // Theme submenu
    BMenu* themeMenu = new BMenu("Theme");
    themeMenu->SetRadioMode(true);
    struct { const char* label; uint32 msg; int32 val; } themes[] = {
        { "System default", MSG_SET_THEME_SYSTEM, 0 },
        { "Light",          MSG_SET_THEME_LIGHT,  1 },
        { "Dark",           MSG_SET_THEME_DARK,   2 },
    };
    for (auto& t : themes) {
        BMenuItem* item = new BMenuItem(t.label, new BMessage(t.msg));
        item->SetMarked(s->fTheme == t.val);
        themeMenu->AddItem(item);
    }
    settingsMenu->AddItem(themeMenu);

    // Language submenu
    BMenu* langMenu = new BMenu("Language");
    langMenu->SetRadioMode(true);
    struct { const char* label; uint32 msg; const char* code; } langs[] = {
        { "English",          MSG_SET_LANG_EN, "en" },
        { "Espa\xc3\xb1ol",  MSG_SET_LANG_ES, "es" },
        { "Fran\xc3\xa7" "ais", MSG_SET_LANG_FR, "fr" },
        { "Deutsch",          MSG_SET_LANG_DE, "de" },
        { "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", MSG_SET_LANG_JA, "ja" },
    };
    for (auto& l : langs) {
        BMenuItem* item = new BMenuItem(l.label, new BMessage(l.msg));
        item->SetMarked(strcmp(s->fLanguage, l.code) == 0);
        langMenu->AddItem(item);
    }
    settingsMenu->AddItem(langMenu);

    // Measurement Units submenu
    BMenu* unitsMenu = new BMenu("Measurement Units");
    unitsMenu->SetRadioMode(true);
    BMenuItem* mm      = new BMenuItem("Millimetres (mm)",
        new BMessage(MSG_SET_UNIT_MM));
    BMenuItem* inches  = new BMenuItem("Inches (in)",
        new BMessage(MSG_SET_UNIT_INCHES));
    mm->SetMarked(s->fMetric);
    inches->SetMarked(!s->fMetric);
    unitsMenu->AddItem(mm);
    unitsMenu->AddItem(inches);
    settingsMenu->AddItem(unitsMenu);

    menuBar->AddItem(settingsMenu, 0);
}

bool
CoffeeSettings::HandleSettingsMessage(BMessage* msg)
{
    switch (msg->what) {
        case B_ABOUT_REQUESTED:
            be_app->PostMessage(B_ABOUT_REQUESTED);
            return true;
        case MSG_SET_UNIT_MM:           fMetric = true;  break;
        case MSG_SET_UNIT_INCHES:       fMetric = false; break;
        case MSG_SET_TEMP_CELSIUS:      fCelsius = true;  break;
        case MSG_SET_TEMP_FAHRENHEIT:   fCelsius = false; break;
        case MSG_SET_RATIO_15:          fRatio = 15; break;
        case MSG_SET_RATIO_16:          fRatio = 16; break;
        case MSG_SET_RATIO_17:          fRatio = 17; break;
        case MSG_SET_RATIO_18:          fRatio = 18; break;
        case MSG_SET_THEME_SYSTEM:      fTheme = 0; break;
        case MSG_SET_THEME_LIGHT:       fTheme = 1; break;
        case MSG_SET_THEME_DARK:        fTheme = 2; break;
        case MSG_SET_LANG_EN:           strlcpy(fLanguage, "en", sizeof(fLanguage)); break;
        case MSG_SET_LANG_ES:           strlcpy(fLanguage, "es", sizeof(fLanguage)); break;
        case MSG_SET_LANG_FR:           strlcpy(fLanguage, "fr", sizeof(fLanguage)); break;
        case MSG_SET_LANG_DE:           strlcpy(fLanguage, "de", sizeof(fLanguage)); break;
        case MSG_SET_LANG_JA:           strlcpy(fLanguage, "ja", sizeof(fLanguage)); break;
        default:
            return false;
    }
    Save();
    SyncAllWindows();
    return true;
}

void
CoffeeSettings::SyncAllWindows()
{
    for (int32 i = 0; i < be_app->CountWindows(); i++) {
        BWindow* win = be_app->WindowAt(i);
        if (!win->Lock())
            continue;

        BMenuBar* bar = dynamic_cast<BMenuBar*>(win->FindView("menubar"));
        if (bar) {
            BMenu* settingsMenu = bar->SubmenuAt(0);
            if (settingsMenu
                && strcmp(settingsMenu->Name(), "Settings") == 0) {
                // Temperature (submenu index 0)
                BMenu* tempMenu = settingsMenu->SubmenuAt(0);
                if (tempMenu) {
                    tempMenu->ItemAt(0)->SetMarked(fCelsius);
                    tempMenu->ItemAt(1)->SetMarked(!fCelsius);
                }
                // Ratio (submenu index 1)
                BMenu* ratioMenu = settingsMenu->SubmenuAt(1);
                if (ratioMenu) {
                    for (int32 r = 0; r < ratioMenu->CountItems(); r++)
                        ratioMenu->ItemAt(r)->SetMarked(fRatio == 15 + r);
                }
                // Theme (submenu index 2)
                BMenu* themeMenu = settingsMenu->SubmenuAt(2);
                if (themeMenu) {
                    for (int32 t = 0; t < themeMenu->CountItems(); t++)
                        themeMenu->ItemAt(t)->SetMarked(fTheme == t);
                }
                // Language (submenu index 3)
                BMenu* langMenu = settingsMenu->SubmenuAt(3);
                if (langMenu) {
                    const char* codes[] = { "en", "es", "fr", "de", "ja" };
                    for (int32 l = 0; l < langMenu->CountItems(); l++)
                        langMenu->ItemAt(l)->SetMarked(
                            strcmp(fLanguage, codes[l]) == 0);
                }
                // Units (submenu index 4)
                BMenu* unitsMenu = settingsMenu->SubmenuAt(4);
                if (unitsMenu) {
                    unitsMenu->ItemAt(0)->SetMarked(fMetric);
                    unitsMenu->ItemAt(1)->SetMarked(!fMetric);
                }
            }
        }
        win->Unlock();
    }
}
