#include "Theme.h"

#include "BinaryData.h"

namespace
{
    struct Typefaces
    {
        juce::Typeface::Ptr stencil = load (BinaryData::BigShouldersStencilDisplayBlack_ttf, BinaryData::BigShouldersStencilDisplayBlack_ttfSize);
        juce::Typeface::Ptr display = load (BinaryData::BigShouldersDisplayExtraBold_ttf, BinaryData::BigShouldersDisplayExtraBold_ttfSize);
        juce::Typeface::Ptr mono = load (BinaryData::IBMPlexMonoRegular_ttf, BinaryData::IBMPlexMonoRegular_ttfSize);
        juce::Typeface::Ptr monoBold = load (BinaryData::IBMPlexMonoSemiBold_ttf, BinaryData::IBMPlexMonoSemiBold_ttfSize);

        static juce::Typeface::Ptr load (const char* data, int size)
        {
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }
    };
}

struct ui::FontsLease::Holder
{
    juce::SharedResourcePointer<Typefaces> typefaces;
};

ui::FontsLease::FontsLease() : holder (std::make_unique<Holder>()) {}
ui::FontsLease::~FontsLease() = default;

juce::Font ui::font (Face face, float height, float tracking)
{
    const juce::SharedResourcePointer<Typefaces> typefaces;

    const auto& t = *typefaces;
    const auto& typeface = face == Face::stencil ? t.stencil
                         : face == Face::display ? t.display
                         : face == Face::mono    ? t.mono
                                                 : t.monoBold;
    return juce::Font (juce::FontOptions (typeface).withHeight (height).withKerningFactor (tracking));
}
