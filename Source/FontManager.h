/*
  ==============================================================================

    FontManager.h
    Created: 16 Jul 2025 11:21:13am
    Author:  roeim

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/

enum Weight {
    regular = 400,
    bold = 600
};

struct FontManager {
    static std::tuple<const void *, int> variableFont(Weight weight) {
        switch (weight) {
            case regular:
                return { BinaryData::Inter_18ptRegular_ttf, BinaryData::Inter_18ptRegular_ttfSize };
            case bold:
                return { BinaryData::Inter_18ptBold_ttf, BinaryData::Inter_18ptBold_ttfSize};
            default:
                jassertfalse; // Will break in debug builds
                return { nullptr, 0 };
        }

    }

    static juce::Font inter(float size, Weight weight);
};