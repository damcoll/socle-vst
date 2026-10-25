#include "PluginEditor.h"

namespace
{
    constexpr int headerHeight = 40;
}

SocleEditor::SocleEditor (SocleProcessor& p)
    : AudioProcessorEditor (p), processor (p), generic (p)
{
    addAndMakeVisible (generic);
    setResizable (true, true);
    setSize (520, 640);
    startTimerHz (15);
}

void SocleEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0e0e0e));
    g.setColour (juce::Colour (0xffe9e7e2));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("SOCLE", 12, 0, 200, headerHeight, juce::Justification::centredLeft);

    const auto note = shownNote < 0 ? juce::String ("-")
                                    : juce::String (shownNote) + juce::String::fromUTF8 (" · ")
                                          + juce::MidiMessage::getMidiNoteName (shownNote, true, true, 3);
    g.setColour (juce::Colour (0xffffb000));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (juce::String::fromUTF8 ("DERNIÈRE NOTE ") + note, getLocalBounds().removeFromTop (headerHeight).reduced (12, 0),
                juce::Justification::centredRight);
}

void SocleEditor::resized()
{
    generic.setBounds (getLocalBounds().withTrimmedTop (headerHeight));
}

void SocleEditor::timerCallback()
{
    if (const auto note = processor.lastNote.load (std::memory_order_relaxed); note != shownNote)
    {
        shownNote = note;
        repaint (0, 0, getWidth(), headerHeight);
    }
}
