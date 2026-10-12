// The page around the WebAssembly design tools. It plays the part of GM.EXE (code/GM/GM.ASM): it runs one program after
// the other, and the program that has just ended says, by its exit code, which one runs next. Every program is its own
// WebAssembly module (gm<name>.js / .wasm, built with -sMODULARIZE -sEXPORT_ES6); they all work on the folder /gm, which is
// kept in IndexedDB, so that what one tool saves is there for the next one and for the next visit.
//
// Everything stays in the browser.
'use strict';

const $ = (id) => document.getElementById(id);

// GM.ASM's table: the exit code of a program is the number of the one to run next; 0 ends GameMaker.
const PROGRAMS = ['', 'gmutility', 'gmpalchos', 'gmblocedit', 'gmmonedit', 'gmmapmaker', 'gmcharedit', 'gmimage', 'gmsndedit',
  'gmgrator', 'gmplayer', 'gmmenu'];
const QUIT = 0, PLAYGAME = 10, MENU = 11;
const TITLES = { gmutility: 'Utilities', gmpalchos: 'Palette designer', gmblocedit: 'Block designer', gmmonedit: 'Monster maker',
  gmmapmaker: 'Map maker', gmcharedit: 'Character maker', gmimage: 'Image reader', gmsndedit: 'Sound designer',
  gmgrator: 'Integrator', gmplayer: 'Player', gmmenu: 'Main menu' };

// ---------------------------------------------------------------------------------------------------------------------
// messages
// ---------------------------------------------------------------------------------------------------------------------
const logLines = [];
function log(text, isError) {
  logLines.push(text);
  if (logLines.length > 300) logLines.shift();
  $('log').textContent = logLines.join('\n');
  if (isError) console.warn(text); else console.log(text);
}
function message(text, isError) {
  $('message').textContent = text;
  $('message').className = isError ? 'error' : '';
}
const show = (id, on) => $(id).classList.toggle('hidden', !on);

// ---------------------------------------------------------------------------------------------------------------------
// zip files (stored or deflated), read with the browser's own DecompressionStream
// ---------------------------------------------------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------------------------------------------------
// the shared folder /gm (IndexedDB) as seen by the program that is running
// ---------------------------------------------------------------------------------------------------------------------
const ROOT = '/gm';
let current = null;          // { name, mod, canvas, persistTimer }
let seedFiles = null;        // the shipped files (gmdata.zip), read once

function mkdirs(FS, path) { try { FS.mkdirTree(path); } catch (e) { /* exists */ } }
function dirname(p) { const i = p.lastIndexOf('/'); return i < 0 ? '' : p.slice(0, i); }

function syncfs(mod, fromStore) {
  return new Promise((resolve) => mod.FS.syncfs(fromStore, (err) => { if (err) log('IndexedDB: ' + err, true); resolve(); }));
}

async function loadSeed() {
  if (seedFiles) return seedFiles;
  const r = await fetch('gmdata.zip');
  if (!r.ok) throw new Error('Could not download gmdata.zip (' + r.status + ')');
  seedFiles = await unzip(await r.arrayBuffer());
  return seedFiles;
}

// The first visit: the shipped help files and the sample game go into the store. Later visits find them there, together
// with whatever the person made.
async function mountStore(mod) {
  const FS = mod.FS;
  mkdirs(FS, ROOT);
  let persistent = true;
  try {
    FS.mount(FS.filesystems.IDBFS, {}, ROOT);
    await syncfs(mod, true);
  } catch (e) {
    persistent = false;
    log('Saving is not available in this browser (' + e.message + '); your work will be lost when the page closes.', true);
  }
  if (!FS.analyzePath(ROOT + '/.seeded').exists) {
    for (const f of await loadSeed()) {
      mkdirs(FS, ROOT + '/' + dirname(f.path));
      FS.writeFile(ROOT + '/' + f.path, f.data);
    }
    FS.writeFile(ROOT + '/.seeded', '1');
    if (persistent) await syncfs(mod, false);
  }
  FS.chdir(ROOT);
  return persistent;
}

function persistNow() {
  if (current && current.persistent) current.mod.FS.syncfs(false, (err) => { if (err) log('IndexedDB: ' + err, true); });
}

// ---------------------------------------------------------------------------------------------------------------------
// running one program
// ---------------------------------------------------------------------------------------------------------------------
function freshCanvas() {
  const stage = $('stage');
  stage.textContent = '';
  const c = document.createElement('canvas');
  c.id = 'canvas';
  c.tabIndex = 0;
  c.addEventListener('contextmenu', (e) => e.preventDefault());
  stage.appendChild(c);
  return c;
}

// resolves with the exit code, or rejects if the module could not start
async function runProgram(name, args) {
  const canvas = freshCanvas();
  let notify;
  const ended = new Promise((resolve) => { notify = resolve; });
  window.gmProgramExited = (code) => notify(code);
  const factory = (await import('./' + name + '.js')).default;
  const mod = await factory({
    noInitialRun: true,
    canvas,
    print: (t) => log(t, false),
    printErr: (t) => log(t, true),
    onAbort: (what) => { log('The program stopped: ' + what, true); notify(-1); },
  });
  const persistent = await mountStore(mod);
  current = { name, mod, canvas, persistent, persistTimer: setInterval(persistNow, 4000) };
  $('program-name').textContent = TITLES[name] || name;
  canvas.focus();
  mod.callMain(args);
  const code = await ended;
  clearInterval(current.persistTimer);
  if (persistent) await syncfs(mod, false);
  current = null;
  canvas.remove();
  return code;
}

// GM.ASM's main loop. `last` is the program that ran before (the menu opens on its entry).
async function gameMaker() {
  show('cover', false);
  show('bar', true);
  let last = PLAYGAME;
  let next = await runChecked(MENU, ['cOoL', String.fromCharCode(65 + last)]);
  while (next > QUIT && next <= MENU) {
    const ran = next;
    const args = ran === MENU ? ['cOoL', String.fromCharCode(65 + last)] : [];
    if (ran === PLAYGAME) {
      next = await runPlayer();
    } else {
      next = await runChecked(ran, args);
    }
    if (ran !== MENU) last = ran;
  }
  finished(next);
}

async function runChecked(prog, args) {
  try {
    const code = await runProgram(PROGRAMS[prog], args);
    return code < 0 ? QUIT - 1 : code;
  } catch (e) {
    log('Could not run ' + PROGRAMS[prog] + ': ' + (e.message || e), true);
    return QUIT - 1;
  }
}

// The player is one more program of this page (gmplayer, the same engine as the player page, built as a module like the
// editors). It runs in the shared folder: its own menu lists the games in it (the sample game, and the ones made or
// imported here), and what a game saves (scores, saved games) stays there. Whatever way it ends, the menu is next.
async function runPlayer() {
  const code = await runChecked(PLAYGAME, []);
  return code < 0 ? code : MENU;
}

function finished(code) {
  $('stage').textContent = '';
  show('bar', false);
  show('cover', true);
  $('cover').querySelector('h1').textContent = 'GameMaker closed';
  $('start').textContent = 'Start again';
  $('start').disabled = false;
  $('start').onclick = () => location.reload();
  message(code < 0 ? 'A program could not start. See "Messages".' : 'Your work is saved in this browser.', code < 0);
  if (document.fullscreenElement) document.exitFullscreen();
}

// ---------------------------------------------------------------------------------------------------------------------
// keyboard: what a browser would act on while a program runs
// ---------------------------------------------------------------------------------------------------------------------
const KEEP_FOR_PROGRAM = new Set(['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space', 'Tab', 'Backspace', 'Enter',
  'PageUp', 'PageDown', 'Home', 'End', 'Insert', 'Delete', 'Escape', 'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'F10']);
window.addEventListener('keydown', (e) => {
  if (!current) return;
  if (e.target && /^(INPUT|TEXTAREA|SELECT|BUTTON)$/.test(e.target.tagName) && e.code !== 'Tab') return;
  if (KEEP_FOR_PROGRAM.has(e.code) || (e.altKey && /^(Arrow|Enter|Key)/.test(e.code))) e.preventDefault();
}, true);
// the program that has ended no longer listens (its window is closed), but a late event must not reach it
window.addEventListener('pagehide', persistNow);
document.addEventListener('visibilitychange', () => { if (document.hidden) persistNow(); });

// ---------------------------------------------------------------------------------------------------------------------
// taking projects out (zip download) and bringing them in (zip file or drop)
// ---------------------------------------------------------------------------------------------------------------------
const crcTable = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; }
  return t;
})();
function crc32(bytes) {
  let c = 0xFFFFFFFF;
  for (let i = 0; i < bytes.length; i++) c = crcTable[(c ^ bytes[i]) & 255] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}

async function deflateRaw(bytes) {
  if (typeof CompressionStream === 'undefined') return null;
  const stream = new Blob([bytes]).stream().pipeThrough(new CompressionStream('deflate-raw'));
  return new Uint8Array(await new Response(stream).arrayBuffer());
}

// files: [{ path, data: Uint8Array }] -> Blob of a zip file (deflated where that is smaller)
async function makeZip(files) {
  const enc = new TextEncoder(), parts = [], central = [];
  const now = new Date();
  const dosTime = (now.getHours() << 11) | (now.getMinutes() << 5) | (now.getSeconds() >> 1);
  const dosDate = ((now.getFullYear() - 1980) << 9) | ((now.getMonth() + 1) << 5) | now.getDate();
  let offset = 0;
  for (const f of files) {
    const name = enc.encode(f.path), crc = crc32(f.data);
    let method = 0, body = f.data;
    const packed = f.data.length > 64 ? await deflateRaw(f.data) : null;
    if (packed && packed.length < f.data.length) { method = 8; body = packed; }
    const head = new DataView(new ArrayBuffer(30));
    head.setUint32(0, 0x04034b50, true); head.setUint16(4, 20, true); head.setUint16(6, 0x0800, true);   // 0x0800: UTF-8 names
    head.setUint16(8, method, true); head.setUint16(10, dosTime, true); head.setUint16(12, dosDate, true);
    head.setUint32(14, crc, true); head.setUint32(18, body.length, true); head.setUint32(22, f.data.length, true);
    head.setUint16(26, name.length, true);
    parts.push(head.buffer, name, body);
    const cd = new DataView(new ArrayBuffer(46));
    cd.setUint32(0, 0x02014b50, true); cd.setUint16(4, 20, true); cd.setUint16(6, 20, true); cd.setUint16(8, 0x0800, true);
    cd.setUint16(10, method, true); cd.setUint16(12, dosTime, true); cd.setUint16(14, dosDate, true);
    cd.setUint32(16, crc, true); cd.setUint32(20, body.length, true); cd.setUint32(24, f.data.length, true);
    cd.setUint16(28, name.length, true); cd.setUint32(42, offset, true);
    central.push(cd.buffer, name);
    offset += 30 + name.length + body.length;
  }
  let csize = 0;
  for (const c of central) csize += c.byteLength;
  const end = new DataView(new ArrayBuffer(22));
  end.setUint32(0, 0x06054b50, true); end.setUint16(8, files.length, true); end.setUint16(10, files.length, true);
  end.setUint32(12, csize, true); end.setUint32(16, offset, true);
  return new Blob([...parts, ...central, end.buffer], { type: 'application/zip' });
}

function sameBytes(a, b) {
  if (a.length !== b.length) return false;
  for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
  return true;
}

// what the person made: every file of /gm that is not one of the shipped ones (unchanged) or the program's own settings
function walkFiles(FS, dir, rel, out) {
  for (const n of FS.readdir(dir)) {
    if (n === '.' || n === '..') continue;
    const full = dir + '/' + n, r = rel ? rel + '/' + n : n;
    if (FS.isDir(FS.stat(full).mode)) walkFiles(FS, full, r, out); else out.push({ path: r, full });
  }
  return out;
}

async function userFiles(FS) {
  const seed = new Map();
  for (const f of await loadSeed()) seed.set(f.path.toLowerCase(), f.data);
  const out = [];
  for (const f of walkFiles(FS, ROOT, '', [])) {
    const key = f.path.toLowerCase();
    if (key === '.seeded' || key === 'gm.cfg') continue;
    const data = FS.readFile(f.full);
    if (seed.has(key) && sameBytes(seed.get(key), data)) continue;
    out.push({ path: f.path, data });
  }
  return out;
}

let barStatusTimer = 0;
function barStatus(text, isError) {
  const s = $('bar-status');
  s.textContent = text;
  s.style.color = isError ? 'var(--danger)' : 'var(--muted)';
  showBar();
  clearTimeout(barStatusTimer);
  barStatusTimer = setTimeout(() => { s.textContent = ''; }, 8000);
}

function formatSize(n) { return n < 1024 ? n + ' bytes' : n < 1048576 ? (n / 1024).toFixed(1) + ' KB' : (n / 1048576).toFixed(1) + ' MB'; }

function closeDialog() {
  $('export-dialog').close();
  if (current) current.canvas.focus();
}

async function openExport() {
  if (!current) return;
  if (document.pointerLockElement) document.exitPointerLock();
  const files = await userFiles(current.mod.FS);
  const groups = new Map();                       // top-level folder -> files; '' = loose files in the main folder
  for (const f of files) {
    const i = f.path.indexOf('/'), g = i < 0 ? '' : f.path.slice(0, i);
    if (!groups.has(g)) groups.set(g, []);
    groups.get(g).push(f);
  }
  const list = $('export-list');
  list.textContent = '';
  const boxes = [];
  for (const [g, fs] of [...groups].sort((a, b) => a[0].localeCompare(b[0]))) {
    const label = document.createElement('label');
    const box = document.createElement('input');
    box.type = 'checkbox'; box.checked = true;
    boxes.push({ box, files: fs, group: g });
    const size = fs.reduce((s, f) => s + f.data.length, 0);
    label.append(box, ' ', g ? g + '/' : 'Files in the main folder', ' (' + fs.length + (fs.length === 1 ? ' file, ' : ' files, ') + formatSize(size) + ')');
    list.appendChild(label);
  }
  const none = boxes.length === 0;
  $('export-empty').classList.toggle('hidden', !none);
  $('export-go').disabled = none;
  $('export-go').onclick = async () => {
    const chosen = boxes.filter((b) => b.box.checked);
    if (!chosen.length) return;
    const all = chosen.flatMap((b) => b.files);
    const name = chosen.length === 1 && chosen[0].group ? chosen[0].group.toLowerCase() : 'gamemaker-projects';
    const blob = await makeZip(all);
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = name + '.zip';
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 60000);
    closeDialog();
    barStatus('Saved ' + name + '.zip (' + all.length + ' files)');
  };
  $('export-dialog').showModal();
}

function safePath(name) {
  const parts = name.replace(/\\/g, '/').split('/').filter((p) => p && p !== '.');
  if (!parts.length || parts.some((p) => p === '..' || /[:*?"<>|\0]/.test(p))) return null;
  return parts.join('/');
}

async function importZip(file) {
  if (!current) return;
  try {
    const entries = (await unzip(await file.arrayBuffer())).map((f) => ({ path: safePath(f.path), data: f.data })).filter((f) => f.path);
    if (!entries.length) { barStatus('That zip file has no files in it', true); return; }
    const FS = current.mod.FS;
    const replaced = entries.filter((f) => FS.analyzePath(ROOT + '/' + f.path).exists).length;
    if (replaced && !window.confirm(replaced + (replaced === 1 ? ' file' : ' files') + ' with the same name will be replaced. Continue?')) return;
    for (const f of entries) {
      mkdirs(FS, ROOT + '/' + dirname(f.path));
      FS.writeFile(ROOT + '/' + f.path, f.data);
    }
    if (current.persistent) await syncfs(current.mod, false);
    barStatus('Added ' + entries.length + (entries.length === 1 ? ' file' : ' files') + ' from ' + file.name + '. Open the file list again to see them.');
  } catch (e) {
    barStatus('Could not read ' + file.name + ': ' + (e.message || e), true);
  }
}

$('btn-export').onclick = (e) => { e.target.blur(); openExport(); };
$('btn-import').onclick = (e) => { e.target.blur(); if (document.pointerLockElement) document.exitPointerLock(); $('import-file').click(); };
$('import-file').onchange = (e) => { const f = e.target.files[0]; e.target.value = ''; if (f) importZip(f); };
$('export-cancel').onclick = closeDialog;
$('export-dialog').addEventListener('keydown', (e) => { e.stopPropagation(); if (e.key === 'Escape') closeDialog(); });   // not the program's keys
$('export-dialog').addEventListener('close', () => { if (current) current.canvas.focus(); });
window.addEventListener('dragover', (e) => { if (current && e.dataTransfer && [...e.dataTransfer.types].includes('Files')) e.preventDefault(); });
window.addEventListener('drop', (e) => {
  if (!current || !e.dataTransfer || !e.dataTransfer.files.length) return;
  e.preventDefault();
  for (const f of e.dataTransfer.files) {
    if (/\.zip$/i.test(f.name)) importZip(f); else barStatus('Only .zip files can be added (not ' + f.name + ')', true);
  }
});

// the bar: shown when the pointer is near the top edge
let hideTimer = 0;
function showBar() {
  $('bar').classList.remove('faded');
  clearTimeout(hideTimer);
  hideTimer = setTimeout(() => $('bar').classList.add('faded'), 3500);
}
window.addEventListener('mousemove', (e) => { if (current && e.clientY < 60) showBar(); });

$('btn-full').onclick = (e) => {
  e.target.blur();
  if (document.fullscreenElement) document.exitFullscreen(); else document.documentElement.requestFullscreen().catch((err) => log('Full screen refused: ' + err.message, true));
};
document.addEventListener('fullscreenchange', () => {
  const on = !!document.fullscreenElement;
  if (navigator.keyboard && navigator.keyboard.lock) {
    if (on) navigator.keyboard.lock(['Escape']).catch(() => {}); else navigator.keyboard.unlock();   // Esc goes to the program
  }
  $('btn-full').textContent = on ? 'Leave full screen' : 'Full screen';
  if (current) current.canvas.focus();
});
// mouse capture: the browser hides its cursor (pointer lock) and the program's own cursor is the only one; the program gets
// the movement (dosplat.c adds it up). The browser gives the mouse back on Esc, which is also this page's way out.
function toggleCapture() {
  if (!current) return;
  if (document.pointerLockElement) document.exitPointerLock();
  else {
    // raw movement first: with the ordinary lock some systems (remote desktops, virtual machines) stop reporting movement
    // when the real pointer reaches the edge of the host screen, and the program's cursor would never reach its own edge
    const c = current.canvas;
    const plain = () => { const r2 = c.requestPointerLock(); if (r2 && r2.catch) r2.catch((err) => log('Mouse capture refused: ' + err.message, true)); };
    let r;
    try { r = c.requestPointerLock({ unadjustedMovement: true }); } catch (err) { plain(); return; }
    if (r && r.catch) r.catch(plain);
  }
}
$('btn-capture').onclick = (e) => { e.target.blur(); toggleCapture(); };
document.addEventListener('pointerlockchange', () => {
  $('btn-capture').textContent = document.pointerLockElement ? 'Release mouse' : 'Capture mouse';
  if (current) current.canvas.focus();
});
window.addEventListener('keydown', (e) => { if (e.altKey && e.code === 'KeyG') { e.preventDefault(); toggleCapture(); } }, true);
$('log-toggle').onclick = () => $('log').classList.toggle('hidden');

// ---------------------------------------------------------------------------------------------------------------------
// start
// ---------------------------------------------------------------------------------------------------------------------
window.addEventListener('error', (e) => log('Error: ' + e.message, true));
(async function init() {
  if (!window.WebAssembly) { message('This browser does not support WebAssembly.', true); return; }
  try {
    await loadSeed();
    message('Ready.');
    $('start').disabled = false;
  } catch (e) {
    message(e.message || String(e), true);
    return;
  }
  $('start').onclick = () => { $('start').disabled = true; gameMaker(); };
  if (new URLSearchParams(location.search).has('autostart')) $('start').click();
})();
