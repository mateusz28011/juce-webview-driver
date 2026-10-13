/*
 * demo.test.mjs — end-to-end test of the running WebView Driver Demo.
 *
 * Start the Debug standalone first, then:  node --test examples/demo-plugin/test/demo.test.mjs
 *
 * Every step runs against the real plugin: the page in its WebView, the APVTS
 * parameter in C++, and the window as the OS compositor draws it.
 */
import { test, after } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
// Inside this repo the client is imported from the checkout. In your own project:
//   import { connect, expect } from 'juce-webview-driver';
import { connect, expect } from '../../../tools/e2e.mjs';

const page = await connect({ timeout: 5000, activate: 'WebView Driver Demo' }).catch((e) => {
  throw new Error(`Could not reach the demo. Is the Debug standalone running? (${e.message})`);
});
after(() => page.close());
// The page's module script attaches its handlers asynchronously; wait for it.
await page.waitForFunction(`document.documentElement.dataset.ready === 'true'`);

const near = (expected) => (value) => Math.abs(value - expected) < 0.005;

test('moving the slider in the WebView changes the C++ parameter', async () => {
  await page.getByTestId('gain').fill(0.8);

  await expect.poll(() => page.backend('getGain')).toSatisfy(near(0.8));
  await expect(page.getByTestId('gain-value')).toHaveText('0.80');
});

test('a change made in C++ shows up in the WebView', async () => {
  await page.backend('setGain', 0.25);

  await expect(page.getByTestId('gain')).toHaveValue('0.25');
  await expect(page.getByTestId('gain-value')).toHaveText('0.25');
});

test('a native screenshot captures the WebGL meter', async (t) => {
  if (!(await page.capabilities()).screenshotAvailable)
    return t.skip('native screenshots are not available on this platform');

  // The meter really drew with WebGL (a null context would leave it blank).
  await expect(page.locator('[data-testid=meter][data-rendered=true]')).toHaveCount(1);
  const png = await page.getByTestId('meter').screenshot();

  assert.ok(fs.statSync(png).size > 0, `screenshot written to ${png}`);
  t.diagnostic(`meter screenshot: ${png}`);
});
