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

    // Flancs noirs vissés
    for (const auto x : {0.0f, bounds.getWidth() - 18.0f})
    {
        juce::ColourGradient metal(colour::railDark, x, 0.0f, colour::railDark.brighter(0.1f), x + 18.0f,
                                   0.0f, false);
        metal.addColour(0.45, colour::railLight);
        g.setGradientFill(metal);
        g.fillRect(x, 0.0f, 18.0f, bounds.getHeight());
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRect(x == 0.0f ? 17.0f : x, 0.0f, 1.0f, bounds.getHeight());
        for (const auto y : {12.0f, bounds.getHeight() - 12.0f})
            bolt({x + 9.0f, y}, 4.2f);
    }

    g.setColour(colour::ink);
    g.setFont(font(28.0f, true, 0.2f));
    g.drawText(juce::String::fromUTF8(LABRYS_DISPLAY_NAME),
               juce::Rectangle<float>(34.0f, 8.0f, 220.0f, 26.0f), juce::Justification::centredLeft);
    drawLabel(g, juce::String::fromUTF8("SYNTHÉTISEUR GÉNÉRATIF"), 172.0f, 28.0f, 8.4f, colour::sub, false,
              juce::Justification::centredLeft);
    drawLabel(g, "LABRYS", 926.0f, 28.0f, 9.4f, colour::sub, true, juce::Justification::centredRight);
    drawLabel(g, "RAW POWA STUDIO", 926.0f, 439.0f, 6.4f, colour::sub, false,
              juce::Justification::centredRight);

    // Modules : cadre fin arrondi, nom dans un cartouche plein, vis aux coins
    for (const auto& m : layout::kModules)
    {
        const juce::Rectangle<float> frame(m.x, m.y, m.w, m.h);
        g.setColour(colour::ink);
        g.drawRoundedRectangle(frame, 7.0f, 1.1f);
        const auto name = juce::String::fromUTF8(m.name);
        const auto width = juce::GlyphArrangement::getStringWidth(font(8.8f, true, 0.14f), name) + 16.0f;
        const juce::Rectangle<float> tab(m.x + 12.0f, m.y - 7.5f, width, 15.0f);
        g.fillRoundedRectangle(tab, 3.0f);
        g.setColour(colour::tabInk);
        g.setFont(font(8.8f, true, 0.14f));
        g.drawText(name, tab, juce::Justification::centred, false);
        for (const auto corner :
             {frame.getTopRight().translated(-9.0f, 9.0f), frame.getBottomLeft().translated(9.0f, -9.0f),
              frame.getBottomRight().translated(-9.0f, -9.0f)})
            bolt(corner, 2.6f);
    }

    for (const auto& knob : layout::kKnobs)
        drawLabel(g, knob.label, knob.x, labelY(knob), knob.radius >= 9.0f ? 7.6f : 7.0f,
                  knob.radius >= 9.0f ? colour::ink : colour::sub);

    // Séquenceur
    for (std::size_t lane = 0; lane < 2; ++lane)
    {
        const auto y = layout::kSeqRowY[lane];
        drawLabel(g, "SEQ " + juce::String(lane + 1), 40.0f, y + 2.5f, 8.6f, colour::ink, true,
                  juce::Justification::centredLeft);
        for (int step = 0; step < 8; ++step)
            drawLabel(g, juce::String(step + 1),
                      layout::kLedX0 + static_cast<float>(step) * layout::kLedSpacing, y - 12.0f, 6.4f,
                      colour::sub);
    }
    const auto buttonLabel = [&g](const char* text, float x, float y)
    { drawLabel(g, text, x, y - layout::kButtonSize / 2.0f - 5.0f, 7.0f, colour::ink); };
    for (const auto& button : layout::kComboButtons)
        buttonLabel(button.label, button.x, button.y);
    buttonLabel("RUN / STOP", kRunX, layout::kTransportY);
    buttonLabel("CHAIN", kChainX, layout::kTransportY);
    buttonLabel("SHIFT", kShiftX, layout::kTransportY);
    buttonLabel("HOST SYNC", kSyncX, layout::kTransportY);
    drawLabel(g, "QUANTIZER", static_cast<float>(kDisplayArea.getCentreX()), 131.0f, 7.0f, colour::ink);

    // Routage
    drawLabel(g, "ORDER", 550.0f, 262.0f, 7.6f, colour::ink);
    drawLabel(g, "VCW", 524.0f, 408.0f, 7.0f, colour::sub);
    drawLabel(g, "VCF", 576.0f, 408.0f, 7.0f, colour::sub);

    // Patch bay : pointillé entre les entrées et les sorties
    juce::Path separator;
    separator.addLineSegment({776.0f, 278.5f, 920.0f, 278.5f}, 0.0f);
    juce::Path dashed;
    const std::array<float, 2> dashes{2.0f, 3.0f};
    juce::PathStrokeType(0.6f).createDashedStroke(dashed, separator, dashes.data(), 2);
    g.setColour(colour::ink.withAlpha(0.5f));
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

    // En-tête : presets et réglages globaux
    previousPreset.setBounds(396, 12, 18, 18);
    presetDisplay.setBounds(418, 10, 230, 22);
    nextPreset.setBounds(652, 12, 18, 18);
    settingsButton.setBounds(690, 11, 20, 20);
    presetDisplay.setTooltip(
        juce::String::fromUTF8("Preset en cours. Cliquer pour choisir, enregistrer ou supprimer un preset."));
    previousPreset.onClick = [this] { processor.presets.step(-1); };
    nextPreset.onClick = [this] { processor.presets.step(1); };
    presetDisplay.onClick = [this] { showPresetMenu(); };
    settingsButton.onClick = [this] { showSettingsMenu(); };
    for (auto* component : std::initializer_list<juce::Component*>{&previousPreset, &presetDisplay,
                                                                   &nextPreset, &settingsButton})
        addAndMakeVisible(component);

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

// --- Menus de l'en-tête -----------------------------------------------------------------------------------

void Panel::showPresetMenu()
{
    auto& presets = processor.presets;
    const auto users = presets.userNames();
    const auto current = presets.currentName();
    constexpr int kUserBase = 1000, kSave = 5000, kDelete = 5001;

    juce::PopupMenu menu;
    menu.addSectionHeader(juce::String::fromUTF8("Presets d'usine"));
    for (int i = 0; i < presets.numFactoryPresets(); ++i)
        menu.addItem(1 + i, presets.factoryName(i), true, presets.factoryName(i) == current);
    if (!users.isEmpty())
    {
        menu.addSectionHeader("Mes presets");
        for (int i = 0; i < users.size(); ++i)
            menu.addItem(kUserBase + i, users[i], true, users[i] == current);
    }
    menu.addSeparator();
    menu.addItem(kSave, "Enregistrer sous...");
    menu.addItem(kDelete, "Supprimer \"" + current + "\"", users.contains(current));

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetDisplay),
                       [safe = juce::Component::SafePointer<Panel>(this), users, current](int choice)
                       {
                           if (safe == nullptr || choice == 0)
                               return;
                           auto& manager = safe->processor.presets;
                           if (choice == kSave)
                               safe->askPresetName();
                           else if (choice == kDelete)
                               manager.deleteUser(current);
                           else if (choice >= kUserBase)
                               manager.loadUser(users[choice - kUserBase]);
                           else
                               manager.loadFactory(choice - 1);
                       });
}

void Panel::askPresetName()
{
    auto* window = new juce::AlertWindow("Enregistrer le preset",
                                         "Nom du preset :", juce::MessageBoxIconType::NoIcon, this);
    window->addTextEditor("name", processor.presets.currentName());
    window->addButton("Enregistrer", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Annuler", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    window->enterModalState(true,
                            juce::ModalCallbackFunction::create(
                                [safe = juce::Component::SafePointer<Panel>(this), window](int result)
                                {
                                    if (safe != nullptr && result == 1)
                                        safe->processor.presets.saveUser(
                                            window->getTextEditorContents("name"));
                                }),
                            true);
}

void Panel::showSettingsMenu()
{
    const auto s = processor.settings();
    juce::PopupMenu quality;
    quality.addItem(12, juce::String::fromUTF8("×2 (économe)"), true, s.oversampling == 2);
    quality.addItem(14, juce::String::fromUTF8("×4 (normal)"), true, s.oversampling == 4);
    quality.addItem(18, juce::String::fromUTF8("×8 (haute qualité)"), true, s.oversampling == 8);

    juce::PopupMenu menu;
    menu.addSectionHeader(juce::String::fromUTF8("Réglages globaux"));
    menu.addItem(1, "Le jack RESET rappelle aussi le buffer", true, s.resetRecallsBuffer);
    menu.addItem(2, juce::String::fromUTF8("CV des séquenceurs unipolaires (0 à +5 V)"), true, s.unipolarCv);
    menu.addItem(3, juce::String::fromUTF8("Drift analogique des oscillateurs"), true, s.analogDrift);
    menu.addItem(4, juce::String::fromUTF8("Coloration du wavefolder à FOLD = 0"), true, s.vcwZeroColour);
    menu.addSubMenu(juce::String::fromUTF8("Suréchantillonnage"), quality);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton),
                       [safe = juce::Component::SafePointer<Panel>(this)](int choice)
                       {
                           if (safe == nullptr || choice == 0)
                               return;
                           auto settings = safe->processor.settings();
                           if (choice == 1)
                               settings.resetRecallsBuffer = !settings.resetRecallsBuffer;
                           else if (choice == 2)
                               settings.unipolarCv = !settings.unipolarCv;
                           else if (choice == 3)
                               settings.analogDrift = !settings.analogDrift;
                           else if (choice == 4)
                               settings.vcwZeroColour = !settings.vcwZeroColour;
                           else
                               settings.oversampling = choice - 10;
                           safe->processor.setSettings(settings);
                       });
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

    presetDisplay.show(processor.presets.currentName().toUpperCase());
    for (std::size_t i = 0; i < comboButtons.size(); ++i)
        comboButtons[i]->setLit(handler.isHeld(static_cast<PanelButton>(i)));
    scope.refresh();
    cables.refresh();
}
} // namespace dedale::ui
