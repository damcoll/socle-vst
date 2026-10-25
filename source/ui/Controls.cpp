#include "Controls.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace ui
{
    namespace
    {
        constexpr float knobStart = -2.35619f; // -135 degrees
        constexpr float knobEnd = 2.35619f;
    }

    Knob::Knob (float defaultValue)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (knobStart, knobEnd, true);
        setDoubleClickReturnValue (true, defaultValue);
        setMouseDragSensitivity (180);
        setVelocityBasedMode (false);
    }

    void Knob::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto disc = bounds.withSizeKeepingCentre (size, size);
        const auto centre = disc.getCentre();
        const auto radius = size * 0.5f;

        g.setColour (colours::graphite);
        g.fillEllipse (disc);
        g.setColour (isMouseOverOrDragging() ? colours::craie : colours::poussiere);
        g.drawEllipse (disc.reduced (1.0f), 2.0f);

        const auto angle = knobStart + (float) valueToProportionOfLength (getValue()) * (knobEnd - knobStart);
        const auto direction = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (colours::craie);
        g.drawLine ({ centre + direction * (radius * 0.3f), centre + direction * (radius - 5.0f) }, 2.0f);
    }

    Fader::Fader (float defaultValue)
    {
        setSliderStyle (juce::Slider::LinearVertical);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setDoubleClickReturnValue (true, defaultValue);
        setVelocityBasedMode (false);
    }

    void Fader::paint (juce::Graphics& g)
    {
        constexpr float capHeight = 12.0f;
        const auto bounds = getLocalBounds().toFloat();
        const auto travel = bounds.getHeight() - capHeight;
        const auto top = bounds.getY() + travel * (1.0f - (float) valueToProportionOfLength (getValue()));

        g.setColour (colours::graphite);
        g.fillRect (juce::Rectangle<float> (4.0f, bounds.getHeight()).withCentre (bounds.getCentre()));

        const auto cap = juce::Rectangle<float> (bounds.getX() + 1.0f, top, bounds.getWidth() - 2.0f, capHeight);
        g.setColour (colours::poussiere);
        g.fillRect (cap);
        g.setColour (isMouseOverOrDragging() ? colours::ambre : colours::craie);
        g.drawRect (cap, 2.0f);
    }

    Segmented::Segmented (juce::RangedAudioParameter& parameter, const juce::StringArray& keyLabels, bool isVertical)
        : labels (keyLabels),
          vertical (isVertical),
          attachment (parameter, [this] (float value)
          {
              selected = juce::jlimit (0, labels.size() - 1, juce::roundToInt (value));
              repaint();
          })
    {
        setTitle (parameter.getName (64));
        attachment.sendInitialUpdate();
    }

    juce::Rectangle<float> Segmented::keyBounds (int index) const
    {
        constexpr float gap = 4.0f;
        const auto n = (float) labels.size();
        const auto bounds = getLocalBounds().toFloat();
        if (vertical)
        {
            const auto h = (bounds.getHeight() - gap * (n - 1.0f)) / n;
            return { bounds.getX(), bounds.getY() + (float) index * (h + gap), bounds.getWidth(), h };
        }
        const auto w = (bounds.getWidth() - gap * (n - 1.0f)) / n;
        return { bounds.getX() + (float) index * (w + gap), bounds.getY(), w, bounds.getHeight() };
    }

    int Segmented::keyAt (juce::Point<int> position) const
    {
        for (int i = 0; i < labels.size(); ++i)
            if (keyBounds (i).expanded (2.0f).contains (position.toFloat()))
                return i;
        return -1;
    }

    void Segmented::paint (juce::Graphics& g)
    {
        g.setFont (font (Face::mono, 10.0f, 0.1f));
        for (int i = 0; i < labels.size(); ++i)
        {
            const auto key = keyBounds (i).reduced (0.5f);
            const auto on = i == selected;
            g.setColour (on ? colours::ambre : colours::ecran);
            g.fillRect (key);
            g.setColour (i == hovered && ! on ? colours::craie : colours::beton);
            g.drawRect (key, 1.0f);
            g.setColour (on ? colours::noirAbri : colours::poussiere);
            g.drawText (labels[i], key, juce::Justification::centred);
        }
    }

    void Segmented::mouseDown (const juce::MouseEvent& e)
    {
        if (const auto key = keyAt (e.getPosition()); key >= 0 && key != selected)
            attachment.setValueAsCompleteGesture ((float) key);
    }

    void Segmented::mouseMove (const juce::MouseEvent& e)
    {
        if (const auto key = keyAt (e.getPosition()); key != hovered)
        {
            hovered = key;
            repaint();
        }
    }

    void Segmented::mouseExit (const juce::MouseEvent&)
    {
        hovered = -1;
        repaint();
    }

    Selector::Selector (juce::RangedAudioParameter& parameter, const juce::StringArray& choiceLabels)
        : labels (choiceLabels),
          attachment (parameter, [this] (float value)
          {
              selected = juce::jlimit (0, labels.size() - 1, juce::roundToInt (value));
              repaint();
          })
    {
        setTitle (parameter.getName (64));
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        attachment.sendInitialUpdate();
    }

    void Selector::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (colours::ecran);
        g.fillRect (bounds);
        g.setColour (isMouseOver() ? colours::poussiere : colours::beton);
        g.drawRect (bounds, 2.0f);

        g.setColour (colours::ambre);
        g.setFont (font (Face::monoBold, 10.0f, 0.08f));
        g.drawText (labels[selected], bounds, juce::Justification::centred);
    }

    void Selector::mouseDown (const juce::MouseEvent&)
    {
        juce::PopupMenu menu;
        for (int i = 0; i < labels.size(); ++i)
            menu.addItem (i + 1, labels[i], true, i == selected);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [safe = juce::Component::SafePointer<Selector> (this)] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->attachment.setValueAsCompleteGesture ((float) (result - 1));
                            });
    }

    void Selector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
    {
        const auto step = wheel.deltaY > 0.0f ? 1 : (wheel.deltaY < 0.0f ? -1 : 0);
        const auto next = juce::jlimit (0, labels.size() - 1, selected + step);
        if (next != selected)
            attachment.setValueAsCompleteGesture ((float) next);
    }

    void Key::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (down ? colours::ambre : colours::graphite);
        g.fillRoundedRectangle (bounds, 2.0f);
        g.setColour (highlighted && ! down ? colours::craie : colours::poussiere);
        g.drawRoundedRectangle (bounds, 2.0f, 2.0f);

        g.setColour (down ? colours::noirAbri : colours::craie);
        g.setFont (font (Face::mono, 10.0f, 0.15f));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }

    namespace
    {
        // Classic VU scale: the needle moves linearly with the signal voltage, +3 VU at the right stop.
        // 0 VU is set to -6 dBFS, so a hard note lands around 0 and the loudest peaks reach the top.
        constexpr float zeroVuGain = 0.5f;
        constexpr float fullScaleGain = zeroVuGain * 1.4125f; // +3 VU

        float scalePosition (float vuDb) { return juce::Decibels::decibelsToGain (vuDb) * zeroVuGain / fullScaleGain; }

        constexpr float sweep = 0.87f; // half the scale angle, ~50 degrees
    }

    void VuMeter::update (float peak, const juce::String& label)
    {
        const auto target = juce::jlimit (0.0f, 1.0f, peak / fullScaleGain);

        // VU-like ballistics at 30 Hz: quick rise, slower fall
        needle += (target - needle) * (target > needle ? 0.45f : 0.12f);

        if (label != name)
            name = label;
        repaint();
    }

    void VuMeter::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (colours::ecran);
        g.fillRect (bounds);
        g.setColour (colours::beton);
        g.drawRect (bounds, 3.0f);

        g.saveState();
        g.reduceClipRegion (getLocalBounds().reduced (3));

        // the needle pivots on the bottom edge of the screen; the scale and its numbers fill the width
        const auto pivot = juce::Point<float> (bounds.getCentreX(), bounds.getBottom() - 4.0f);
        const auto radius = juce::jmin ((bounds.getWidth() * 0.5f - 12.0f) / std::sin (sweep) - 14.0f, bounds.getHeight() - 34.0f);
        const auto pointAt = [&] (float position, float r)
        {
            const auto angle = (position * 2.0f - 1.0f) * sweep;
            return pivot + juce::Point<float> (std::sin (angle), -std::cos (angle)) * r;
        };

        g.setColour (colours::ambre);

        // scale line, thicker above 0 VU
        juce::Path arc, hot;
        arc.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, -sweep, (scalePosition (0.0f) * 2.0f - 1.0f) * sweep, true);
        hot.addCentredArc (pivot.x, pivot.y, radius + 1.5f, radius + 1.5f, 0.0f, (scalePosition (0.0f) * 2.0f - 1.0f) * sweep, sweep, true);
        g.strokePath (arc, juce::PathStrokeType (1.0f));
        g.strokePath (hot, juce::PathStrokeType (4.0f));

        // minor ticks every dB from -10 to +3, major ticks with numbers
        for (int db = -10; db <= 3; ++db)
        {
            const auto p = scalePosition ((float) db);
            g.drawLine ({ pointAt (p, radius), pointAt (p, radius + 4.0f) }, 1.0f);
        }

        g.setFont (font (Face::mono, 9.0f));
        const std::pair<int, const char*> marks[] = { { -20, "20" }, { -10, "10" }, { -7, "7" }, { -5, "5" }, { -3, "3" }, { 0, "0" }, { 3, "+3" } };
        for (const auto& [db, label] : marks)
        {
            const auto p = scalePosition ((float) db);
            g.drawLine ({ pointAt (p, radius - 4.0f), pointAt (p, radius + 7.0f) }, 1.5f);
            const auto at = pointAt (p, radius + 14.0f);
            g.drawText (label, juce::Rectangle<float> (24.0f, 10.0f).withCentre (at), juce::Justification::centred);
        }

        // needle, and the hub hidden half under the bottom edge
        g.drawLine ({ pivot, pointAt (needle, radius + 6.0f) }, 1.8f);
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (pivot));

        g.setFont (font (Face::mono, 10.0f, 0.05f));
        g.drawText ("VU", 10, getHeight() - 20, 40, 12, juce::Justification::centredLeft);
        g.drawText (name, getWidth() - 70, getHeight() - 20, 60, 12, juce::Justification::centredRight);
        g.restoreState();
    }
}
