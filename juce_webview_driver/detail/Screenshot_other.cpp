/*
  ==============================================================================
    Screenshot_other.cpp  (module: juce_webview_driver)

    Unsupported-platform window capture. Stubbed for now.
    TODO (Linux):   capture + crop via XComposite (redirected pixmap); WebKitGTK's
                    get_snapshot() hits the same black-WebGL ceiling, so it is not
                    the path to take.
  ==============================================================================
*/

#include "Screenshot.h"

#if WEBVIEW_DRIVER_ENABLED

namespace webview_driver::detail
{

void captureWindowAsync (juce::Component& comp,
                         juce::File target,
                         juce::Rectangle<int> viewportCrop,
                         std::function<void (bool, juce::File, juce::String)> done)
{
    juce::ignoreUnused (comp, viewportCrop);
    done (false, target,
          "native screenshot is implemented on macOS and Windows; this platform "
          "is unsupported (Linux XComposite region crop is a TODO)");
}

void captureStreamAsync (juce::Component& comp,
                         juce::File dir,
                         int fps,
                         int durationMs,
                         juce::Rectangle<int> viewportCrop,
                         std::function<void (juce::String, double, int, int)> onFrame,
                         std::function<void (bool, int, juce::String)> onDone,
                         std::function<bool()> shouldStop)
{
    juce::ignoreUnused (comp, dir, fps, durationMs, viewportCrop, onFrame, shouldStop);
    onDone (false, 0, "frame-rate capture is not implemented on this platform");
}

bool streamCaptureOsSupported() { return true; }

void waitForCaptureWorkers (int) {}

} // namespace webview_driver::detail

#endif // WEBVIEW_DRIVER_ENABLED
