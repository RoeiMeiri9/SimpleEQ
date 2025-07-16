/*
  ==============================================================================

    FontManager.cpp
    Created: 16 Jul 2025 12:30:12pm
    Author:  roeim

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FontManager.h"

juce::Font FontManager::inter(float size, Weight weight) {
    auto [fontData, fontSize] = variableFont(weight);

    // COPY the data to a heap-allocated buffer
    std::unique_ptr<char[]> fontCopy(new char[fontSize]);
    std::memcpy(fontCopy.get(), fontData, fontSize);

    auto typeface = juce::Typeface::createSystemTypefaceFor(fontCopy.get(), fontSize);
    fontCopy.release(); // JUCE will now own this memory

    juce::Font font(typeface);
    font.setHeight(size);
    return font;
}