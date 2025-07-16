/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Palette.h"

void LookAndFeel::drawRotarySlider(
	juce::Graphics &g,
	int x,
	int y,
	int width,
	int height,
	float sliderPosProportional,
	float rotaryStartAngle,
	float rotaryEndAngle,
	juce::Slider &slider
) {
	using namespace juce;

	auto bounds = Rectangle<float>(x, y, width, height);

	ColourGradient KnobBackground(
		Palette::KnobBGGradientTop,
		0.0f, 0.0f,
		Palette::KnobBGGradientBottom,
		0.0f, bounds.getHeight(),
		false
	);

	g.setGradientFill(KnobBackground);
	g.fillEllipse(bounds);

	auto center = bounds.getCentre();

	float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

	juce::ColourGradient innerGlow(
		Palette::KnobInlineShadow3,
		center.x, center.y,
		Palette::KnobInlineShadow1,
		center.x - radius * 0.65f, center.y - radius,
		true // radial
	);

	// Stop at 10px above center
	float pixelOffset = radius * 0.6f;
	float relativeStop = pixelOffset / radius;
	innerGlow.addColour(relativeStop, Palette::KnobInlineShadow2);

	pixelOffset = radius * 0.3f;
	relativeStop = pixelOffset / radius;
	innerGlow.addColour(relativeStop, Palette::KnobInlineShadow3);

	g.setGradientFill(innerGlow);
	g.fillEllipse(bounds);

	g.setColour(Palette::KnobBorder);
	g.drawEllipse(bounds, 1.f);

	if (auto *rswl = dynamic_cast<RotarySliderWithLabels *>(&slider)) {
		auto center = bounds.getCentre();
		auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

		auto radius = bounds.getWidth() * 0.5f - 4.0f;

		// Build vertical indicator bar (4px wide, height based on text)
		Rectangle<float> r;
		r.setSize(3.0f, 14.0f);

		// Calculate point on circumference for bottom center of the bar
		auto circumferencePoint = juce::Point<float>(
			center.x + radius * std::cos(sliderAngRad - juce::MathConstants<float>::halfPi),
			center.y + radius * std::sin(sliderAngRad - juce::MathConstants<float>::halfPi)
		);

		// Set bar center so that bottom center is at circumferencePoint
		r.setCentre(circumferencePoint.x, circumferencePoint.y + r.getHeight() * 0.5f);

		Path p;
		p.addRoundedRectangle(r, 2.0f);

		// Optionally rotate the bar itself by sliderAngRad if you want it tilted
		// If you want vertical bars along the circumference, comment this line out
		p.applyTransform(AffineTransform::rotation(sliderAngRad, circumferencePoint.x, circumferencePoint.y));

		auto bottomCenter = juce::Point<float>(r.getX() + r.getWidth() * 0.5f, r.getBottom());
		auto topCenter = juce::Point<float>(r.getX() + r.getWidth() * 0.5f, r.getY());

		auto start = rotatePointAround(bottomCenter, circumferencePoint, sliderAngRad);
		auto end = rotatePointAround(topCenter, circumferencePoint, sliderAngRad);

		juce::ColourGradient PositionIdentifierGradient(
			Palette::KnobPIGradientTop,
			start.toFloat(),
			Palette::KnobPIGradientBottom,
			end.toFloat(),
			false
		);

		g.setGradientFill(PositionIdentifierGradient);
		g.fillPath(p);

		//g.setFont(rswl->getTextHeight()); //TODO: USE THE FF FONT IN THE FUTURE! 
		//auto text = rswl->getDisplayString();
		//auto strWidth = g.getCurrentFont().getStringWidth(text);

		//r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
		//r.setCentre(center);

		//g.setColour(Colour::fromRGB(10u, 6u, 14u));
		//g.fillRect(r);

		//g.setColour(Colours::white);
		//g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
	}
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g) {
	using namespace juce;

	auto startAng = degreesToRadians(180.f + 45.f);
	auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

	auto range = getRange();

	auto sliderBounds = getSliderBounds();

	//g.setColour(Colours::red);
	//g.drawRect(getLocalBounds());
	//g.setColour(Colours::yellow);
	//g.drawRect(sliderBounds);

	getLookAndFeel().drawRotarySlider(
		g,
		sliderBounds.getX(),
		sliderBounds.getY(),
		sliderBounds.getWidth(),
		sliderBounds.getHeight(),
		jmap(
			getValue(),
			range.getStart(),
			range.getEnd(),
			0.0,
			1.0),
		startAng,
		endAng,
		*this
	);

	auto center = sliderBounds.toFloat().getCentre();
	auto radius = sliderBounds.getWidth() * 0.5f;

	g.setColour(Palette::TextColour);
	static juce::Font font(FontManager::inter(getTextHeight(), regular));
	g.setFont(font);

	int numChoices = labels.size();
	for (int i = 0; i < numChoices; ++i) {
		auto pos = labels[i].pos;
		jassert(0.f <= pos);
		jassert(pos <= 1.f);

		auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

		// c is for the center of the text that will be displaying the minimum value possible for this knob.
		auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1.f, ang);

		Rectangle<float> r;
		auto &str = labels[i].label;
		r.setSize(font.getStringWidth(str), getTextHeight());
		r.setCentre(c);
		r.setY(r.getY() + getTextHeight());

		g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
	}
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
	juce::Rectangle<int> bounds = getLocalBounds();

	auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

	size -= getTextHeight() * 2;

	juce::Rectangle<int> r;
	r.setSize(size, size);
	r.setCentre(bounds.getCentreX(), 0);
	r.setY(2);

	return r;

}

juce::String RotarySliderWithLabels::getDisplayString() const {
	if (auto *choiceParam = dynamic_cast<juce::AudioParameterChoice *>(param))
		return choiceParam->getCurrentChoiceName();

	juce::String str;
	bool addK = false;

	if (auto *floatParam = dynamic_cast<juce::AudioParameterFloat *>(param)) {
		float val = getValue();
		if (val >= 1000) {
			val /= 1000.f;
			addK = true;
		}

		str = juce::String(val, (addK ? 2 : 0));
	} else {
		jassertfalse;
	}
	if (suffix.isNotEmpty()) {
		str << " ";
		if (addK) {
			str << "k";
		}
		str << suffix;
	}

	return str;
}
//==============================================================================


void ControlsContainer::paint(juce::Graphics &g) {
	auto bounds = getLocalBounds().toFloat();
	g.setColour(Palette::ControlsContainer);
	g.fillRoundedRectangle(bounds, cornerRadius);
}

void ControlsContainer::resized() {
	
	const int knobCount = rswlList.size();
	if (knobCount == 0)
		return;

	static juce::Font font(FontManager::inter(28.f, regular));
	titleLabel.setFont(font);

	int labelHeight = static_cast<int>(font.getHeight()); // add small margin

	const int width = knobCount * knobWidth + (knobCount - 1) * knobGap + paddingRightLeft * 2;
	const int height = paddingTopBottom * 2 + knobHeight + frameGap + labelHeight; // 20 = titleLabel height

	setSize(width, height); // optional — only if you're letting the component resize itself

	auto bounds = getLocalBounds().reduced(paddingRightLeft, paddingTopBottom);

	int startX = bounds.getX();
	int y = bounds.getY();

	for (auto *knob : rswlList) {
		knob->setBounds(startX, y, knobWidth, knobHeight);
		startX += knobWidth + knobGap;
	}
	
	titleLabel.setBounds(bounds.withTop(y + knobHeight + frameGap).withHeight(labelHeight));
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor &p):
	audioProcessor(p),
	leftPathProducer(audioProcessor.leftChannelFifo),
	rightPathProducer(audioProcessor.rightChannelFifo)
{
	const auto &params = audioProcessor.getParameters();
	for (auto param : params) {
		param->addListener(this);
	}

	
	updateChain();

	startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
	const auto &params = audioProcessor.getParameters();
	for (auto param : params) {
		param->removeListener(this);
	}
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
	parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate) {
	juce::AudioBuffer<float> tempIncomingBuffer;


	while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0) {
		if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer)) {
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(
				monoBuffer.getWritePointer(0, 0),
				monoBuffer.getReadPointer(0, size),
				monoBuffer.getNumSamples() - size
			);

			juce::FloatVectorOperations::copy(
				monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
				tempIncomingBuffer.getReadPointer(0, 0),
				size
			);

			FFTDataGenerator.producerFFTDataForRendering(monoBuffer, -48.f);
		}
	}


	/*
	if there are FFT data buffers to pul
	if we can pull a buffer
	generate a path.
	*/
	const auto fftSize = FFTDataGenerator.getFFTSize();

	/*
	4800 / 2048 = 23hz <- this is the bin width
	*/
	const auto binWidth = sampleRate / static_cast<double>(fftSize);

	while (FFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
		std::vector<float> fftData;
		if (FFTDataGenerator.getFFTData(fftData)) {
			pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
		}
	}

	/*
	while there are paths that we can pull
	pull as many as we can
	display the most recent path
	*/

	while (pathProducer.getNumPathsAvailable()) {
		pathProducer.getPath(FFTPath);
	}
}

void ResponseCurveComponent::timerCallback() {

	auto fftBounds = getAnalysisArea().toFloat();
	auto sampleRate = audioProcessor.getSampleRate();

	leftPathProducer.process(fftBounds, sampleRate);
	rightPathProducer.process(fftBounds, sampleRate);

	if (parametersChanged.compareAndSetBool(false, true)) {
		// update the monochain
		updateChain();

		// signal a repaint
		//repaint();
	}

	repaint();
}

void ResponseCurveComponent::updateChain() {

	const double sampleRate = audioProcessor.getSampleRate();

	auto chainSettings = getChainSettings(audioProcessor.apvts);
	auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
	updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

	auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
	auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);

	updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics &g) {
	using namespace juce;

	g.drawImage(background, getLocalBounds().toFloat());

	auto responseArea = getAnalysisArea();

	auto w = responseArea.getWidth();

	auto &lowcut = monoChain.get<ChainPositions::LowCut>();
	auto &peak = monoChain.get<ChainPositions::Peak>();
	auto &highcut = monoChain.get<ChainPositions::HighCut>();

	auto sampleRate = audioProcessor.getSampleRate();

	std::vector <double> mags;

	mags.resize(w);
	for (int i = 0; i < w; ++i) {
		double mag = 1.f;
		auto freq = mapToLog10((static_cast<double>(i) / static_cast<double>(w)), 20.0, 20000.0);

		if (!monoChain.isBypassed<ChainPositions::Peak>())
			mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

		if (!lowcut.isBypassed<0>())
			mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowcut.isBypassed<1>())
			mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowcut.isBypassed<2>())
			mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowcut.isBypassed<3>())
			mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

		if (!highcut.isBypassed<0>())
			mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highcut.isBypassed<1>())
			mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highcut.isBypassed<2>())
			mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highcut.isBypassed<3>())
			mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

		mags[i] = Decibels::gainToDecibels(mag);
	}

	Path responseCurve;
	const double outputMin = responseArea.getBottom();
	const double outputMax = responseArea.getY();
	auto map = [outputMin, outputMax](double input) {
		return jmap(input, -24.0, 24.0, outputMin, outputMax);
		};

	responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	for (size_t i = 1; i < mags.size(); ++i) {
		responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
	}

	auto leftChannelFFTPath = leftPathProducer.getPath();
	auto rightChannelFFTPath = rightPathProducer.getPath();

	leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
	rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

	ColourGradient FFTPathGradient(
		Colour(0xFFFEFFFF),
		0.0f, 0.0f,
		Colour(0x08ADADB9),
		0.0f, static_cast<float>(getHeight()),
		false
	);

	FFTPathGradient.multiplyOpacity(0.25f);

	g.setGradientFill(FFTPathGradient);
	g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
	g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));

	ColourGradient ResponseCurveGradient(
		Colour(0xFFFEFFFF),
		0.0f, 0.0f,
		Colour(0x08ADADB9),
		0.0f, static_cast<float>(getHeight()),
		false
	);

	ResponseCurveGradient.addColour(0.6f, Colour(0xFFFEFFFF));

	g.setGradientFill(ResponseCurveGradient);
	g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized() {
	using namespace juce;
	background = Image(Image::PixelFormat::ARGB, getWidth(), getHeight(), true);

	Graphics g(background);

	const Array<float> freqs{
		20, 30, 40, 50, 60, 70, 80, 90,
		100, 200, 300, 400, 500, 600, 700, 800, 900,
		1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
		10000, 20000
	};

	auto renderArea = getAnalysisArea();
	auto width = renderArea.getWidth();
	auto left = renderArea.getX();
	auto right = renderArea.getRight();
	auto top = renderArea.getY();
	auto bottom = renderArea.getBottom();

	Array<float> xs;

	for (float f : freqs) {
		auto normX = mapFromLog10(f, 20.f, 20000.f);
		xs.add(left + width * normX);
	}

	std::size_t i = 0;

	for (float x : xs) {
		g.setColour(isPowerOfTen(freqs[i]) ? Palette::BrightGrillLine : Palette::DarkGrillLine);
		g.drawVerticalLine(x, top, bottom);
		++i;
	}

	const Array<float> gain{
		24, 12, 0, -12, -24
	};
	
	static juce::Font font(FontManager::inter(fontHeight, regular));
	g.setFont(font);

	for (float gDb : gain) {
		auto y = jmap(gDb, -24.f, 24.f, static_cast<float>(bottom), static_cast<float>(top));

		g.setColour(gDb == 0 ? Palette::BrightGrillLine : Palette::DarkGrillLine);
		g.drawHorizontalLine(y, left, right);

		String str;
		if (gDb > 0)
			str << "+";
		str << gDb;

		auto textWidth = font.getStringWidth(str);

		Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setX(getWidth() - textWidth - 5);
		r.setCentre(r.getCentreX(), y);

		g.setColour(Palette::TextColour);
		g.drawFittedText(str, r, juce::Justification::centred, 1);

		str.clear();
		str << (gDb - 24.f);
		r.setX(5);
		textWidth = g.getCurrentFont().getStringWidth(str);
		r.setSize(textWidth, fontHeight);
		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	for (int i = 0; i < freqs.size(); ++i) {
		auto f = freqs[i];
		if (!(isPowerOfTen(f) || isPowerOfTen(f / 2.f) || isPowerOfTen(f / 5.f))) {
			continue;
		}
		auto x = xs[i];
		bool addK = false;
		String str;
		if (f >= 1000) {
			addK = true;
			f /= 1000.f;
		}

		str << f;
		if (addK)
			str << "k";

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setCentre(x, 0);
		r.setY(bottom + 1);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}
}

bool ResponseCurveComponent::isPowerOfTen(float num) {
	if (num <= 0)
		return false;

	float logVal = std::log10(num);
	return std::floor(logVal) == logVal;
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() {
	auto bounds = getLocalBounds();

	//bounds.reduce(10, /*JUCE_LIVE_CONSTANT(10),*/ 
	//			  8 /*JUCE_LIVE_CONSTANT(8)*/);

	bounds.removeFromTop(12);
	bounds.removeFromBottom(2);

	bounds.removeFromLeft(30);
	bounds.removeFromRight(30);

	return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea() {
	auto bounds = getRenderArea();

	bounds.removeFromTop(4);
	bounds.removeFromBottom(14);

	return bounds;
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor &p)
	: AudioProcessorEditor(&p), audioProcessor(p),

	responseCurveComponent(audioProcessor),

	peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
	peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
	peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
	lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
	highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
	lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
	highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),


	peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
	peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
	peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
	lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
	highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
	lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),

	lowCutControls("Low Cut"),
	peakControls("Peak Control"),
	highCutControls("HighCut"){

	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.



	peakFreqSlider.labels.add({ 0.f, "20Hz" });
	peakFreqSlider.labels.add({ 1.f, "20kHz" });

	peakGainSlider.labels.add({ 0.f, "-24dB" });
	peakGainSlider.labels.add({ 1.f, "+24dB" });

	peakQualitySlider.labels.add({ 0.f, "0.1" });
	peakQualitySlider.labels.add({ 1.f, "10.0" });

	lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
	lowCutFreqSlider.labels.add({ 1.f, "20kHz" });

	highCutFreqSlider.labels.add({ 0.f, "20Hz" });
	highCutFreqSlider.labels.add({ 1.f, "20kHz" });

	lowCutSlopeSlider.labels.add({ 0.f, "12" });
	lowCutSlopeSlider.labels.add({ 1.f, "48" });

	highCutSlopeSlider.labels.add({ 0.f, "12" });
	highCutSlopeSlider.labels.add({ 1.f, "48" });

	lowCutControls.addKnob(&lowCutFreqSlider);
	lowCutControls.addKnob(&lowCutSlopeSlider);

	peakControls.addKnob(&peakGainSlider);
	peakControls.addKnob(&peakFreqSlider);
	peakControls.addKnob(&peakQualitySlider);

	highCutControls.addKnob(&highCutFreqSlider);
	highCutControls.addKnob(&highCutSlopeSlider);

	for (auto *comp : getComps()) {
		addAndMakeVisible(comp);
	}

	//setSize(1072, 792);
	setSize(1258, 795);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor() {
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics &g) {
	using namespace juce;

	ColourGradient bg(
		Colour(0xFF1A1A26),
		0.0f, 0.0f,
		Colour(0xFF1A1A26),
		0.0f, static_cast<float>(getHeight()),
		false
	);

	bg.addColour(0.5f, Colour(0xFF1E1E2A));

	g.setGradientFill(bg);
	g.fillAll();

}

void SimpleEQAudioProcessorEditor::resized() {
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..

	auto bounds = getLocalBounds();

	bounds.removeFromTop(80);

	auto responseArea = bounds.removeFromTop(483);

	responseCurveComponent.setOpaque(false);
	responseCurveComponent.setBounds(responseArea);

	bounds.removeFromTop(24);

	//auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
	//auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	auto cardsBound = bounds;

	cardsBound.setHeight(lowCutControls.getHeight());
	cardsBound.removeFromLeft(24);
// You can use the width you previously set
	lowCutControls.setBounds(cardsBound.removeFromLeft(lowCutControls.getWidth() + controlCardGap));
	peakControls.setBounds(cardsBound.removeFromLeft(peakControls.getWidth() + controlCardGap));
	highCutControls.setBounds(cardsBound.removeFromLeft(highCutControls.getWidth()));


	/*lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
	lowCutSlopeSlider.setBounds(lowCutArea);
	highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
	highCutSlopeSlider.setBounds(highCutArea);

	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
	peakQualitySlider.setBounds(bounds);*/
}

std::vector<juce::Component *> SimpleEQAudioProcessorEditor::getComps() {
	return {
		&responseCurveComponent,
		&lowCutControls,
		&peakControls,
		&highCutControls
	};
}