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
// Layout constants (shared across all windows)
// -------------------------------------------------------
static const int kBtnW = 140;   // main-window button width
static const int kBtnH = 120;   // main-window button height
static const int kPad  =  10;   // standard padding
static const int kLblW = 110;   // fixed width for input-row labels
