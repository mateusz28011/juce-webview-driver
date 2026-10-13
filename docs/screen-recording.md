# macOS Screen Recording permission (and why a stable signature matters)

This page is macOS-only. On Windows 11, `shot` uses Windows.Graphics.Capture and
needs no permission, only a non-minimized window.

The native `shot` capture needs the **host app** to hold **Screen Recording**
permission (System Settings → Privacy & Security → Screen Recording). If it
doesn't, the host reports the real reason, e.g.:

```
SCShareableContent failed: The user declined TCCs … code -3801
```

`-3801` is a permission denial, **not** a bug in the capture code or in your UI.
Note that `ping`, `hello` and `screenshotAvailable: true` only prove that the host
*can* capture. They say nothing about whether this build holds the grant.

## Sign with a stable identity

The trap: macOS keys the grant to the app's **code signature**. An **ad-hoc**
signature (the default for local/Debug builds) gets a *fresh cdhash on every
rebuild*, so the grant evaporates and you're back to `-3801` after each build —
even though you "already allowed it". Fix it by signing the Standalone with a
**stable** self-signed identity so the designated requirement stays constant:

1. Once, create a self-signed Code Signing certificate in the login keychain:
   Keychain Access → Certificate Assistant → Create a Certificate →
   Self-Signed Root, type "Code Signing", e.g. named "My Local Codesign".
2. Re-sign the built app with it after every build. A CMake `POST_BUILD` step
   does this automatically; [`examples/demo-plugin/CMakeLists.txt`](../examples/demo-plugin/CMakeLists.txt)
   has a copyable one driven by `-DLOCAL_CODESIGN_IDENTITY="My Local Codesign"`.
   By hand:

   ```bash
   codesign --force --deep --sign "My Local Codesign" MyApp.app
   codesign --verify --deep --strict MyApp.app
   ```

3. Approve the app once in System Settings → Privacy & Security → Screen
   Recording, then **relaunch it** (ScreenCaptureKit reads the grant at launch).

After approving once, the grant **survives rebuilds** (same cert → same
designated requirement). `security find-identity -p codesigning` may list the
self-signed cert as `CSSMERR_TP_NOT_TRUSTED`, yet `codesign` signs with it fine —
that's expected. Switching from an ad-hoc to a stable signature (or to another
certificate) is a new identity for macOS, so it needs one more approval.

## Before a native capture

1. Quit every running copy of the app, including an installed one that shares
   the bundle ID, so the capture can't hit the wrong process.
2. Build and re-sign; require `codesign --verify --deep --strict` to pass.
3. Launch that exact `.app` through LaunchServices, never its
   `Contents/MacOS/...` binary (the grant is looked up for the bundle):

   ```bash
   /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister -f MyApp.app
   open -n MyApp.app        # add -g to keep it in the background
   ```

4. Make sure the grant exists for **this signed bundle**. If it doesn't, have the
   user enable it in System Settings and relaunch. Don't fire a speculative
   `shot` just to trigger the permission prompt, and don't routinely clear a
   valid grant with `tccutil reset ScreenCapture <bundle-id>`: a stable
   signature exists precisely to keep it. The exception is a stale record: if
   the app keeps failing with `-3801` even after you removed it from the list
   and relaunched, and macOS no longer asks, reset it once with that command,
   relaunch, and approve the prompt.
5. Wait until the page is loaded and `document.visibilityState === 'visible'`
   (see "Backgrounded host pauses the page" in [e2e.md](e2e.md)).

## Without Screen Recording

DOM, computed styles and bounds cover most layout assertions. A 2D canvas can be
exported without native capture through a chunked read:

```js
const dataUrl = await page.readBig(`document.querySelector('canvas')?.toDataURL('image/png')`);
```

That is the canvas buffer only, not the surrounding UI, and it is not a reliable
way to read a WebGL canvas. Native `shot` is.
