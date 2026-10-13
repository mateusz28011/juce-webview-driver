# Demo plugin: a JUCE WebView UI under test

A minimal JUCE 9 plugin (Standalone, VST3, AU) whose whole UI is a WebView: one
**Gain** slider bound to a real `AudioProcessorValueTreeState` parameter, and a
WebGL meter. The Debug build embeds `juce_webview_driver`, and
[`test/demo.test.mjs`](test/demo.test.mjs) drives the running app:

1. moves the slider in the WebView and checks the parameter **in C++**;
2. changes the parameter **from C++** and checks the WebView follows;
3. takes a native screenshot of the WebGL meter.

Everything runs against the live plugin, not a copy of the frontend in a browser.

## Run it

Requirements: CMake ≥ 3.22, a C++17 compiler, Node ≥ 22. JUCE 9.0.4 is fetched on
first configure (pass `-DDEMO_JUCE_DIR=/path/to/JUCE` to reuse a checkout).

```bash
cmake -S examples/demo-plugin -B build/demo
cmake --build build/demo --target WebViewDriverDemo_Standalone
```

Start the Standalone, then run the test from the repo root:

```bash
open "build/demo/WebViewDriverDemo_artefacts/Debug/Standalone/WebView Driver Demo.app"   # macOS
node --test examples/demo-plugin/test/demo.test.mjs
```

On Windows the app is `build\demo\WebViewDriverDemo_artefacts\Debug\Standalone\WebView Driver Demo.exe`.

### macOS: screenshots need Screen Recording

The screenshot test needs the app to hold Screen Recording permission. Without it
the test fails with `-3801`; the first two tests don't need it. To make the grant
survive rebuilds, sign the build with a stable self-signed identity; the CMake file
does it after every build when you name one:

```bash
cmake -S examples/demo-plugin -B build/demo -DLOCAL_CODESIGN_IDENTITY="My Local Codesign"
```

Then approve **WebView Driver Demo** once in System Settings → Privacy & Security →
Screen Recording and relaunch it. Creating the certificate and the full checklist
are in [`docs/screen-recording.md`](../../docs/screen-recording.md).

### Windows: WebView2 SDK

`NEEDS_WEBVIEW2` links WebView2 statically, so JUCE looks for the
`Microsoft.Web.WebView2` NuGet package in
`%userprofile%\AppData\Local\PackageManagement\NuGet\Packages`. Install it once
from PowerShell:

```powershell
Register-PackageSource -provider NuGet -name nugetRepository -location https://www.nuget.org/api/v2
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -Source nugetRepository
```

or point CMake at an existing copy with `-DJUCE_WEBVIEW2_PACKAGE_LOCATION=<dir>`.

## What to copy into your own plugin

[`Source/Plugin.cpp`](Source/Plugin.cpp) marks the three driver calls with
`DRIVER`: `withCapture` on the WebView options, `connect` + `start` once the
WebView exists, and `stop` before it is destroyed. They compile to nothing outside
Debug builds. The rest is an ordinary JUCE WebView plugin:

- `WebSliderRelay` + `WebSliderParameterAttachment` bind the page's slider to the
  parameter, with JUCE's own `getSliderState` on the page side
  ([`ui/index.html`](ui/index.html));
- two native functions, `getGain` and `setGain`, give the test a direct line to
  the C++ state through `page.backend(...)`.
