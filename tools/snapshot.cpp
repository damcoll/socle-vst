// Saves a picture of the plugin window, to check the front panel without a DAW.
//
//   socle_snapshot <file.png> [width]

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main (int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI gui;

    const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "snapshot.png");
    const auto width = argc > 2 ? juce::String (argv[2]).getIntValue() : 1100;

    std::unique_ptr<juce::AudioProcessor> processor (createPluginFilter());
    processor->setPlayConfigDetails (0, 2, 48000.0, 512);
    processor->prepareToPlay (48000.0, 512);

    // play a note and move the needle, as if a bass line were playing
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 28, (juce::uint8) 120), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 40, (juce::uint8) 100), 0);
    juce::AudioBuffer<float> buffer (2, 512);
    processor->processBlock (buffer, midi);

    auto* editor = processor->createEditorAndMakeActive();
    editor->setSize (width, width * 600 / 1100);

    // let the editor's timer read the levels a few times
    for (int i = 0; i < 6; ++i)
    {
        juce::MidiBuffer none;
        processor->processBlock (buffer, none);
        juce::Thread::sleep (35);
        juce::Timer::callPendingTimersSynchronously();
    }

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
    file.deleteFile();
    juce::FileOutputStream out (file);
    juce::PNGImageFormat png;
    const auto ok = out.openedOk() && png.writeImageToStream (image, out);
    processor->editorBeingDeleted (editor);
    delete editor;
    return ok ? 0 : 1;
}
