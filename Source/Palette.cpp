/*
  ==============================================================================

	Palette::cpp
	Created: 16 Jul 2025 12:54:47pm
	Author:  roeim

  ==============================================================================
*/

#include "Palette.h"
#include <JuceHeader.h>

using Colour = juce::Colour;


// General
const Colour Palette::TextColour = Colour(0xFFFEFFFF);

// Knob
const Colour Palette::KnobBGGradientTop = Colour(0xFF191A24);
const Colour Palette::KnobBGGradientBottom = Colour(0xFF222534);
const Colour Palette::KnobBorder = Colour(0xFF18181F);
const Colour Palette::KnobInlineShadow1 = Colour(0xC535374C);
const Colour Palette::KnobInlineShadow2 = Colour(0x3C35374C);
const Colour Palette::KnobInlineShadow3 = Colour(0x0035374C);
/*PI = Position Indicator*/
const Colour Palette::KnobPIGradientTop = Colour(0x61BDBDC6);
/*PI = Position Indicator*/
const Colour Palette::KnobPIGradientBottom = Colour(0xFFBDBDC6);
const Colour Palette::KnobRange = Colour(0xFF22222F);
const Colour Palette::KnobRangeApplied = Colour(0xF0FEFFFF);


// Control Container
const Colour Palette::ControlsContainer = Colour(0xFF1E1E2C);

// Response Curve
const Colour Palette::BrightGrillLine = Colour::fromRGBA(255u, 255u, 255u, 50u);
const Colour Palette::DarkGrillLine = Colour::fromRGBA(255u, 255u, 255u, 25u);
// FFT Curve
const Colour Palette::FFTBodyGradientTop = Colour(0x20B2B2BE);
const Colour Palette::FFTBodyGradientBottom = Colour(0x0EB2B2BE);
const Colour Palette::FFTOutlineGradientTop = Palette::TextColour;
const Colour Palette::FFTOutlineGradientBottom = Colour(0x14ADADB9);