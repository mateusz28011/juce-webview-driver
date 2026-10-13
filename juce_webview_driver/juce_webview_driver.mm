/*
  ==============================================================================
    juce_webview_driver — Apple (Objective-C++) compilation unit.

    JUCE compiles this .mm INSTEAD of juce_webview_driver.cpp on Apple platforms
    (see JUCEModuleSupport.cmake), so the macOS screenshot path (ScreenCaptureKit)
    can use Objective-C. Other platforms compile juce_webview_driver.cpp.
  ==============================================================================
*/

#include "juce_webview_driver.h"

#include "detail/WebViewDriver.cpp"
#include "detail/LayerDebug_mac.mm"
#include "detail/Screenshot_mac.mm"
