# Coffee Toolkit

A native Haiku GUI application for coffee enthusiasts.  Provides a suite
of tools for brewing, extraction analysis, and roast evaluation.

## Features

| Tool | Description |
|---|---|
| Brew Ratio Calculator | Calculate coffee dose from water volume and a 1:X ratio (presets or custom) |
| Extraction Calculator | Compute extraction yield from TDS or Brix, with a colour-coded gauge and brew tips |
| Roast Color Analyzer | Estimate Agtron roast score from a photograph, with drag-to-select sampling region |
| Particle Analyzer | Grind particle size distribution — photo estimate, sieve cascade, or calibrated sheet modes |

### Settings

All windows share a persistent settings menu with:

- **Temperature** — Celsius / Fahrenheit
- **Default Brew Ratio** — 1:15 through 1:18
- **Theme** — System default / Light / Dark (applied live to all open windows)
- **Language** — English, Español, Français, Deutsch, 日本語
- **Measurement Units** — Millimetres / Inches

## Screenshots

_(add screenshots here once the app is running)_

## Building

### Requirements

- Haiku OS (x86_64)
- GCC 13 or later (included with Haiku)
- Haiku development libraries (included with Haiku)

### Build

```bash
git clone https://github.com/YOUR_USERNAME/coffee-toolkit.git
cd coffee-toolkit
make
```

The compiled binary will be named `coffee_toolkit` in the project directory.

### Clean

```bash
make clean
```

### Manual build (without make)

```bash
g++ -std=c++17 -o coffee_toolkit \
    main.cpp MainWindow.cpp BrewRatioWindow.cpp \
    ExtractionWindow.cpp RoastColorWindow.cpp ParticleWindow.cpp Settings.cpp \
    -lbe -lroot -ltranslation -ltracker -lshared
g++ -std=c++17 -o set_icon set_icon.cpp -lbe -lroot -ltranslation
xres -o coffee_toolkit coffee_toolkit.rsrc
mimeset -f coffee_toolkit
./set_icon coffee_toolkit toolkit.png
```

## Project Structure

```
coffee-toolkit/
├── Constants.h             # Shared message constants and layout values
├── main.cpp                # Application entry point (CoffeeToolkitApp)
├── MainWindow.h/.cpp       # Four-button launcher window
├── BrewRatioWindow.h/.cpp  # Brew ratio calculator
├── ExtractionWindow.h/.cpp # Extraction calculator + gauge view
├── RoastColorWindow.h/.cpp # Roast color analyzer + gauge + thumbnail view
├── ParticleWindow.h/.cpp   # Particle analyzer (photo / sieve / calibrated modes)
├── Settings.h/.cpp         # Persistent settings singleton + cross-window sync
├── toolkit.png             # Application icon source (1024×1024 RGBA)
├── set_icon.cpp            # Build helper: scales PNG and writes icon attributes
├── coffee_toolkit.rdef     # Resource definitions (app signature, version)
└── Makefile
```

## How the Roast Color Analyzer Works

1. Load a photograph of ground coffee (ideally on a neutral grey card,
   evenly lit, filling the frame)
2. Optionally drag a selection rectangle on the preview to sample a
   specific region — otherwise the central 50% of the image is used
3. Each pixel in the sample region is linearised from sRGB and converted
   to CIE luminance: `L = 0.2126R + 0.7152G + 0.0722B`
4. Average luminance is mapped linearly to the Agtron scale (25–95)
5. The result is classified into a SCAA roast level band with brew tips

> **Note:** This is a photographic approximation. Professional Agtron
> measurement uses near-infrared reflectance on a calibrated instrument.
> Results are most accurate when images are taken under consistent,
> diffuse lighting on a neutral background.

## How the Extraction Calculator Works

Supports two brew methods:

- **Percolation** (pour-over, espresso, moka pot):
  `EY% = (TDS% × brew_weight_g) / dose_g`
- **Immersion** (French press, AeroPress, cold brew):
  `EY% = (TDS% × water_g) / dose_g`

TDS can be entered directly or converted from Brix (`TDS = Brix × 0.85`).

Ideal extraction yield ranges (SCAA):
- Percolation: 18–22%
- Immersion: 18–24%

## How the Brew Ratio Calculator Works

The brew ratio (coffee:water) determines strength and extraction. This calculator
computes the required coffee dose from your water volume and chosen ratio:

```
coffee (g) = water (ml) ÷ ratio
```

**Presets:**
- **1:12** — Strong, concentrated (espresso-like intensity)
- **1:15** — Standard, balanced (most pour-over recipes)
- **1:16** — Light, tea-like clarity
- **1:17** — Very light, subtle

**Custom:** Enter any ratio (e.g., 1:13.5) for specific recipes.

Example: 250 ml water at 1:15 → 250 ÷ 15 = **16.7 g coffee**

> **Tip:** Weigh both water and coffee for consistency. Volume measurements
> (tablespoons, scoops) vary too much for repeatable results.

## How the Particle Analyzer Works

Three modes, selectable via radio buttons:

**Photo Estimate:** Load a photograph of coffee grounds. The selected region
(or centre 50%) is divided into 8×8 px cells; average per-cell luminance
variance is mapped to a grind band (Extra Fine <250 µm → Extra Coarse
>1200 µm). Result shown on a gradient gauge with contextual brew tips.

**Sieve Cascade:** Physically sieve grounds and photograph each fraction on a
white background. The app thresholds each image (luminance < 0.7 = particle)
to compute relative dark-pixel area, accumulates entries for user-selected
sieve sizes (212–1700 µm), and renders a labelled bar-chart with a D50
estimate.

**Calibrated Sheet:** Photograph grounds on paper alongside a known scale
reference and enter the px/mm calibration. The app thresholds the image and
flood-fills connected components to measure each particle blob; effective
diameter = 2·√(area/π). Results shown as a histogram with D50, D90, and
particle count.

## Theme Support

The Theme setting (Settings → Theme) applies live to all open windows.
Custom-drawn views — gauge bars, distribution charts, and BTextView tips
areas — switch between light and dark palettes immediately. Standard
Haiku controls follow the system appearance as usual.

## Author

David Masson

## License

MIT License — Copyright (c) 2026 David Masson. See [LICENSE](LICENSE) for details.
