#pragma once
/*  Constants.h – Coffee Toolkit
 *
 *  All shared message constants and layout values.
 *  Include this everywhere instead of redeclaring.
 */

#include <SupportDefs.h>

// -------------------------------------------------------
// Top-level window commands (MainWindow → child windows)
// -------------------------------------------------------
const uint32 MSG_BREW_RATIO  = 'BRRC';
const uint32 MSG_PARTICLE    = 'PCLR';
const uint32 MSG_EXTRACTION  = 'EXTR';
const uint32 MSG_ROAST_COLOR = 'RCLR';

// -------------------------------------------------------
// BrewRatioWindow internals
// -------------------------------------------------------
const uint32 MSG_CALCULATE    = 'CALC';
const uint32 MSG_RATIO_PICKED = 'RPCK';

// -------------------------------------------------------
// ExtractionWindow internals
// -------------------------------------------------------
const uint32 MSG_EXT_CALC  = 'EXCL';
const uint32 MSG_USE_TDS   = 'UTDS';
const uint32 MSG_USE_BRIX  = 'UBRX';
const uint32 MSG_BREW_PERC = 'BPRC';
const uint32 MSG_BREW_IMMS = 'BIMM';

// -------------------------------------------------------
// RoastColorWindow internals
// -------------------------------------------------------
const uint32 MSG_ROAST_OPEN        = 'ROPN';
const uint32 MSG_ROAST_REFS        = 'RREF';
const uint32 MSG_SELECTION_CHANGED = 'RSEL';
const uint32 MSG_CLEAR_SELECTION   = 'RCLS';

// -------------------------------------------------------
// Settings menu messages
// -------------------------------------------------------
const uint32 MSG_SET_TEMP_CELSIUS     = 'STC ';
const uint32 MSG_SET_TEMP_FAHRENHEIT  = 'STF ';
const uint32 MSG_SET_RATIO_15         = 'SR15';
const uint32 MSG_SET_RATIO_16         = 'SR16';
const uint32 MSG_SET_RATIO_17         = 'SR17';
const uint32 MSG_SET_RATIO_18         = 'SR18';
const uint32 MSG_SET_THEME_SYSTEM     = 'STS ';
const uint32 MSG_SET_THEME_LIGHT      = 'STL ';
const uint32 MSG_SET_THEME_DARK       = 'STD ';
const uint32 MSG_SET_LANG_EN          = 'SLEN';
const uint32 MSG_SET_LANG_ES          = 'SLES';
const uint32 MSG_SET_LANG_FR          = 'SLFR';
const uint32 MSG_SET_LANG_DE          = 'SLDE';
const uint32 MSG_SET_LANG_JA          = 'SLJA';

// -------------------------------------------------------
// Layout constants (shared across all windows)
// -------------------------------------------------------
static const int kBtnW = 140;   // main-window button width
static const int kBtnH = 120;   // main-window button height
static const int kPad  =  10;   // standard padding
static const int kLblW = 110;   // fixed width for input-row labels
