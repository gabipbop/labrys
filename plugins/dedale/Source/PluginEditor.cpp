#include "PluginEditor.h"

namespace
{
constexpr int kDefaultWidth = 1200; // 125 %
const juce::Identifier kWidthProperty{"ui_width"};
} // namespace

DedaleEditor::DedaleEditor(DedaleProcessor& processor)
    : AudioProcessorEditor(processor), dedale(processor), panel(processor, *this)
{
    using namespace dedale::ui;
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(panel);

    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(kDesignWidth) / kDesignHeight);
    setResizeLimits(kDesignWidth * 3 / 4, kDesignHeight * 3 / 4, kDesignWidth * 2, kDesignHeight * 2);
    const auto width =
        juce::jlimit(kDesignWidth * 3 / 4, kDesignWidth * 2,
                     static_cast<int>(dedale.apvts.state.getProperty(kWidthProperty, kDefaultWidth)));
    setSize(width, width * kDesignHeight / kDesignWidth);
}

DedaleEditor::~DedaleEditor()
{
    setLookAndFeel(nullptr);
}

void DedaleEditor::paint(juce::Graphics& g)
{
    g.fillAll(dedale::ui::colour::panelBottom);
}

void DedaleEditor::resized()
{
    const auto scale = static_cast<float>(getWidth()) / static_cast<float>(dedale::ui::kDesignWidth);
    panel.setTransform(juce::AffineTransform::scale(scale));
    panel.setTopLeftPosition(0, 0);
    dedale.apvts.state.setProperty(kWidthProperty, getWidth(), nullptr); // taille retrouvée à la réouverture
}
