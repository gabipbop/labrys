#include "Panel.h"

#include "Layout.h"

namespace dedale::ui
{
namespace
{
constexpr float kRunX = 106, kChainX = 252, kShiftX = 320, kSyncX = 354;
const juce::Rectangle<int> kDisplayArea{380, 136, 112, 22};
const juce::Rectangle<int> kScopeArea{524, 150, 220, 56};
const juce::Rectangle<int> kOrderArea{523, 269, 62, 50};

float labelY(const layout::Knob& knob) noexcept
{
    return knob.y - knob.radius - (knob.radius >= 9.0f ? 9.0f : 5.0f);
}

bool latches(PanelButton button) noexcept // boutons qui restent tenus en mode SHIFT
{
    return button == PanelButton::bitShift1 || button == PanelButton::bitShift2 ||
           button == PanelButton::length1 || button == PanelButton::length2 || button == PanelButton::buffer;
}
} // namespace

// --- Façade ----------------------------------------------------------------------------------------------

void Panel::Background::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(colour::panelTop, 0.0f, 0.0f, colour::panelBottom, 0.0f,
                                           bounds.getHeight(), false));
    g.fillRect(bounds);

    const auto bolt = [&g](juce::Point<float> centre, float radius)
    {
        const auto head = juce::Rectangle<float>(2.0f * radius, 2.0f * radius).withCentre(centre);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe2e2df), head.getTopLeft(),
                                               juce::Colour(0xff4c4d50), head.getBottomRight(), false));
        g.fillEllipse(head);
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.drawEllipse(head, 0.5f);
        g.drawLine({centre.translated(-radius * 0.6f, -radius * 0.35f),
                    centre.translated(radius * 0.6f, radius * 0.35f)},
                   0.8f);
    };

    // Flancs
    for (const auto x : {0.0f, bounds.getWidth() - 18.0f})
    {
        const juce::Rectangle<float> rail(x, 0.0f, 18.0f, bounds.getHeight());
        if (skin().hazard)
        {
            g.setColour(skin().railDark);
            g.fillRect(rail);
            g.saveState();
            g.reduceClipRegion(rail.toNearestInt());
            g.setColour(skin().railLight);
            for (float y = -20.0f; y < bounds.getHeight() + 20.0f; y += 22.0f) // bandes de signalisation
            {
                juce::Path stripe;
                stripe.addQuadrilateral(x, y + 18.0f, x + 18.0f, y, x + 18.0f, y + 11.0f, x, y + 29.0f);
                g.fillPath(stripe);
            }
            g.restoreState();
        }
        else
        {
            juce::ColourGradient metal(skin().railDark, x, 0.0f, skin().railDark.brighter(0.1f), x + 18.0f,
                                       0.0f, false);
            metal.addColour(0.45, skin().railLight);
            g.setGradientFill(metal);
            g.fillRect(rail);
        }
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRect(x == 0.0f ? 17.0f : x, 0.0f, 1.0f, bounds.getHeight());
        if (skin().bolts)
            for (const auto y : {12.0f, bounds.getHeight() - 12.0f})
                bolt({x + 9.0f, y}, 4.2f);
    }

    g.setColour(skin().ink);
    g.setFont(titleFont(24.0f));
    g.drawText(juce::String::fromUTF8(LABRYS_DISPLAY_NAME),
               juce::Rectangle<float>(34.0f, 8.0f, 220.0f, 26.0f), juce::Justification::centredLeft);
    drawLabel(g, juce::String::fromUTF8("SYNTHÉTISEUR GÉNÉRATIF"), 172.0f, 28.0f, 8.4f, skin().sub, false,
              juce::Justification::centredLeft);
    drawLabel(g, "LABRYS", 926.0f, 28.0f, 9.4f, skin().sub, true, juce::Justification::centredRight);
    drawLabel(g, "RAW POWA STUDIO", 926.0f, 439.0f, 6.4f, skin().sub, false,
              juce::Justification::centredRight);

    // Modules : cadre fin, nom dans un cartouche plein
    for (const auto& m : layout::kModules)
    {
        const juce::Rectangle<float> frame(m.x, m.y, m.w, m.h);
        g.setColour(skin().ink);
        g.strokePath(framePath(frame, 7.0f), juce::PathStrokeType(skin().chamfer ? 1.3f : 1.1f));
        const auto name = juce::String::fromUTF8(m.name);
        const auto width = juce::GlyphArrangement::getStringWidth(font(8.8f, true, 0.14f), name) + 16.0f;
        const juce::Rectangle<float> tab(m.x + 12.0f, m.y - 7.5f, width, 15.0f);
        g.fillPath(framePath(tab, 3.0f));
        g.setColour(colour::tabInk);
        g.setFont(font(8.8f, true, 0.14f));
        g.drawText(name, tab, juce::Justification::centred, false);
        if (skin().bolts)
            for (const auto corner :
                 {frame.getTopRight().translated(-9.0f, 9.0f), frame.getBottomLeft().translated(9.0f, -9.0f),
                  frame.getBottomRight().translated(-9.0f, -9.0f)})
                bolt(corner, 2.6f);
    }

    for (const auto& knob : layout::kKnobs)
        drawLabel(g, knob.label, knob.x, labelY(knob), knob.radius >= 9.0f ? 7.6f : 7.0f,
                  knob.radius >= 9.0f ? skin().ink : skin().sub);

    // Séquenceur
    for (std::size_t lane = 0; lane < 2; ++lane)
    {
        const auto y = layout::kSeqRowY[lane];
        drawLabel(g, "SEQ " + juce::String(lane + 1), 40.0f, y + 2.5f, 8.6f, skin().ink, true,
                  juce::Justification::centredLeft);
        for (int step = 0; step < 8; ++step)
            drawLabel(g, juce::String(step + 1),
                      layout::kLedX0 + static_cast<float>(step) * layout::kLedSpacing, y - 12.0f, 6.4f,
                      skin().sub);
    }
    const auto buttonLabel = [&g](const char* text, float x, float y)
    { drawLabel(g, text, x, y - layout::kButtonSize / 2.0f - 5.0f, 7.0f, skin().ink); };
    for (const auto& button : layout::kComboButtons)
        buttonLabel(button.label, button.x, button.y);
    buttonLabel("RUN / STOP", kRunX, layout::kTransportY);
    buttonLabel("CHAIN", kChainX, layout::kTransportY);
    buttonLabel("SHIFT", kShiftX, layout::kTransportY);
    buttonLabel("HOST SYNC", kSyncX, layout::kTransportY);
    drawLabel(g, "QUANTIZER", static_cast<float>(kDisplayArea.getCentreX()), 131.0f, 7.0f, skin().ink);

    // Routage
    drawLabel(g, "ORDER", 550.0f, 262.0f, 7.6f, skin().ink);
    drawLabel(g, "VCW", 524.0f, 408.0f, 7.0f, skin().sub);
    drawLabel(g, "VCF", 576.0f, 408.0f, 7.0f, skin().sub);

    // Patch bay : pointillé entre les entrées et les sorties
    juce::Path separator;
    separator.addLineSegment({776.0f, 278.5f, 920.0f, 278.5f}, 0.0f);
    juce::Path dashed;
    const std::array<float, 2> dashes{2.0f, 3.0f};
    juce::PathStrokeType(0.6f).createDashedStroke(dashed, separator, dashes.data(), 2);
    g.setColour(skin().ink.withAlpha(0.5f));
    g.fillPath(dashed);
}

// --- Panneau ---------------------------------------------------------------------------------------------

Panel::Panel(DedaleProcessor& processorToControl, juce::Component& popupParent)
    : processor(processorToControl), leds(processorToControl),
      order(*processorToControl.apvts.getParameter("order")), scope(processorToControl),
      patchBay(processorToControl), cables(processorToControl, patchBay)
{
    setSize(kDesignWidth, kDesignHeight);
    background.setInterceptsMouseClicks(false, false);
    background.setBufferedToImage(true);
    background.setBounds(getLocalBounds());
    addAndMakeVisible(background);

    for (const auto& spec : layout::kKnobs)
    {
        auto& knob = *knobs.emplace_back(
            std::make_unique<Knob>(processor.apvts, spec.parameter, spec.radius, spec.tooltip, popupParent));
        const auto side = 2.0f * (spec.radius + (spec.radius >= 9.0f ? 7.0f : 5.0f));
        knob.setBounds(juce::Rectangle<float>(side, side).withCentre({spec.x, spec.y}).toNearestInt());
        addAndMakeVisible(knob);
    }

    for (const auto& spec : layout::kComboButtons)
    {
        auto& button = addButton(comboButtons[static_cast<std::size_t>(spec.button)], spec.tooltip, spec.x,
                                 spec.y, layout::kButtonSize);
        button.onStateChange = [this, id = spec.button, &button]
        {
            auto& wasDown = comboDown[static_cast<std::size_t>(id)];
            if (button.isDown() == wasDown)
                return;
            wasDown = button.isDown();
            wasDown ? comboPressed(id) : comboReleased(id);
        };
    }

    const auto toggle = [this](std::unique_ptr<SquareButton>& slot, const char* parameter,
                               const char* tooltip, float x, float width)
    {
        auto& button = addButton(slot, tooltip, x, layout::kTransportY, width);
        button.setClickingTogglesState(true);
        if (parameter != nullptr)
            attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                processor.apvts, parameter, button));
    };
    toggle(runButton, "run", "Démarre ou arrête les deux séquenceurs, sans retour au pas 1.", kRunX, 30.0f);
    toggle(chainButton, "chain", "Enchaîne SEQ1 puis SEQ2 en une seule séquence de 16 pas.", kChainX,
           layout::kButtonSize);
    toggle(syncButton, "sync_mode",
           "Allumé : l'horloge suit le tempo de la DAW. Éteint : horloge interne (TEMPO).", kSyncX,
           layout::kButtonSize);
    toggle(shiftButton, nullptr,
           "Pour les combinaisons à la souris : allumer SHIFT, cliquer BIT SHIFT, LENGTH ou BUFFER (ils "
           "restent tenus), puis l'autre bouton.",
           kShiftX, layout::kButtonSize);
    shiftButton->onClick = [this]
    {
        if (!shiftButton->getToggleState())
        {
            handler.releaseAll();
            advanceGrace = 0;
        }
    };

    leds.setInterceptsMouseClicks(false, false);
    leds.setBounds(70, 80, 210, 128);
    display.setBounds(kDisplayArea);
    order.setBounds(kOrderArea);
    scope.setBounds(kScopeArea);
    patchBay.setBounds(764, 60, 168, 366);
    cables.setBounds(getLocalBounds());
    for (auto* component :
         std::initializer_list<juce::Component*>{&leds, &display, &order, &scope, &patchBay, &cables})
        addAndMakeVisible(component);

    refresh();
    startTimerHz(30);
}

Panel::~Panel()
{
    stopTimer();
}

SquareButton& Panel::addButton(std::unique_ptr<SquareButton>& slot, const char* tooltip, float x, float y,
                               float width)
{
    slot = std::make_unique<SquareButton>(juce::String::fromUTF8(tooltip));
    slot->setBounds(
        juce::Rectangle<float>(width + 6.0f, layout::kButtonSize + 6.0f).withCentre({x, y}).toNearestInt());
    addAndMakeVisible(*slot);
    return *slot;
}

void Panel::perform(const ActionList& actions)
{
    for (const auto& action : actions)
        processor.perform(action);
}

void Panel::comboPressed(PanelButton button)
{
    if (!shiftButton->getToggleState())
        return perform(handler.press(button));

    // Mode SHIFT : BIT SHIFT, LENGTH et BUFFER restent tenus jusqu'au clic suivant ; les autres sont des
    // appuis brefs.
    if (handler.isHeld(button))
    {
        perform(handler.release(button));
        if (button != PanelButton::advance)
            return;
    }
    perform(handler.press(button));
    if (button == PanelButton::advance)
        advanceGrace = 18; // 0,6 s : le temps de cliquer RESET pour la combinaison à trois boutons
    else if (!latches(button) && !(button == PanelButton::reset && handler.isHeld(PanelButton::buffer)))
        perform(handler.release(button));
}

void Panel::comboReleased(PanelButton button)
{
    if (!shiftButton->getToggleState())
        perform(handler.release(button));
}

void Panel::refresh()
{
    const auto longPress = handler.advanceTime(1.0 / 30.0);
    perform(longPress);
    if (longPress.count > 0 && shiftButton->getToggleState())
    {
        // Sauvegarde ou effacement faits : en mode SHIFT, on relâche BUFFER (et RESET) pour l'utilisateur.
        handler.release(PanelButton::reset);
        handler.release(PanelButton::buffer);
    }
    if (advanceGrace > 0 && --advanceGrace == 0)
        perform(handler.release(PanelButton::advance));

    const auto mode = handler.displayMode();
    const auto division = juce::roundToInt(processor.apvts.getRawParameterValue("clock_div")->load());
    leds.refresh(mode, division);
    const auto quantizer = processor.display.quantMode.load();
    display.show(mode == ButtonComboHandler::DisplayMode::clockDivision
                     ? "DIV  " + juce::String(division).paddedLeft('0', 2)
                     : juce::String(quantizer + 1).paddedLeft('0', 2) + "  " + quantizerName(quantizer));

    for (std::size_t i = 0; i < comboButtons.size(); ++i)
        comboButtons[i]->setLit(handler.isHeld(static_cast<PanelButton>(i)));
    scope.refresh();
    cables.refresh();
}
} // namespace dedale::ui
