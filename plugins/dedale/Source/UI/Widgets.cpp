#include "Widgets.h"

#include "Layout.h"

#include <BinaryData.h>

namespace dedale::ui
{
namespace
{
#define DEDALE_FACE(name)                                                                                    \
    juce::Typeface::createSystemTypefaceFor(BinaryData::name##_ttf, BinaryData::name##_ttfSize)

// Polices par famille : texte courant, gras, titre.
juce::Typeface::Ptr typeface(int family, int role)
{
    static const std::array<std::array<juce::Typeface::Ptr, 3>, 4> faces{{
        {DEDALE_FACE(BarlowCondensedSemiBold), DEDALE_FACE(BarlowCondensedBold),
         DEDALE_FACE(BarlowCondensedBold)},
        {DEDALE_FACE(SairaCondensedSemiBold), DEDALE_FACE(SairaCondensedBold),
         DEDALE_FACE(SairaStencilOneRegular)},
        {DEDALE_FACE(ShareTechMonoRegular), DEDALE_FACE(ShareTechMonoRegular), DEDALE_FACE(QuanticoBold)},
        {DEDALE_FACE(ChakraPetchSemiBold), DEDALE_FACE(ChakraPetchBold), DEDALE_FACE(ChakraPetchBold)},
    }};
    return faces[static_cast<std::size_t>(family)][static_cast<std::size_t>(role)];
}

int currentSkin = 0;

juce::Point<float> onCircle(juce::Point<float> centre, float radius, float angle)
{
    return centre + juce::Point<float>(std::sin(angle), -std::cos(angle)) * radius;
}
} // namespace

const Skin& skin() noexcept
{
    static const std::array<Skin, kNumSkins> skins{{
        // 0 : la première version (classique)
        {"Classique", 0, 1.0f, juce::Colour(0xffefe8d8), juce::Colour(0xffa8a294), juce::Colour(0xfff4b63a),
         juce::Colour(0xff7a3f1c), juce::Colour(0xffc8733a), LedShape::round, false, false, false},
        // 1 : atelier, acier brut, pochoir, fentes lumineuses, rouge signal
        {"Acier", 1, 0.98f, juce::Colour(0xfff2f2ee), juce::Colour(0xff9a9b9d), juce::Colour(0xffff3b1f),
         juce::Colour(0xff2c2d30), juce::Colour(0xff6f7276), LedShape::slit, true, true, false},
        // 2 : appareil de mesure, police de terminal, voyants carrés, tout en blanc et rouge
        {"Terminal", 2, 0.84f, juce::Colour(0xffffffff), juce::Colour(0xff8c8c8c), juce::Colour(0xffff2a1a),
         juce::Colour(0xff0c0c0c), juce::Colour(0xff1c1c1c), LedShape::lamp, false, true, false},
        // 3 : chantier, pans coupés partout, jaune de signalisation
        {"Chantier", 3, 0.86f, juce::Colour(0xfff5f1e6), juce::Colour(0xff9c978a), juce::Colour(0xffffd400),
         juce::Colour(0xff151515), juce::Colour(0xffffd400), LedShape::cut, true, true, true},
    }};
    return skins[static_cast<std::size_t>(currentSkin)];
}

void setSkin(int index) noexcept
{
    currentSkin = juce::jlimit(0, kNumSkins - 1, index);
}

juce::Font font(float height, bool bold, float kerning)
{
    return juce::Font(juce::FontOptions(typeface(skin().fontFamily, bold ? 1 : 0))
                          .withHeight(height * skin().fontScale))
        .withExtraKerningFactor(kerning);
}

juce::Font titleFont(float height)
{
    return juce::Font(juce::FontOptions(typeface(skin().fontFamily, 2)).withHeight(height))
        .withExtraKerningFactor(0.2f);
}

juce::Path framePath(juce::Rectangle<float> r, float corner)
{
    juce::Path path;
    if (!skin().chamfer)
    {
        path.addRoundedRectangle(r, corner);
        return path;
    }
    const auto c = corner * 0.8f;
    path.startNewSubPath(r.getX() + c, r.getY());
    path.lineTo(r.getRight() - c, r.getY());
    path.lineTo(r.getRight(), r.getY() + c);
    path.lineTo(r.getRight(), r.getBottom() - c);
    path.lineTo(r.getRight() - c, r.getBottom());
    path.lineTo(r.getX() + c, r.getBottom());
    path.lineTo(r.getX(), r.getBottom() - c);
    path.lineTo(r.getX(), r.getY() + c);
    path.closeSubPath();
    return path;
}

void drawLabel(juce::Graphics& g, const juce::String& text, float cx, float y, float height,
               juce::Colour colour, bool bold, juce::Justification justification)
{
    g.setColour(colour);
    g.setFont(font(height, bold));
    const auto width = 160.0f;
    const auto left = justification.testFlags(juce::Justification::left)    ? cx
                      : justification.testFlags(juce::Justification::right) ? cx - width
                                                                            : cx - width / 2.0f;
    g.drawText(text, juce::Rectangle<float>(left, y - height, width, height * 1.3f), justification, false);
}

void drawGlass(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(colour::glass);
    g.fillRoundedRectangle(area, 3.0f);
    g.setColour(colour::bezelStroke);
    g.drawRoundedRectangle(area, 3.0f, 1.2f);
}

const char* quantizerName(int mode) noexcept
{
    static constexpr std::array<const char*, 16> names{
        "LIBRE",     "CHROMA", "MAJEUR", "PENTA", "MIN MEL", "MIN HARM", "DIM 6",   "PAR TONS",
        "HIRAJOSHI", "7SUS4",  "MAJ7",   "MAJ13", "MIN7",    "MIN11",    "HANDPAN", "QUADS"};
    return names[static_cast<std::size_t>(juce::jlimit(0, 15, mode))];
}

// --- Knob ------------------------------------------------------------------------------------------------

KnobLookAndFeel::KnobLookAndFeel()
{
    setColour(juce::BubbleComponent::backgroundColourId, colour::glass);
    setColour(juce::BubbleComponent::outlineColourId, colour::bezelStroke);
    setColour(juce::TooltipWindow::backgroundColourId, colour::glass);
    setColour(juce::TooltipWindow::textColourId, skin().ink);
    setColour(juce::TooltipWindow::outlineColourId, colour::bezelStroke);
    setColour(juce::Label::textColourId, skin().lit);
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position,
                                       float startAngle, float endAngle, juce::Slider& slider)
{
    const auto centre = juce::Rectangle<int>(x, y, width, height).toFloat().getCentre();
    const auto r = static_cast<Knob&>(slider).radius;
    const auto big = r >= 9.0f;
    const auto angle = startAngle + position * (endAngle - startAngle);

    if (big)
    {
        g.setColour(skin().ink.withAlpha(0.85f));
        for (int i = 0; i <= 10; ++i)
        {
            const auto a = startAngle + static_cast<float>(i) / 10.0f * (endAngle - startAngle);
            const auto major = i % 5 == 0;
            g.drawLine({onCircle(centre, r + 2.6f, a), onCircle(centre, r + (major ? 6.2f : 4.8f), a)},
                       major ? 1.0f : 0.7f);
        }
    }

    g.setColour(juce::Colours::black.withAlpha(0.4f)); // ombre portée
    g.fillEllipse(juce::Rectangle<float>(2.0f * r + 1.2f, 2.0f * r + 1.2f)
                      .withCentre(centre.translated(r * 0.06f, r * 0.16f)));

    const auto body = juce::Rectangle<float>(2.0f * r, 2.0f * r).withCentre(centre);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a3c), centre.x - r * 0.3f, centre.y - r * 0.4f,
                                           juce::Colour(0xff050505), centre.x + r, centre.y + r, true));
    g.fillEllipse(body);
    g.setColour(juce::Colours::black);
    g.drawEllipse(body, 0.6f);

    if (big)
    {
        // Cannelures de la jupe
        juce::Path ring, flutes;
        ring.addEllipse(body.reduced(1.3f));
        const auto circumference = juce::MathConstants<float>::twoPi * (r - 1.3f);
        const std::array<float, 2> dashes{
            1.1f, circumference / static_cast<float>(juce::roundToInt(r * 1.8f)) - 1.1f};
        juce::PathStrokeType(1.6f).createDashedStroke(flutes, ring, dashes.data(), 2);
        g.setColour(juce::Colour(0xff3d3d40));
        g.fillPath(flutes);

        // Capuchon aluminium
        const auto cap = juce::Rectangle<float>(1.12f * r, 1.12f * r).withCentre(centre);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff4f4f2), cap.getTopLeft(),
                                               juce::Colour(0xff6c6d70), cap.getBottomRight(), false));
        g.fillEllipse(cap);
        g.setColour(juce::Colour(0xff101010));
        g.drawEllipse(cap, 0.6f);
    }

    g.setColour(juce::Colours::white);
    juce::Path pointer;
    pointer.addLineSegment(
        {onCircle(centre, big ? r * 0.6f : 0.0f, angle), onCircle(centre, r * 0.94f, angle)}, 0.0f);
    g.strokePath(pointer, juce::PathStrokeType(juce::jmax(1.3f, r * 0.1f), juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

Knob::Knob(juce::AudioProcessorValueTreeState& state, const char* parameterId, float knobRadius,
           const char* tooltip, juce::Component& popupParent)
    : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox), radius(knobRadius),
      attachment(state, parameterId, *this)
{
    setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setPopupDisplayEnabled(true, true, &popupParent);
    setTooltip(juce::String::fromUTF8(tooltip));
    setMouseDragSensitivity(180);
}

// --- Bouton ----------------------------------------------------------------------------------------------

SquareButton::SquareButton(const juce::String& tooltip) : juce::Button({})
{
    setTooltip(tooltip);
}

void SquareButton::setLit(bool shouldBeLit)
{
    if (lit != shouldBeLit)
    {
        lit = shouldBeLit;
        repaint();
    }
}

void SquareButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const auto on = lit || down || getToggleState();
    const auto face = getLocalBounds().toFloat().reduced(3.0f);
    if (on)
    {
        g.setColour(skin().lit.withAlpha(0.22f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    }
    const auto shape = framePath(face, skin().chamfer ? 3.5f : 2.0f);
    g.setColour(on ? skin().lit : highlighted ? colour::off.brighter(0.15f) : colour::off);
    g.fillPath(shape);
    g.setColour(on ? juce::Colours::black.withAlpha(0.35f) : colour::offStroke);
    g.strokePath(shape, juce::PathStrokeType(0.8f));
}

// --- LEDs du séquenceur ----------------------------------------------------------------------------------

void SequencerLeds::refresh(ButtonComboHandler::DisplayMode mode, int clockDivision)
{
    auto& d = processor.display;
    juce::uint64 state = static_cast<juce::uint64>(mode) | (static_cast<juce::uint64>(clockDivision) << 2) |
                         (static_cast<juce::uint64>(d.quantMode.load()) << 8) |
                         (d.chained.load() ? 1u << 13 : 0u);
    for (std::size_t lane = 0; lane < 2; ++lane)
    {
        const auto packed = static_cast<juce::uint64>(d.bits[lane].load()) |
                            (static_cast<juce::uint64>(d.playHead[lane].load()) << 8) |
                            (static_cast<juce::uint64>(d.writeHead[lane].load()) << 13) |
                            (static_cast<juce::uint64>(d.length[lane].load()) << 18);
        state |= packed << (16 + 24 * lane);
    }
    if (state == shown)
        return;
    shown = state;
    displayMode = mode;
    division = clockDivision;
    repaint();
}

void SequencerLeds::paint(juce::Graphics& g)
{
    using Mode = ButtonComboHandler::DisplayMode;
    auto& d = processor.display;
    const auto origin = getPosition().toFloat();
    const auto chained = d.chained.load();
    const auto length1 = d.length[0].load();

    for (int lane = 0; lane < 2; ++lane)
    {
        const auto l = static_cast<std::size_t>(lane);
        const auto y = layout::kSeqRowY[l] - origin.y;
        for (int step = 0; step < 8; ++step)
        {
            const auto centre = juce::Point<float>(
                layout::kLedX0 + static_cast<float>(step) * layout::kLedSpacing - origin.x, y);
            auto led = colour::ledOff;
            auto ring = false;
            auto active = step < d.length[l].load();

            if (displayMode == Mode::sequence)
            {
                // En mode chain, les deux play heads parcourent les pas de SEQ1 puis ceux de SEQ2.
                const auto position = chained && lane == 1 ? step + length1 : step;
                const auto isHead = [&](int head, int owner)
                { return chained ? head == position : owner == lane && head == step; };
                const auto playing = isHead(d.playHead[0].load(), 0) || isHead(d.playHead[1].load(), 1);
                ring = active && (isHead(d.writeHead[0].load(), 0) || isHead(d.writeHead[1].load(), 1));
                if (active && playing)
                    led = colour::ledGreen;
                else if (active && (d.bits[l].load() >> step & 1) != 0)
                    led = colour::ledRed;
            }
            else
            {
                // Réglage : les 16 LEDs en vert sombre, une rouge sur la valeur courante (1 à 16).
                const auto value = displayMode == Mode::quantizer ? d.quantMode.load() : division - 1;
                active = true;
                led = lane * 8 + step == value ? colour::ledRed
                                               : colour::ledGreen.withMultipliedBrightness(0.35f);
            }

            const auto on = led != colour::ledOff;
            const auto dark = active ? colour::ledOff : colour::ledOff.withMultipliedBrightness(0.5f);
            const auto rim = active ? colour::bezelStroke : colour::bezelStroke.withAlpha(0.35f);
            switch (skin().led)
            {
            case LedShape::round:
            {
                if (on)
                {
                    g.setColour(led.withAlpha(0.2f));
                    g.fillEllipse(juce::Rectangle<float>(17.0f, 17.0f).withCentre(centre));
                }
                const auto bezel = juce::Rectangle<float>(12.8f, 12.8f).withCentre(centre);
                g.setColour(colour::bezel);
                g.fillEllipse(bezel);
                g.setColour(rim);
                g.drawEllipse(bezel, 0.8f);
                g.setColour(on ? led : dark);
                g.fillEllipse(juce::Rectangle<float>(8.6f, 8.6f).withCentre(centre));
                if (on)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.55f));
                    g.fillEllipse(
                        juce::Rectangle<float>(2.4f, 2.4f).withCentre(centre.translated(-1.3f, -1.4f)));
                }
                break;
            }
            case LedShape::slit: // fente lumineuse : barre étroite, bords vifs
            {
                const auto slot = juce::Rectangle<float>(16.0f, 6.0f).withCentre(centre);
                g.setColour(colour::bezel);
                g.fillRect(slot.expanded(1.4f));
                g.setColour(rim);
                g.drawRect(slot.expanded(1.4f), 0.7f);
                g.setColour(on ? led : dark);
                g.fillRect(slot);
                if (on)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.5f));
                    g.fillRect(slot.withHeight(1.2f));
                }
                break;
            }
            case LedShape::lamp: // voyant carré dans un cadre métal vissé
            {
                const auto frame = juce::Rectangle<float>(14.0f, 14.0f).withCentre(centre);
                g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd9d9d6), frame.getTopLeft(),
                                                       juce::Colour(0xff55565a), frame.getBottomRight(),
                                                       false));
                g.fillRect(frame);
                g.setColour(juce::Colours::black);
                g.fillRect(frame.reduced(2.2f));
                g.setColour(on ? led : dark);
                g.fillRect(frame.reduced(3.2f));
                if (!active)
                {
                    g.setColour(juce::Colours::black.withAlpha(0.55f));
                    g.fillRect(frame);
                }
                break;
            }
            case LedShape::cut: // pastille à pans coupés
            {
                const auto lens = juce::Rectangle<float>(13.0f, 13.0f).withCentre(centre);
                juce::Path octagon;
                octagon.addPolygon(centre, 8, 7.6f, juce::MathConstants<float>::pi / 8.0f);
                g.setColour(colour::bezel);
                g.fillPath(octagon);
                g.setColour(rim);
                g.strokePath(octagon, juce::PathStrokeType(0.9f));
                juce::Path inner;
                inner.addPolygon(centre, 8, 5.2f, juce::MathConstants<float>::pi / 8.0f);
                g.setColour(on ? led : dark);
                g.fillPath(inner);
                juce::ignoreUnused(lens);
                break;
            }
            }
            if (ring)
            {
                g.setColour(colour::writeHead);
                if (skin().led == LedShape::round)
                {
                    g.drawEllipse(juce::Rectangle<float>(18.8f, 18.8f).withCentre(centre), 1.5f);
                }
                else // repère du write head : deux traits cyan, au-dessus et au-dessous
                {
                    g.fillRect(
                        juce::Rectangle<float>(16.0f, 1.6f).withCentre(centre.translated(0.0f, -10.0f)));
                    g.fillRect(
                        juce::Rectangle<float>(16.0f, 1.6f).withCentre(centre.translated(0.0f, 10.0f)));
                }
            }
        }
    }
}

// --- Afficheur -------------------------------------------------------------------------------------------

void ModeDisplay::show(const juce::String& newText)
{
    if (text != newText)
    {
        text = newText;
        repaint();
    }
}

void ModeDisplay::paint(juce::Graphics& g)
{
    drawGlass(g, getLocalBounds().toFloat().reduced(0.6f));
    g.setColour(skin().lit);
    g.setFont(font(12.5f, false, 0.14f));
    g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
}

// --- ORDER -----------------------------------------------------------------------------------------------

OrderSwitch::OrderSwitch(juce::RangedAudioParameter& parameter)
    : attachment(parameter,
                 [this](float value)
                 {
                     position = juce::roundToInt(value);
                     repaint();
                 })
{
    setTooltip(juce::String::fromUTF8(
        "Ordre des deux chemins : en parallèle, wavefolder puis filtre, ou filtre puis wavefolder."));
    attachment.sendInitialUpdate();
}

void OrderSwitch::paint(juce::Graphics& g)
{
    const juce::Rectangle<float> slot(0.6f, 0.6f, 14.0f, 46.0f);
    g.setColour(colour::glass);
    g.fillRoundedRectangle(slot, 3.0f);
    g.setColour(colour::bezelStroke);
    g.drawRoundedRectangle(slot, 3.0f, 1.0f);
    g.setColour(skin().ink);
    g.fillRoundedRectangle(slot.withHeight(14.0f)
                               .translated(0.0f, 1.0f + static_cast<float>(position) * 15.0f)
                               .reduced(1.5f, 0.0f),
                           2.0f);

    static constexpr std::array<const char*, 3> names{"PARALLEL", "VCW > VCF", "VCF > VCW"};
    for (int i = 0; i < 3; ++i)
        drawLabel(g, names[static_cast<std::size_t>(i)], 20.0f, 11.5f + static_cast<float>(i) * 15.0f, 7.4f,
                  i == position ? skin().ink : skin().sub, false, juce::Justification::centredLeft);
}

void OrderSwitch::mouseDown(const juce::MouseEvent& event)
{
    attachment.setValueAsCompleteGesture(static_cast<float>(juce::jlimit(0, 2, (event.y - 1) / 15)));
}

// --- Oscilloscope ----------------------------------------------------------------------------------------

ScopeView::ScopeView(DedaleProcessor& processorToShow) : processor(processorToShow)
{
    setTooltip(juce::String::fromUTF8("Oscilloscope de la sortie. Un clic l'éteint ou le rallume."));
}

void ScopeView::refresh()
{
    std::array<float, 2048> fresh;
    const auto count =
        static_cast<std::size_t>(processor.pullScope(fresh.data(), static_cast<int>(fresh.size())));
    if (count == 0 || !enabled)
        return;
    std::move(samples.begin() + static_cast<std::ptrdiff_t>(count), samples.end(), samples.begin());
    std::copy_n(fresh.begin(), count, samples.end() - static_cast<std::ptrdiff_t>(count));
    repaint();
}

void ScopeView::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced(0.6f);
    drawGlass(g, area);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine(juce::roundToInt(area.getX() + area.getWidth() * static_cast<float>(i) / 8.0f),
                           area.getY() + 1.0f, area.getBottom() - 1.0f);
    g.drawHorizontalLine(juce::roundToInt(area.getCentreY()), area.getX() + 1.0f, area.getRight() - 1.0f);
    if (!enabled)
        return drawLabel(g, "SCOPE OFF", area.getCentreX(), area.getCentreY() + 3.0f, 8.0f, skin().sub);

    // Départ sur un passage à zéro montant, pour une image stable.
    constexpr std::size_t window = 1024;
    std::size_t start = samples.size() - window;
    for (std::size_t i = samples.size() - window; i > 1; --i)
    {
        if (samples[i - 1] < 0.0f && samples[i] >= 0.0f)
        {
            start = i;
            break;
        }
    }
    start = juce::jmin(start, samples.size() - window);

    juce::Path trace;
    const auto inner = area.reduced(5.0f, 4.0f);
    for (std::size_t i = 0; i < window; i += 3)
    {
        const auto x = inner.getX() + inner.getWidth() * static_cast<float>(i) / static_cast<float>(window);
        const auto y = inner.getCentreY() -
                       juce::jlimit(-1.0f, 1.0f, samples[start + i] * 1.6f) * inner.getHeight() * 0.5f;
        i == 0 ? trace.startNewSubPath(x, y) : trace.lineTo(x, y);
    }
    g.setColour(skin().lit);
    g.strokePath(trace, juce::PathStrokeType(1.1f));
}

void ScopeView::mouseDown(const juce::MouseEvent&)
{
    enabled = !enabled;
    repaint();
}
} // namespace dedale::ui
