// Node target of the web build: make the process environment visible to getenv() (Emscripten starts with its own
// minimal ENV), so that GM_HEADLESS, FDUMP, FTRACE and friends work as they do natively.
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
  if (typeof process !== 'undefined' && process.env) {
    for (var k in process.env) ENV[k] = process.env[k];
  }
});
