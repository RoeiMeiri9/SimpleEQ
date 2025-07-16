/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FontManager.h"

enum FFTOrder {
	order2048 = 11,
	order4096 = 12,
	order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator {
	/**
	produces the FFT data from an audio buffer.
	*/
	void producerFFTDataForRendering(const juce::AudioBuffer<float> &audioData, const float negativeInfinity) {
		const auto fftSize = getFFTSize();

		fftData.assign(fftData.size(), 0);
		auto *readIndex = audioData.getReadPointer(0);
		std::copy(readIndex, readIndex + fftSize, fftData.begin());

		//first apply a windowing function to our data
		window->multiplyWithWindowingTable(fftData.data(), fftSize);		// [1]

		//then render our FFT data..
		forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());	// [2]

		int numBins = (int)fftSize / 2;

		//normalize the fft values.
		for (int i = 0; i < numBins; ++i) {
			fftData[i] /= (float)numBins;
		}

		//convert them to decibels
		for (int i = 0; i < numBins; ++i) {
			fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
		}

		fftDataFifo.push(fftData);
	}

	void changeOrder(FFTOrder newOrder) {
		//when you change order, recreate the window, forwardFFT, fifo, fftData
		//also reset the fifoIndex
		//things that need recreating should be created on the heap via std::make_unique<>

		order = newOrder;
		auto fftSize = getFFTSize();

		forwardFFT = std::make_unique<juce::dsp::FFT>(order);
		window = std::make_unique<juce::dsp::WindowingFunction<float>>(
			fftSize,
			juce::dsp::WindowingFunction<float>::blackmanHarris
		);

		fftData.clear();
		fftData.resize(fftSize * 2, 0);

		fftDataFifo.prepare(fftData.size());
	}
	//==============================================================================
	int getFFTSize() const { return 1 << order; }
	int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
	//==============================================================================
	bool getFFTData(BlockType &fftData) { return fftDataFifo.pull(fftData); }
private:
	FFTOrder order;
	BlockType fftData;
	std::unique_ptr<juce::dsp::FFT> forwardFFT;
	std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

	Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator {
	/*
	converts 'renderData[]' into a juce::Path
	*/
	void generatePath(
		const std::vector<float> &renderData,
		juce::Rectangle<float> fftBounds,
		int fftSize,
		float binWidth,
		float negativeInfinity
	) {
		if (envelopeData.size() != renderData.size())
			envelopeData = std::vector<float>(renderData.size(), negativeInfinity);

		auto top = fftBounds.getY();
		auto bottom = fftBounds.getBottom();
		auto width = fftBounds.getWidth();

		int numBins = static_cast<int>(fftSize) / 2;

		PathType p;

		p.preallocateSpace(3 * static_cast<int>(fftBounds.getWidth()) + 2); // Maybe add 2 more slots for the new dots?

		auto map = [bottom, top, negativeInfinity](float v) {
			return juce::jmap(
				v,
				negativeInfinity, 0.f,
				bottom, top); // In the original implementation by Matkat Music, the bottom was (float)bottom, but bottom is already a float. So should I do the same?
			};
		auto y = map(envelopeData[0]);

		jassert(!std::isnan(y) && !std::isinf(y));

		p.startNewSubPath(0, bottom);
		p.lineTo(0, y);
		
		for (int binNum = 1; binNum < numBins;) {
			int pathResolution = juce::jmap<int>(binNum, 0, numBins, 2, 20);

			float inputVal = renderData[binNum];
			float &env = envelopeData[binNum];

			if (inputVal > env) {
				env += (inputVal - env) * 0.5f; // This will make the bin to snap insantly
			} else {
				env -= 0.85f; // decay per frame (in dB)
				env = std::max(env, inputVal); // don't decay below actual value
			}

			auto y = map(env);

			jassert(!std::isnan(y) && !std::isinf(y));

			if (!std::isnan(y) && !std::isinf(y)) {
				auto binFreq = binNum * binWidth;
				auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
				int binX = std::floor(normalizedBinX * width);
				
				// Next point	
				int nextI = binNum + pathResolution;
				if (nextI >= numBins) {
					p.lineTo(binX, y);
				} else {
					float nextVal = envelopeData[nextI];
					float nextY = map(nextVal);
					float nextFreq = nextI * binWidth;
					auto nextNormalizedBinX = juce::mapFromLog10(nextFreq, 20.f, 20000.f);
					int nextBinX = std::floor(nextNormalizedBinX * width);

					float midX = (binX + nextBinX) * 0.5f;
					float midY = (y + nextY) * 0.5f;

					p.quadraticTo(binX, y, midX, midY);
				}
			}
			binNum += pathResolution;
		}
		p.lineTo(width + 30.f, bottom);
		p.closeSubPath();

		pathFifo.push(p);
	}

	int getNumPathsAvailable() const {
		return pathFifo.getNumAvailableForReading();
	}

	bool getPath(PathType &path) {
		return pathFifo.pull(path);
	}

private:
	Fifo<PathType> pathFifo;
	std::vector<float> envelopeData;

};

struct LookAndFeel: juce::LookAndFeel_V4 {
	void drawRotarySlider(juce::Graphics &,
						  int x, int y, int width, int height,
						  float sliderPosProportional,
						  float rotaryStartAngle,
						  float rotaryEndAngle,
						  juce::Slider &) override;

	juce::Point<float> rotatePointAround(const juce::Point<float> &point,
										 const juce::Point<float> &center,
										 float angleRadians) {
		 // Translate point to origin relative to center
		auto translated = point - center;

		// Rotate using rotation matrix
		auto rotated = juce::Point<float>(
			translated.x * std::cos(angleRadians) - translated.y * std::sin(angleRadians),
			translated.x * std::sin(angleRadians) + translated.y * std::cos(angleRadians)
		);

		// Translate back
		return rotated + center;
	}

private:
	float lineW = 3.0f;
};

struct RotarySliderWithLabels: juce::Slider {
	RotarySliderWithLabels(juce::RangedAudioParameter &rap, const juce::String &unitSuffix): juce::Slider(
		juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
		juce::Slider::TextEntryBoxPosition::NoTextBox
	),
		param(&rap),
		suffix(unitSuffix) {
		setLookAndFeel(&lnf);
	}

	~RotarySliderWithLabels() {
		setLookAndFeel(nullptr);
	}

	struct LabelPos {
		float pos;
		juce::String label;
	};

	juce::Array<LabelPos> labels;

	void paint(juce::Graphics &g) override;
	juce::Rectangle<int> getSliderBounds() const;
	int getTextHeight() const { return 14; };
	juce::String getDisplayString() const;

private:
	LookAndFeel lnf;

	juce::RangedAudioParameter *param;
	juce::String suffix;
};

struct ControlsContainer: public juce::Component {
	ControlsContainer(const juce::String &title): titleLabel("Title", title) {
		titleLabel.setJustificationType(juce::Justification::centred);
		titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFEFFFF));
		addAndMakeVisible(titleLabel);
	}

	void addKnob(RotarySliderWithLabels *knob) {
		rswlList.add(knob);
		addAndMakeVisible(knob);

		repaint();
		resized();
	}

	void paint(juce::Graphics &g) override;
	void resized() override;

private:
	juce::Label titleLabel;
	//NOTE: Do NOT remove any object from here! it will cause use-after-free bugs.
	juce::Array<RotarySliderWithLabels *> rswlList;

	const int knobWidth = 100;
	const int knobHeight = 100;
	const int knobGap = 48;
	const int frameGap = 16;
	const int paddingRightLeft = 48;
	const int paddingTopBottom = 24;
	const int marginBypass = 8;
	const int cornerRadius = 8;
};

struct PathProducer {

	PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType> &scsf):
		channelFifo(&scsf) {
		FFTDataGenerator.changeOrder(FFTOrder::order2048);
		monoBuffer.setSize(1, FFTDataGenerator.getFFTSize());
	}

	void process(juce::Rectangle<float> fftBounds, double sampleRate);

	juce::Path getPath() { return FFTPath; }

private:
	SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType> *channelFifo;

	juce::AudioBuffer<float> monoBuffer;

	FFTDataGenerator<std::vector<float>> FFTDataGenerator;

	AnalyzerPathGenerator<juce::Path> pathProducer;

	juce::Path FFTPath;
};

struct ResponseCurveComponent: juce::Component,
	juce::AudioProcessorParameter::Listener,
	juce::Timer {
	ResponseCurveComponent(SimpleEQAudioProcessor &);
	~ResponseCurveComponent();

	void parameterValueChanged(int parameterIndex, float newValue) override;

	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

	void timerCallback() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

private:
	SimpleEQAudioProcessor &audioProcessor;
	juce::Atomic<bool> parametersChanged{ false };

	MonoChain monoChain;

	void updateChain();
	bool isPowerOfTen(float num);

	juce::Image background;

	juce::Rectangle<int> getRenderArea();

	juce::Rectangle<int> getAnalysisArea();

	PathProducer leftPathProducer, rightPathProducer;

	const int fontHeight = 14;
};

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor: public juce::AudioProcessorEditor {
public:
	SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor &);
	~SimpleEQAudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics &) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	SimpleEQAudioProcessor &audioProcessor;

	RotarySliderWithLabels peakFreqSlider,
		peakGainSlider,
		peakQualitySlider,
		lowCutFreqSlider,
		highCutFreqSlider,
		lowCutSlopeSlider,
		highCutSlopeSlider;

	ResponseCurveComponent responseCurveComponent;

	ControlsContainer lowCutControls,
		peakControls,
		highCutControls;

	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;

	Attachment peakFreqSliderAttachment,
		peakGainSliderAttachment,
		peakQualitySliderAttachment,
		lowCutFreqSliderAttachment,
		highCutFreqSliderAttachment,
		lowCutSlopeSliderAttachment,
		highCutSlopeSliderAttachment;

	std::vector<juce::Component *> getComps();

	const int controlCardGap = 12;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQAudioProcessorEditor)
};
