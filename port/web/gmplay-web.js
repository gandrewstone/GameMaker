// The page around the WebAssembly engine: choosing a game (a bundled pack, a dropped folder or a .zip), putting its files
// into the engine's in-memory file system, keeping scores / saved games / settings in IndexedDB, and the on-page
// controls for what a browser takes away from the keyboard (F11, F12, Alt+..., Esc in full screen).
//
// Everything stays in the browser: dropped files are read locally and nothing is uploaded.
'use strict';
(function () {
  const $ = (id) => document.getElementById(id);
  const params = new URLSearchParams(location.search);

  // ---------------------------------------------------------------------------------------------------------------
  // Emscripten module (gmplay.js is loaded after this object exists; the engine starts only when a game is chosen)
  // ---------------------------------------------------------------------------------------------------------------
  const logLines = [];
  function log(text, isError) {
    logLines.push(text);
    if (logLines.length > 300) logLines.shift();
    $('log').textContent = logLines.join('\n');
    if (isError) console.warn(text); else console.log(text);
  }

  const Module = (window.Module = {
    noInitialRun: true,
    canvas: $('canvas'),
    print: (t) => log(t, false),
    printErr: (t) => log(t, true),
    onExit: (code) => { if (!finished) gameEnded(code); },
    onAbort: (what) => { if (!finished) fatal('The engine stopped: ' + what); },
  });

  let runtimeReady;
  const runtime = new Promise((resolve) => { runtimeReady = resolve; });
  Module.onRuntimeInitialized = () => runtimeReady();

  function loadEngine() {
    const s = document.createElement('script');
    s.src = 'gmplay.js';
    s.onerror = () => fatal('Could not load gmplay.js');
    document.body.appendChild(s);
  }

  // ---------------------------------------------------------------------------------------------------------------
  // zip files (stored or deflated), read with the browser's own DecompressionStream
  // ---------------------------------------------------------------------------------------------------------------
  async function inflateRaw(bytes) {
    if (typeof DecompressionStream === 'undefined') throw new Error('This browser cannot read zip files (no DecompressionStream)');
    const stream = new Blob([bytes]).stream().pipeThrough(new DecompressionStream('deflate-raw'));
    return new Uint8Array(await new Response(stream).arrayBuffer());
  }

  async function unzip(buffer) {
    const v = new DataView(buffer), u8 = new Uint8Array(buffer), dec = new TextDecoder('utf-8');
    let eocd = -1;
    for (let i = u8.length - 22; i >= Math.max(0, u8.length - 65557); i--) {
      if (v.getUint32(i, true) === 0x06054b50) { eocd = i; break; }
    }
    if (eocd < 0) throw new Error('This is not a zip file');
    const count = v.getUint16(eocd + 10, true);
    let p = v.getUint32(eocd + 16, true);
    const files = [];
    for (let n = 0; n < count; n++) {
      if (v.getUint32(p, true) !== 0x02014b50) throw new Error('Damaged zip file');
      const method = v.getUint16(p + 10, true);
      const csize = v.getUint32(p + 20, true);
      const nlen = v.getUint16(p + 28, true), elen = v.getUint16(p + 30, true), clen = v.getUint16(p + 32, true);
      const local = v.getUint32(p + 42, true);
      const name = dec.decode(u8.subarray(p + 46, p + 46 + nlen));
      p += 46 + nlen + elen + clen;
      if (name.endsWith('/')) continue;
      const start = local + 30 + v.getUint16(local + 26, true) + v.getUint16(local + 28, true);
      const raw = u8.subarray(start, start + csize);
      if (method !== 0 && method !== 8) throw new Error('Unsupported zip compression in ' + name);
      files.push({ path: name, data: method === 0 ? raw : await inflateRaw(raw) });
    }
    return files;
  }

  // ---------------------------------------------------------------------------------------------------------------
  // collecting files from a drop or a file picker -> [{path, data}]
  // ---------------------------------------------------------------------------------------------------------------
  function entryFile(entry) { return new Promise((res, rej) => entry.file(res, rej)); }
  function entryBatch(reader) { return new Promise((res, rej) => reader.readEntries(res, rej)); }

  async function walk(entry, prefix, out) {
    if (entry.isFile) {
      out.push({ path: prefix + entry.name, data: new Uint8Array(await (await entryFile(entry)).arrayBuffer()) });
    } else if (entry.isDirectory) {
      const reader = entry.createReader();
      for (let batch = await entryBatch(reader); batch.length; batch = await entryBatch(reader)) {
        for (const child of batch) await walk(child, prefix + entry.name + '/', out);
      }
    }
  }

  async function filesFromList(list) {          // <input type=file>, possibly with webkitdirectory
    const out = [];
    for (const f of list) out.push({ path: f.webkitRelativePath || f.name, data: new Uint8Array(await f.arrayBuffer()) });
    return out;
  }

  async function collect(files) {               // files: [{path,data}]; a single .zip is unpacked
    if (files.length === 1 && /\.zip$/i.test(files[0].path)) return unzip(files[0].data.buffer.slice(files[0].data.byteOffset, files[0].data.byteOffset + files[0].data.byteLength));
    return files;
  }

  // ---------------------------------------------------------------------------------------------------------------
  // the in-memory game folder and the saved state in IndexedDB
  // ---------------------------------------------------------------------------------------------------------------
  const GAME_ROOT = '/game';
  let game = null;            // { id, dir, gam, stamps: Map(file name -> size:mtime when last saved or loaded) }
  let persistReady = false;

  function mkdirs(path) { try { Module.FS.mkdirTree(path); } catch (e) { /* exists */ } }
  function dirname(p) { const i = p.lastIndexOf('/'); return i < 0 ? '' : p.slice(0, i); }
  function basename(p) { return p.slice(p.lastIndexOf('/') + 1); }

  function installGame(files) {
    const FS = Module.FS;
    if (!files.length) throw new Error('No files were found');
    // The game's .gam file: the shallowest one.
    const gams = files.filter((f) => /\.gam$/i.test(f.path)).sort((a, b) => a.path.split('/').length - b.path.split('/').length);
    if (!gams.length) throw new Error('There is no .gam file in what you chose. A GameMaker game is a folder with a .gam file and its pictures, maps and sounds.');
    const gam = gams[0];
    if (gam.data.length < 2 || gam.data[0] !== 0x47 || gam.data[1] !== 0x4d) {
      throw new Error(basename(gam.path) + ' is in the older GameMaker file format (it came with its own player program). This player runs games made with GameMaker 3.0.');
    }
    const base = dirname(gam.path);
    const inside = files.filter((f) => base === '' || f.path === base || f.path.startsWith(base + '/'));
    mkdirs(GAME_ROOT);
    for (const f of inside) {
      const rel = base === '' ? f.path : f.path.slice(base.length + 1);
      if (rel.includes('/')) mkdirs(GAME_ROOT + '/' + dirname(rel));
      FS.writeFile(GAME_ROOT + '/' + rel, f.data);
    }
    const id = basename(gam.path).replace(/\.gam$/i, '').toLowerCase().replace(/[^a-z0-9_-]/g, '_');
    game = { id, dir: GAME_ROOT, gam: basename(gam.path), stamps: new Map(), count: inside.length };
  }

  function syncfs(fromStore) {
    return new Promise((resolve) => Module.FS.syncfs(fromStore, (err) => { if (err) log('IndexedDB: ' + err, true); resolve(); }));
  }

  // What identifies a file's current state: size and modification time.
  function stampOf(path) {
    const st = Module.FS.stat(path);
    return { dir: Module.FS.isDir(st.mode), key: st.size + ':' + (st.mtime instanceof Date ? st.mtime.getTime() : st.mtime) };
  }

  function snapshot() {
    for (const name of Module.FS.readdir(game.dir)) {
      if (name === '.' || name === '..') continue;
      const s = stampOf(game.dir + '/' + name);
      if (!s.dir) game.stamps.set(name, s.key);
    }
  }

  async function mountPersistence() {
    const FS = Module.FS;
    try {
      mkdirs('/persist');
      FS.mount(FS.filesystems.IDBFS, {}, '/persist');
      await syncfs(true);
      persistReady = true;
    } catch (e) {
      log('Saving is not available in this browser (' + e.message + '); scores and settings will not be kept.', true);
      snapshot();
      return;
    }
    mkdirs('/persist/settings');
    mkdirs('/persist/games/' + game.id);
    // put back what an earlier visit saved (scores, saved games, recordings) over the shipped files
    for (const name of FS.readdir('/persist/games/' + game.id)) {
      if (name === '.' || name === '..') continue;
      FS.writeFile(game.dir + '/' + name, FS.readFile('/persist/games/' + game.id + '/' + name));
    }
    snapshot();                       // from here on, a file whose size or time differs from this was changed by the game
  }

  let lastSettings = '';
  function persistNow() {
    if (!persistReady || !game) return;
    const FS = Module.FS;
    let dirty = false;
    for (const name of FS.readdir(game.dir)) {
      if (name === '.' || name === '..') continue;
      const s = stampOf(game.dir + '/' + name);
      if (s.dir || game.stamps.get(name) === s.key) continue;
      FS.writeFile('/persist/games/' + game.id + '/' + name, FS.readFile(game.dir + '/' + name));
      game.stamps.set(name, s.key);
      dirty = true;
    }
    if (FS.readdir('/persist/settings').includes('gmplay.ini')) {   // (a failed lookup is an error path that aborts once the runtime has exited)
      const ini = FS.readFile('/persist/settings/gmplay.ini', { encoding: 'utf8' });
      if (ini !== lastSettings) { lastSettings = ini; dirty = true; }
    }
    if (dirty) FS.syncfs(false, (err) => { if (err) log('IndexedDB: ' + err, true); });
  }

  // ---------------------------------------------------------------------------------------------------------------
  // browser-owned features: full screen (with Esc kept for the game) and the controls bar
  // ---------------------------------------------------------------------------------------------------------------
  function command(cmd, arg) { if (Module._gm_web_command) Module._gm_web_command(cmd, arg | 0); }

  window.gmSetFullscreen = function (on) {
    if (on) {
      document.documentElement.requestFullscreen().catch((e) => log('Full screen refused: ' + e.message, true));
    } else if (document.fullscreenElement) {
      document.exitFullscreen();
    }
  };

  document.addEventListener('fullscreenchange', () => {
    const on = !!document.fullscreenElement;
    // In full screen browsers normally take Esc for themselves; the Keyboard Lock API (Chrome, Edge) gives it to the page.
    if (navigator.keyboard && navigator.keyboard.lock) {
      if (on) navigator.keyboard.lock(['Escape']).catch(() => {}); else navigator.keyboard.unlock();
    }
    command(5, on ? 1 : 0);
    $('btn-full').textContent = on ? 'Leave full screen' : 'Full screen';
  });

  // Keys that the browser would otherwise act on while a game is running.
  const GAME_KEYS = new Set(['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space', 'Tab', 'Backspace', 'Enter',
    'PageUp', 'PageDown', 'Home', 'End', 'Insert', 'Delete', 'F1', 'F2', 'F3', 'F4', 'F6', 'F7', 'F8', 'F9', 'F10']);
  // After the engine has exited its key handlers would call into a finished module; keep the events from reaching them.
  for (const type of ['keydown', 'keyup', 'keypress']) {
    window.addEventListener(type, (e) => { if (finished) e.stopImmediatePropagation(); }, true);
  }
  window.addEventListener('keydown', (e) => {
    if (!running) return;
    if (e.target && /^(INPUT|TEXTAREA|SELECT|BUTTON)$/.test(e.target.tagName) && e.code !== 'Tab') return;
    if (GAME_KEYS.has(e.code) || (e.altKey && /^(Arrow|Enter|Key)/.test(e.code)) || e.code === 'Escape') e.preventDefault();
  }, true);

  let hideTimer = 0;
  function showBar() {
    $('bar').classList.remove('hidden');
    clearTimeout(hideTimer);
    hideTimer = setTimeout(() => { if (running && !$('help').open) $('bar').classList.add('hidden'); }, 3500);
  }
  window.addEventListener('mousemove', (e) => { if (running && e.clientY < 70) showBar(); });
  window.addEventListener('touchstart', () => { if (running) showBar(); }, { passive: true });

  function wireBar() {
    $('btn-full').onclick = (e) => { e.target.blur(); window.gmSetFullscreen(!document.fullscreenElement); };
    $('btn-scale').onclick = (e) => { e.target.blur(); command(3, 0); };
    $('btn-mute').onclick = (e) => { e.target.blur(); muted = !muted; $('btn-mute').textContent = muted ? 'Sound off' : 'Sound on'; command(2, 0); };
    $('vol').oninput = (e) => { muted = false; $('btn-mute').textContent = 'Sound on'; command(1, +e.target.value); };
    $('vol').onchange = (e) => e.target.blur();
    $('btn-help').onclick = (e) => { e.target.blur(); $('help').showModal(); };
    $('btn-quit').onclick = () => { persistNow(); setTimeout(() => location.reload(), 300); };
  }
  let muted = false;
  let running = false;

  // ---------------------------------------------------------------------------------------------------------------
  // start and end of a game
  // ---------------------------------------------------------------------------------------------------------------
  function show(id, on) { $(id).classList.toggle('hidden', !on); }

  function message(text, isError) {
    const m = $('message');
    m.textContent = text;
    m.className = isError ? 'error' : '';
  }

  function fatal(text) {
    show('start', true);
    message(text, true);
    running = false;
    show('bar', false);
  }

  async function startGame(files) {
    try {
      message('Reading the game files...');
      show('picker', false);
      await runtime;
      installGame(await collect(files));
      message('Loading ' + game.gam + '...');
      await mountPersistence();
      applyStoredSettings();
      Module.FS.chdir(game.dir);
      const args = [];
      for (const k of ['scale']) if (params.has(k)) args.push('--' + k + '=' + params.get(k));
      if (params.has('volume')) args.push('--volume=' + params.get('volume'));
      if (params.get('mute') === '1') args.push('--mute');
      args.push(game.gam);
      show('start', false);
      show('bar', true);
      wireBar();
      running = true;
      showBar();
      persistTimer = setInterval(persistNow, 2000);
      window.addEventListener('beforeunload', persistNow);
      document.addEventListener('visibilitychange', () => { if (document.hidden) persistNow(); });
      Module.callMain(args);
    } catch (e) {
      show(window.GM_STANDALONE ? 'standalone' : 'picker', true);
      fatal(e.message || String(e));
    }
  }

  function applyStoredSettings() {
    try {
      const ini = Module.FS.readFile('/persist/settings/gmplay.ini', { encoding: 'utf8' });
      const vol = /volume=(\d+)/.exec(ini), mute = /mute=(\d)/.exec(ini);
      if (vol) $('vol').value = vol[1];
      if (mute && mute[1] === '1') { muted = true; $('btn-mute').textContent = 'Sound off'; }
    } catch (e) { /* first visit */ }
  }

  let finished = false;
  let persistTimer = 0;
  // The engine calls this when it is done (see page_notify in gmplay_main.cpp); the module itself stays alive.
  window.gmEngineExited = (code) => { if (!finished) gameEnded(code); };
  function gameEnded(code) {
    finished = true;
    running = false;
    clearInterval(persistTimer);
    show('bar', false);
    show('end', true);
    $('end-text').textContent = code === 0 ? 'The game ended.' : 'The game stopped (code ' + code + '). See "Messages" on the first page.';
    if (document.fullscreenElement) document.exitFullscreen();
    try { persistNow(); } catch (e) { log('Could not save after the game ended: ' + e.message, true); }
  }

  // ---------------------------------------------------------------------------------------------------------------
  // the start page: bundled games, drop zone, file pickers
  // ---------------------------------------------------------------------------------------------------------------
  async function loadBundled(entry) {
    try {
      message('Downloading ' + entry.name + '...');
      show('picker', false);
      const r = await fetch(entry.zip);
      if (!r.ok) throw new Error('Could not download ' + entry.zip + ' (' + r.status + ')');
      const files = await unzip(await r.arrayBuffer());
      await startGame(files);
    } catch (e) {
      show(window.GM_STANDALONE ? 'standalone' : 'picker', true);
      fatal(e.message || String(e));
    }
  }

  async function listBundled() {
    try {
      const r = await fetch('games.json', { cache: 'no-cache' });
      if (!r.ok) return;
      const list = await r.json();
      const box = $('bundled');
      for (const g of list) {
        const b = document.createElement('button');
        b.className = 'game';
        b.innerHTML = '<strong></strong><span></span>';
        b.firstChild.textContent = g.name;
        b.lastChild.textContent = (g.bytes / 1048576).toFixed(1) + ' MB';
        b.onclick = () => loadBundled(g);
        box.appendChild(b);
        if (params.get('game') === g.id) b.classList.add('wanted');
      }
      show('bundled-box', list.length > 0);
    } catch (e) { /* no bundled games */ }
  }

  function setupDrop() {
    const zone = $('drop');
    ['dragenter', 'dragover'].forEach((t) => window.addEventListener(t, (e) => { e.preventDefault(); zone.classList.add('over'); }));
    ['dragleave', 'drop'].forEach((t) => window.addEventListener(t, (e) => { e.preventDefault(); zone.classList.remove('over'); }));
    window.addEventListener('drop', async (e) => {
      if (running) return;
      const items = [...(e.dataTransfer.items || [])].map((i) => (i.webkitGetAsEntry ? i.webkitGetAsEntry() : null)).filter(Boolean);
      const out = [];
      try {
        if (items.length) {
          for (const entry of items) await walk(entry, '', out);
        } else {
          out.push(...(await filesFromList(e.dataTransfer.files)));
        }
      } catch (err) { fatal(err.message); return; }
      startGame(out);
    });
    $('pick-folder').onchange = async (e) => startGame(await filesFromList(e.target.files));
    $('pick-zip').onchange = async (e) => startGame(await filesFromList(e.target.files));
  }

  // A single-file build (build_web.sh --standalone): the page, the engine and one game in one HTML file, which opens from
  // disk. The build defines window.GM_STANDALONE = { name } and window.GM_GAME_ZIP (bytes) before this script's init() runs
  // its last line, and puts the inlined engine after this script.
  function setupStandalone(sa) {
    document.title = sa.name + ' - GameMaker';
    $('title-h1').textContent = sa.name;
    $('subtitle').textContent = 'A game made with GameMaker (1994)';
    show('bundled-box', false);
    show('picker', false);
    show('standalone', true);
    $('sa-play').onclick = async () => {
      show('standalone', false);
      try {
        const z = window.GM_GAME_ZIP;
        await startGame(await unzip(z.buffer.slice(z.byteOffset, z.byteOffset + z.byteLength)));
      } catch (e) {
        show('standalone', true);
        fatal(e.message || String(e));
      }
    };
  }

  // The single-file build stores binary data (the engine's .wasm, the game's zip) as hex text.
  window.gmHex2Bytes = function (h) {
    const n = h.length >> 1, b = new Uint8Array(n);
    for (let i = 0; i < n; i++) {
      const c1 = h.charCodeAt(2 * i), c2 = h.charCodeAt(2 * i + 1);
      b[i] = (((c1 & 15) + (c1 > 57 ? 9 : 0)) << 4) | ((c2 & 15) + (c2 > 57 ? 9 : 0));
    }
    return b;
  };

  function init() {
    if (!window.WebAssembly) { fatal('This browser does not support WebAssembly.'); return; }
    $('log-toggle').onclick = () => $('log').classList.toggle('hidden');
    $('again').onclick = () => location.reload();
    if (window.GM_STANDALONE) {
      setupStandalone(window.GM_STANDALONE);      // the engine is inlined in the same file
    } else {
      setupDrop();
      listBundled();
      loadEngine();
    }
    window.addEventListener('error', (e) => log('Error: ' + e.message, true));
  }
  init();
})();
