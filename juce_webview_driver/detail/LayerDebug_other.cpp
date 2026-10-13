/*
  ==============================================================================
    LayerDebug_other.cpp — non-Apple stub. WebView2/other backends have no
    equivalent one-call overlay; report "not available".
  ==============================================================================
*/

#if WEBVIEW_DRIVER_ENABLED

#include "LayerDebug.h"

namespace webview_driver::detail
{

bool setCompositingDebugOverlays (juce::Component*, bool)
{
    return false;
}

std::string getCaLayerTreeAsText (juce::Component*)
{
    return {};
}

} // namespace webview_driver::detail

#endif
