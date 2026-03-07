# Coffee Toolkit — Project Brief

**Author:** David Masson
**App signature:** `application/x-vnd.DavidMasson.CoffeeToolkit`

A native Haiku OS GUI application written in C++ using the Haiku API
(BWindow, BView, BLayoutBuilder, etc.). The app has a main window with
four large buttons, each opening a tool window.

## Completed Tools

### Brew Ratio Calculator
Dropdown of preset ratios (1:12, 1:15, 1:16, 1:17) plus a custom 1:X
input. Calculates coffee dose in grams from water volume in ml using:

    coffee = water / ratio

### Extraction Calculator
Takes TDS% or Brix input (radio toggle, Brix converted via `× 0.85`),
percolation or immersion brew type (radio toggle), brew weight and dose
inputs. Calculates extraction yield and displays it on a colour-coded
horizontal gauge (blue = under, green = ideal, red = over). Ideal ranges
are 18–22% for percolation, 18–24% for immersion. Shows contextual tips
based on result.

### Roast Color Analyzer
Loads a JPEG/PNG image via BFilePanel and displays a 320×180 interactive
thumbnail. The user can drag a selection rectangle on the thumbnail to
define a custom sampling region; otherwise the central 50% is used.
Pixels in the region are linearised from sRGB and converted to CIE
luminance, averaged, then mapped linearly to the Agtron scale (25–95).
Result is displayed on a dark-to-light gradient gauge with roast band
labels. The result label shows whether a custom selection or the centre
region was used. Analysis runs automatically on image load and on
selection change. A "Clear Selection" button resets back to centre crop.

Agtron roast bands (SCAA standard):
- 95–75: Very Light (Cinnamon / White Roast)
- 75–65: Light (City)
- 65–55: Medium (City+)
- 55–45: Medium-Dark (Full City / Full City+)
- 45–35: Dark (Vienna / French)
- 35–25: Very Dark (Italian / Spanish)

### Particle Analyzer
Three analysis modes selectable via radio buttons at the top of the window:

**Mode 1 — Photo Estimate:** Load a photograph of coffee grounds. The
selected region (or centre 50%) is divided into 8×8 px cells; the average
per-cell luminance variance (local contrast map) is mapped to a Fine→Coarse
grind band (Extra Fine <250 µm through Extra Coarse >1200 µm). Result shown
on a custom gradient gauge with a pointer and contextual brew tips.

**Mode 2 — Sieve Cascade:** The user physically sieves grounds and
photographs each sieve fraction on a white background. The app thresholds
each image (luminance < 0.7 = particle) to compute the relative dark-pixel
area, accumulates entries for user-selected sieve sizes (212–1700 µm), and
renders a labelled bar-chart distribution with a D50 estimate.

**Mode 3 — Calibrated Sheet:** The user photographs grounds on paper with
a known scale reference and enters the pixels-per-mm calibration. The app
thresholds the image and flood-fills connected components to measure each
particle blob; effective diameter = 2·√(area/π). Results shown as a
particle-diameter histogram with D50, D90, and particle count.

## Settings System

Persistent settings are managed by the `CoffeeSettings` singleton
(`Settings.h/.cpp`). On first run, the language is auto-detected from
the OS locale via `BLocaleRoster`, falling back to English if the system
language is not among the supported set (en, es, fr, de, ja). Saved
settings always take precedence once a settings file exists.

Settings are synchronised across all open windows via `SyncAllWindows()`,
which walks `be_app->WindowAt()` and updates every Settings submenu in
place, then broadcasts `MSG_THEME_CHANGED` to every window so custom
views and BTextView tips areas repaint with the new theme colours.

### Theme System

The Theme setting (System default / Light / Dark) is stored as an integer
(0 / 1 / 2) and applied to all custom-drawn content via four accessor
methods on `CoffeeSettings`:

| Accessor | Light | Dark | System |
|---|---|---|---|
| `ThemePanelBg()` | (245, 245, 242) | (38, 38, 42) | `ui_color(B_PANEL_BACKGROUND_COLOR)` |
| `ThemeTextColor()` | (20, 20, 20) | (210, 210, 210) | `ui_color(B_PANEL_TEXT_COLOR)` |
| `ThemeDimTextColor()` | (90, 90, 90) | (150, 150, 150) | (120, 120, 120) |
| `ThemeOutlineColor()` | (100, 100, 100) | (80, 80, 80) | (70, 70, 70) |

Each custom `Draw()` method fetches these values at the top and uses them
for background fills, outlines, tick marks, and label colours. Standard
Haiku controls (BButton, BRadioButton, BTextControl, BMenuBar) are drawn
by the app_server and are unaffected. The coffee-colour gradients in the
gauge bars (roast scale, grind size) are data representations and are
intentionally unchanged across themes.

When `MSG_THEME_CHANGED` is received, each tool window updates the
`SetViewColor` and text colour of its `BTextView` tips area (via
`SetFontAndColor` over the full text range) and calls `Invalidate()` on
each custom gauge/chart view. Newly opened windows pick up the active
theme in their constructors.

## Key UI Decisions Made During Development

- Fixed-width `BStringView` labels (110px) paired with no-label
  `BTextControl` fields, to avoid label truncation bugs on toggle
- Radio buttons wrapped in separate `BGroupView` containers to prevent
  cross-group mutual-exclusion interference between unrelated pairs
- Bold status label in ExtractionWindow showing the current mode
  combination (e.g. "Mode: TDS | Percolation (ideal 18-22%)")
- Analyse button removed from RoastColorWindow in favour of fully
  automatic reactive analysis (fires on load and on selection change)
- Thumbnails are 320×180 — large enough for drag selection without
  pushing the window height above 600 px on smaller screens
- All tool windows optimized to fit within 600 px total height
  (content + Haiku title bar + borders) by reducing tips scroll areas
  and removing unnecessary height buffers from ResizeTo() calls

## Build

```bash
make
```

Or manually:

```bash
g++ -std=c++17 -o coffee_toolkit \
    main.cpp MainWindow.cpp BrewRatioWindow.cpp \
    ExtractionWindow.cpp RoastColorWindow.cpp ParticleWindow.cpp Settings.cpp \
    -lbe -lroot -ltranslation -ltracker -lshared
g++ -std=c++17 -o set_icon set_icon.cpp -lbe -lroot -ltranslation
rc -o coffee_toolkit.rsrc coffee_toolkit.rdef
xres -o coffee_toolkit coffee_toolkit.rsrc
mimeset -f coffee_toolkit
./set_icon coffee_toolkit toolkit.png
```

The `set_icon` step compiles and runs a small Haiku-native helper that scales
`toolkit.png` to 32×32 and 16×16, converts to the BeOS CMAP8 palette via
`BScreen::IndexForColor`, and writes `BEOS:APP_ICON` / `BEOS:MINI_ICON`
attributes on the binary so Tracker displays the icon.

## File Structure

```
Constants.h              Shared MSG_* constants and layout values
main.cpp                 CoffeeToolkitApp + main()
MainWindow.h/.cpp        Four-button launcher window
BrewRatioWindow.h/.cpp   Brew ratio calculator
ExtractionWindow.h/.cpp  Extraction calculator + ExtractionBarView gauge
RoastColorWindow.h/.cpp  Roast analyzer + RoastGaugeView + ThumbView
ParticleWindow.h/.cpp    Particle analyzer (Photo / Sieve / Calibrated modes)
Settings.h/.cpp          Persistent settings singleton + cross-window sync
toolkit.png              Application icon source (1024×1024 RGBA)
set_icon.cpp             Build helper: scales PNG → CMAP8 icon attributes
coffee_toolkit.rdef      Resource definitions (app signature, version)
Makefile
```
