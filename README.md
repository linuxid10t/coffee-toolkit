# Coffee Toolkit

A native Haiku GUI application for coffee enthusiasts.  Provides a suite
of tools for brewing, extraction analysis, and roast evaluation.

## Features

| Tool | Status | Description |
|---|---|---|
| Brew Ratio Calculator | ✅ Complete | Calculate coffee dose from water volume and a 1:X ratio (presets or custom) |
| Extraction Calculator | ✅ Complete | Compute extraction yield from TDS or Brix, with a colour-coded gauge and brew tips |
| Roast Color Analyzer | ✅ Complete | Estimate Agtron roast score from a photograph, with drag-to-select sampling region |
| Particle Analyzer | ✅ Complete | Grind particle size distribution — photo estimate, sieve cascade, or calibrated sheet modes |

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
    ExtractionWindow.cpp RoastColorWindow.cpp DetailWindow.cpp Settings.cpp \
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
├── DetailWindow.h/.cpp     # Particle analyzer (photo / sieve / calibrated modes)
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

## Author

David Masson

## License

MIT License — Copyright (c) 2026 David Masson. See [LICENSE](LICENSE) for details.
