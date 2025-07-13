/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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

	g.setColour(Colour(97u, 18u, 167u));
	g.fillEllipse(bounds);

	g.setColour(Colour(255u, 154u, 1u));
	g.drawEllipse(bounds, 1.f);

	if (auto *rswl = dynamic_cast<RotarySliderWithLabels *>(&slider)) {
		auto center = bounds.getCentre();
		Path p;

		Rectangle<float> r;
		r.setLeft(center.getX() - 2);
		r.setRight(center.getX() + 2);
		r.setTop(bounds.getY());
		r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

		p.addRoundedRectangle(r, 2.f);

		jassert(rotaryStartAngle < rotaryEndAngle); //TODO: Remove! (only if removed by the tutorial)

		auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

		p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

		g.fillPath(p);

		g.setFont(rswl->getTextHeight()); //TODO: USE THE FF FONT IN THE FUTURE! 
		auto text = rswl->getDisplayString();
		auto strWidth = g.getCurrentFont().getStringWidth(text);

		r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
		r.setCentre(center);

		g.setColour(Colour::fromRGB(10u, 6u, 14u));
		g.fillRect(r);

		g.setColour(Colours::white);
		g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
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

	g.setColour(Colour(0u, 127u, 1u));
	g.setFont(getTextHeight());

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
		r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
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
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor &p): audioProcessor(p) {
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

void ResponseCurveComponent::timerCallback() {
	if (parametersChanged.compareAndSetBool(false, true)) {
		// update the monochain
		updateChain();

		// signal a repaint
		repaint();
	}
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

	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colour::fromRGB(10u, 6u, 14u));

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

	//g.setColour(Colours::orange);
	//g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

	g.setColour(Colours::white);
	g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized() {
	using namespace juce;
	background = Image(Image::PixelFormat::ARGB, getWidth(), getHeight(), true);

	juce::Colour TextColour = Colour::fromRGBA(255u, 255u, 255u, 100u);
	juce::Colour BrightLine = Colour::fromRGBA(255u, 255u, 255u, 50u);
	juce::Colour DarkLine = Colour::fromRGBA(255u, 255u, 255u, 25u);

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
		g.setColour(isPowerOfTen(freqs[i]) ? BrightLine : DarkLine);
		g.drawVerticalLine(x, top, bottom);
		++i;
	}

	const Array<float> gain{
		24, 12, 0, -12, -24
	};

	const int fontHeight = 10;
	g.setFont(fontHeight);

	for (float gDb : gain) {
		auto y = jmap(gDb, -24.f, 24.f, static_cast<float>(bottom), static_cast<float>(top));

		g.setColour(gDb == 0 ? BrightLine : DarkLine);
		g.drawHorizontalLine(y, left, right);

		String str;
		if (gDb > 0)
			str << "+";
		str << gDb;

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setX(getWidth() - textWidth - 5);
		r.setCentre(r.getCentreX(), y);

		g.setColour(TextColour);
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

	bounds.removeFromLeft(20);
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

	peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
	peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
	peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
	lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
	highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
	lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
	highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

	responseCurveComponent(audioProcessor),

	peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
	peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
	peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
	lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
	highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
	lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider) {

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


	for (auto *comp : getComps()) {
		addAndMakeVisible(comp);
	}

	setResizable(true, false);

	setSize(1280, 720);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor() {
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics &g) {
	using namespace juce;

	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colour::fromRGB(10u, 6u, 14u));
}

void SimpleEQAudioProcessorEditor::resized() {
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..

	auto bounds = getLocalBounds();
	float hRatio = 25.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f;
	auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

	responseCurveComponent.setBounds(responseArea);

	bounds.removeFromTop(5);

	auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
	auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
	lowCutSlopeSlider.setBounds(lowCutArea);
	highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
	highCutSlopeSlider.setBounds(highCutArea);

	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
	peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component *> SimpleEQAudioProcessorEditor::getComps() {
	return {
		&peakFreqSlider,
		&peakGainSlider,
		&peakQualitySlider,
		&lowCutFreqSlider,
		&highCutFreqSlider,
		&lowCutSlopeSlider,
		&highCutSlopeSlider,
		&responseCurveComponent,
	};
}