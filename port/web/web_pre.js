// Browser target: runs inside the Emscripten module, before the engine starts (see port/CMakeLists.txt).
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  // Settings (window mode, volume, ...) are kept here; the page backs this folder with IndexedDB (gmplay-web.js).
  ENV['GM_CONFIG_DIR'] = '/persist/settings';
});
// Debugging aid: opening the page with ?wavcap makes the engine also write everything it sends to the audio device to
// /tmp/cap.wav inside the page's file system (read it with Module.FS.readFile).
Module['preRun'].push(function () {
  if (typeof location !== 'undefined' && /[?&]wavcap\b/.test(location.search)) ENV['GM_WAV'] = '/tmp/cap.wav';
});
Module['preRun'].push(function () {
  if (typeof location !== 'undefined' && /[?&]wavcap\b/.test(location.search)) ENV['GM_TIMERLOG'] = '1';   // also prints timer statistics
});
