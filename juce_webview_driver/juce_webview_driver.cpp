/*
  ==============================================================================
    juce_webview_driver — JUCE module compilation unit.
  ==============================================================================
*/

#include "juce_webview_driver.h"

#include "detail/WebViewDriver.cpp"
#include "detail/LayerDebug_other.cpp"

#if JUCE_WINDOWS
 #include "detail/Screenshot_windows.cpp"
#else
 #include "detail/Screenshot_other.cpp"
#endif
