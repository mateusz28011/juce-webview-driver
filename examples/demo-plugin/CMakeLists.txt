#  WebView Driver Demo — a minimal JUCE plugin whose whole UI is a WebView:
#  one Gain slider bound to a real AudioProcessorValueTreeState parameter, plus a
#  WebGL meter. Built in Debug, it embeds juce_webview_driver so the test in
#  test/demo.test.mjs can drive the live UI and check the C++ state.
#
#     cmake -S examples/demo-plugin -B build/demo
#     cmake --build build/demo --target WebViewDriverDemo_Standalone
#
#  JUCE is fetched on first configure. Pass -DDEMO_JUCE_DIR=/path/to/JUCE to reuse
#  a local checkout instead.

cmake_minimum_required(VERSION 3.22)

project(WebViewDriverDemo VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The driver is debug-only; default to a Debug build so it is actually compiled in.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(DEMO_JUCE_DIR "" CACHE PATH "Path to an existing JUCE source tree; empty = fetch")
if(DEMO_JUCE_DIR)
    add_subdirectory("${DEMO_JUCE_DIR}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(juce)
endif()

# This demo lives inside the driver's repo, so it uses the module from this
# checkout. In your own project, use a submodule or FetchContent (see README).
juce_add_module("${CMAKE_CURRENT_LIST_DIR}/../../juce_webview_driver")

set(demo_formats Standalone VST3)
if(APPLE)
    list(APPEND demo_formats AU)
endif()

juce_add_plugin(WebViewDriverDemo
    PRODUCT_NAME              "WebView Driver Demo"
    COMPANY_NAME              "juce-webview-driver"
    BUNDLE_ID                 "io.github.mateusz28011.webviewdriverdemo"
    PLUGIN_MANUFACTURER_CODE  Wvdr
    PLUGIN_CODE               Wvdm
    FORMATS                   ${demo_formats}
    NEEDS_WEB_BROWSER         TRUE
    NEEDS_WEBVIEW2            TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE)

target_sources(WebViewDriverDemo PRIVATE Source/Plugin.cpp)

# The page and JUCE's own frontend interop library, served from memory by the
# WebView's resource provider.
juce_add_binary_data(WebViewDriverDemoUi
    SOURCES
        ui/index.html
        "${JUCE_SOURCE_DIR}/modules/juce_gui_extra/native/typescript/webview-interop/dist/index.js")

target_compile_definitions(WebViewDriverDemo PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# macOS: re-sign the Standalone with a STABLE local identity after each build.
# The default ad-hoc signature changes on every rebuild, and macOS keys the
# Screen Recording grant (needed by native screenshots) to the signature, so the
# grant would be lost after each build. A fixed self-signed Code Signing
# certificate keeps it; approve the app once. See docs/screen-recording.md.
#
#   cmake ... -DLOCAL_CODESIGN_IDENTITY="My Local Codesign"   (or export it)
#
# Unset = no-op, so CI and first-time builds are unaffected.
if(APPLE)
    if(NOT LOCAL_CODESIGN_IDENTITY AND DEFINED ENV{LOCAL_CODESIGN_IDENTITY})
        set(LOCAL_CODESIGN_IDENTITY "$ENV{LOCAL_CODESIGN_IDENTITY}")
    endif()
    if(LOCAL_CODESIGN_IDENTITY)
        add_custom_command(TARGET WebViewDriverDemo_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo
                "[codesign] Re-signing Standalone with '${LOCAL_CODESIGN_IDENTITY}' so the Screen Recording grant survives rebuilds"
            COMMAND codesign --force --deep
                --sign "${LOCAL_CODESIGN_IDENTITY}"
                --identifier "io.github.mateusz28011.webviewdriverdemo"
                "$<TARGET_BUNDLE_DIR:WebViewDriverDemo_Standalone>"
            VERBATIM)
    endif()
endif()

target_link_libraries(WebViewDriverDemo
    PRIVATE
        WebViewDriverDemoUi
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce_webview_driver
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
