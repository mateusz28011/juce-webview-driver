// WebView Driver Demo — one Gain parameter, a WebView UI, and juce_webview_driver.
//
// The parts that matter for the driver are marked "DRIVER". Everything else is a
// plain JUCE WebView plugin: a WebSliderRelay + WebSliderParameterAttachment bind
// the page's slider to the APVTS parameter, and two native functions expose the
// C++ side of that parameter so a test can check it directly.

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_webview_driver/juce_webview_driver.h>   // DRIVER
#include <BinaryData.h>

namespace
{
const juce::ParameterID gainId { "gain", 1 };

std::optional<juce::WebBrowserComponent::Resource> makeResource (const char* data, int size, const char* mime)
{
    const auto* bytes = reinterpret_cast<const std::byte*> (data);
    return juce::WebBrowserComponent::Resource { std::vector<std::byte> (bytes, bytes + size), mime };
}

// Serves the page and JUCE's interop library from BinaryData.
std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html");

    if (url == "/juce/index.js")
        return makeResource (BinaryData::index_js, BinaryData::index_jsSize, "text/javascript");

    return std::nullopt;
}
} // namespace

//==============================================================================
class DemoProcessor final : public juce::AudioProcessor
{
public:
    DemoProcessor()
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    juce::AudioProcessorValueTreeState state { *this, nullptr, "STATE", {
        std::make_unique<juce::AudioParameterFloat> (gainId, "Gain", juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.5f)
    } };

    juce::RangedAudioParameter& gain() { return *state.getParameter (gainId.getParamID()); }

    // Matching mono or stereo in/out, so applyGain only touches channels the host gave us.
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const auto out = layouts.getMainOutputChannelSet();
        return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
            && layouts.getMainInputChannelSet() == out;
    }

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        buffer.applyGain (state.getRawParameterValue (gainId.getParamID())->load());
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        if (auto xml = state.copyState().createXml())
            copyXmlToBinary (*xml, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
            state.replaceState (juce::ValueTree::fromXml (*xml));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};

//==============================================================================
class DemoEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DemoEditor (DemoProcessor& p)
        : AudioProcessorEditor (p), demo (p)
    {
        auto options = juce::WebBrowserComponent::Options {}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2 {}
                                         .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withOptionsFrom (gainRelay)
            // The C++ view of the parameter, so a test can assert on real plugin state.
            .withNativeFunction ("getGain", [this] (const juce::Array<juce::var>&, auto complete)
            {
                complete (demo.gain().convertFrom0to1 (demo.gain().getValue()));
            })
            // Change the parameter from C++ (as a host automation would).
            .withNativeFunction ("setGain", [this] (const juce::Array<juce::var>& args, auto complete)
            {
                auto& param = demo.gain();
                param.setValueNotifyingHost (param.convertTo0to1 ((float) args[0]));
                complete (true);
            })
            .withResourceProvider ([] (const auto& url) { return getResource (url); });

       #if WEBVIEW_DRIVER_ENABLED
        // DRIVER 1/3: fold the capture layer into the options before the WebView exists.
        driver = std::make_shared<webview_driver::WebViewDriver>();
        options = webview_driver::withCapture (std::move (options), driver);
       #endif

        webView = std::make_unique<juce::WebBrowserComponent> (options);
        addAndMakeVisible (*webView);
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

       #if WEBVIEW_DRIVER_ENABLED
        // DRIVER 2/3: wire eval, bounds and screenshots to this WebView and listen
        // on 127.0.0.1 (port 8930, or the next free one).
        webview_driver::connect (*driver, *webView, *this);
        driver->start();
       #endif

        setSize (420, 320);
    }

    ~DemoEditor() override
    {
       #if WEBVIEW_DRIVER_ENABLED
        // DRIVER 3/3: stop before the WebView is destroyed.
        driver->stop();
       #endif
    }

    void resized() override { webView->setBounds (getLocalBounds()); }

private:
    DemoProcessor& demo;

    juce::WebSliderRelay gainRelay { "gain" };
    juce::WebSliderParameterAttachment gainAttachment { demo.gain(), gainRelay, nullptr };

   #if WEBVIEW_DRIVER_ENABLED
    std::shared_ptr<webview_driver::WebViewDriver> driver;
   #endif
    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoEditor)
};

juce::AudioProcessorEditor* DemoProcessor::createEditor() { return new DemoEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DemoProcessor(); }
