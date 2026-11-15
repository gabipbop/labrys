#include "PatchBayView.h"

#include "Layout.h"

namespace dedale::ui
{
juce::Point<float> jackCentre(Jack jack) noexcept
{
    return {layout::jackX(jack.index), layout::jackY(jack.isOutput, jack.index)};
}

juce::Colour cableColour(int output) noexcept
{
    switch (static_cast<Out>(output))
    {
    case Out::eg1:
    case Out::eg2:
    case Out::seq1Cv:
    case Out::seq2Cv:
        return colour::cableCv;
    case Out::seq1Trig:
    case Out::seq2Trig:
    case Out::clock:
        return colour::cableTrigger;
    case Out::vca:
    case Out::mvco:
    case Out::noise:
    case Out::mixer:
    case Out::umix:
        break;
    }
    return colour::cableAudio;
}

// --- Prises ----------------------------------------------------------------------------------------------

PatchBayView::PatchBayView(DedaleProcessor& processorToControl) : processor(processorToControl)
{
    setTooltip(juce::String::fromUTF8(
        "Glisser d'une prise à une autre pour poser un câble. Clic droit ou double-clic sur une prise pour "
        "retirer ses câbles. Sorties : libellé sur fond plein."));
}

juce::Point<float> PatchBayView::toPanel(const juce::MouseEvent& event) const noexcept
{
    return event.position + getPosition().toFloat();
}

Jack PatchBayView::jackAt(juce::Point<float> position) const noexcept
{
    for (const auto isOutput : {false, true})
    {
        const auto count = isOutput ? kNumOutputs : kNumInputs;
        for (int i = 0; i < count; ++i)
            if (jackCentre({isOutput, i}).getDistanceFrom(position) < 13.0f)
                return {isOutput, i};
    }
    return {};
}

void PatchBayView::paint(juce::Graphics& g)
{
    const auto origin = getPosition().toFloat();
    const auto drawJack = [&](Jack jack, const char* name)
    {
        const auto centre = jackCentre(jack) - origin;
        const auto unusable = !jack.isOutput && jack.index == static_cast<int>(In::midi);
        if (jack.isOutput)
        {
            const auto width = juce::GlyphArrangement::getStringWidth(font(6.2f, true), name) + 7.0f;
            g.setColour(skin().ink);
            g.fillRoundedRectangle(
                juce::Rectangle<float>(width, 9.0f).withCentre({centre.x, centre.y - 16.0f}), 1.6f);
        }
        drawLabel(g, name, centre.x, centre.y - 13.2f, 6.2f, jack.isOutput ? colour::tabInk : skin().ink,
                  jack.isOutput);

        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillEllipse(juce::Rectangle<float>(18.0f, 18.0f).withCentre(centre.translated(0.6f, 1.4f)));
        const auto nut =
            juce::Rectangle<float>(2.0f * layout::kJackRadius, 2.0f * layout::kJackRadius).withCentre(centre);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff1f1ee), nut.getTopLeft(),
                                               juce::Colour(0xff8f9092), nut.getBottomRight(), false));
        g.setOpacity(unusable ? 0.35f : 1.0f);
        g.fillEllipse(nut);
        g.setOpacity(1.0f);
        g.setColour(juce::Colour(0xff2a2a2b));
        g.fillEllipse(juce::Rectangle<float>(11.2f, 11.2f).withCentre(centre));
        g.setColour(juce::Colours::black);
        g.fillEllipse(juce::Rectangle<float>(7.2f, 7.2f).withCentre(centre));
        if (jack == hovered && !unusable)
        {
            g.setColour(skin().lit);
            g.drawEllipse(nut.expanded(2.0f), 1.2f);
        }
    };
    for (int i = 0; i < kNumInputs; ++i)
        drawJack({false, i}, layout::kInputNames[static_cast<std::size_t>(i)]);
    for (int i = 0; i < kNumOutputs; ++i)
        drawJack({true, i}, layout::kOutputNames[static_cast<std::size_t>(i)]);
}

void PatchBayView::mouseMove(const juce::MouseEvent& event)
{
    const auto jack = jackAt(toPanel(event));
    if (jack != hovered)
    {
        hovered = jack;
        repaint();
    }
}

void PatchBayView::mouseExit(const juce::MouseEvent&)
{
    hovered = {};
    repaint();
}

void PatchBayView::removeCablesAt(Jack jack)
{
    if (!jack.isValid())
        return;
    if (!jack.isOutput)
        return processor.disconnectCable(static_cast<In>(jack.index));
    for (int i = 0; i < kNumInputs; ++i)
        if (processor.cableSource(static_cast<In>(i)) == jack.index)
            processor.disconnectCable(static_cast<In>(i));
}

void PatchBayView::mouseDown(const juce::MouseEvent& event)
{
    auto jack = jackAt(toPanel(event));
    if (!jack.isValid())
        return;
    if (event.mods.isPopupMenu())
        return removeCablesAt(jack);

    // Depuis une entrée déjà câblée : on reprend son câble par la sortie d'origine, pour le déplacer.
    if (!jack.isOutput)
    {
        if (const auto source = processor.cableSource(static_cast<In>(jack.index)); source >= 0)
        {
            processor.disconnectCable(static_cast<In>(jack.index));
            jack = {true, source};
        }
    }
    dragFrom = jack;
    dragPosition = toPanel(event);
}

void PatchBayView::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragFrom.isValid())
        return;
    dragPosition = toPanel(event);
    hovered = jackAt(dragPosition);
    repaint();
}

void PatchBayView::mouseUp(const juce::MouseEvent& event)
{
    if (!dragFrom.isValid())
        return;
    const auto target = jackAt(toPanel(event));
    if (target.isValid() && target.isOutput != dragFrom.isOutput)
    {
        const auto output = dragFrom.isOutput ? dragFrom.index : target.index;
        const auto input = dragFrom.isOutput ? target.index : dragFrom.index;
        processor.connectCable(static_cast<Out>(output), static_cast<In>(input));
    }
    dragFrom = {};
    repaint();
}

void PatchBayView::mouseDoubleClick(const juce::MouseEvent& event)
{
    removeCablesAt(jackAt(toPanel(event)));
}

// --- Câbles ----------------------------------------------------------------------------------------------

CableOverlay::CableOverlay(DedaleProcessor& processorToShow, PatchBayView& patchBay)
    : processor(processorToShow), view(patchBay)
{
    setInterceptsMouseClicks(false, false);
}

void CableOverlay::refresh()
{
    juce::String state;
    for (int i = 0; i < kNumInputs; ++i)
        state << processor.cableSource(static_cast<In>(i)) << ',';
    state << view.hovered.index << (view.hovered.isOutput ? 'o' : 'i') << view.dragFrom.index
          << view.dragPosition.toString();
    if (state != shown)
    {
        shown = state;
        repaint();
    }
}

void CableOverlay::paint(juce::Graphics& g)
{
    // Courbe de Bézier avec affaissement : plus les prises sont éloignées, plus le câble pend.
    const auto drawCable = [&](juce::Point<float> from, juce::Point<float> to, juce::Colour cable,
                               bool highlighted, bool plugAtEnd)
    {
        const auto low = juce::jmax(from.y, to.y) + 24.0f + std::abs(from.x - to.x) * 0.3f;
        juce::Path path;
        path.startNewSubPath(from);
        path.cubicTo(from.x, low, to.x, low, to.x, to.y);
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.strokePath(path,
                     juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                     juce::AffineTransform::translation(1.5f, 2.5f));
        g.setColour(highlighted ? cable.brighter(0.5f) : cable.withAlpha(0.92f));
        g.strokePath(path, juce::PathStrokeType(highlighted ? 4.4f : 3.4f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
        for (const auto end : {from, to})
        {
            if (end == to && !plugAtEnd)
                continue;
            g.setColour(cable);
            g.fillEllipse(juce::Rectangle<float>(12.4f, 12.4f).withCentre(end));
            g.setColour(juce::Colours::black.withAlpha(0.45f));
            g.fillEllipse(juce::Rectangle<float>(4.8f, 4.8f).withCentre(end));
        }
    };

    for (int i = 0; i < kNumInputs; ++i)
    {
        const auto source = processor.cableSource(static_cast<In>(i));
        if (source < 0)
            continue;
        const auto highlighted = view.hovered == Jack{false, i} || view.hovered == Jack{true, source};
        drawCable(jackCentre({true, source}), jackCentre({false, i}), cableColour(source), highlighted, true);
    }
    if (view.dragFrom.isValid())
        drawCable(jackCentre(view.dragFrom), view.dragPosition,
                  view.dragFrom.isOutput ? cableColour(view.dragFrom.index) : skin().sub, true, false);
}
} // namespace dedale::ui
