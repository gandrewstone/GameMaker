// Browser target of the design tools: runs inside each Emscripten module before the program starts (see cmake/Editors.cmake).
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  // The platform layer keeps its window settings here; the editors do not need them to survive.
  ENV['GM_CONFIG_DIR'] = '/tmp/settings';
});
