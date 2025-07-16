/*
  ==============================================================================

	Palette::h
	Created: 16 Jul 2025 12:54:47pm
	Author:  roeim

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct Palette {
	using Colour = juce::Colour;

	// General
	static const Colour TextColour;

	// Knob
	static const Colour KnobBGGradientTop;
	static const Colour KnobBGGradientBottom;
	static const Colour KnobBorder;
	static const Colour KnobInlineShadow1;
	static const Colour KnobInlineShadow2;
	static const Colour KnobInlineShadow3;
	/*PI = Position Indicator*/
	static const Colour KnobPIGradientTop;
	/*PI = Position Indicator*/
	static const Colour KnobPIGradientBottom;
	static const Colour KnobRange;
	static const Colour KnobRangeApplied;


	// Control Container
	static const Colour ControlsContainer;

	// Response Curve
	static const Colour BrightGrillLine;
	static const Colour DarkGrillLine;

	~Palette() { DBG("Palette destroyed"); }

};