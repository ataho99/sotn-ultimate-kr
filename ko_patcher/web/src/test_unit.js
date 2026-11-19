// worker.js 의 SHA-1·Adler-32·VCDIFF 디코더 단위 시험 (xdelta3 -S none 출력 대조). node test_unit.js <xdelta3 경로>
const fs = require('fs'), cp = require('child_process'), crypto = require('crypto'), os = require('os'), path = require('path');
const W = require('./worker.js');
const XD = process.argv[2] || 'xdelta3';
let fail = 0;
function ok(c, m) { if (!c) { fail++; console.log('FAIL', m); } else console.log('ok  ', m); }

// SHA-1
for (const n of [0, 1, 55, 56, 63, 64, 65, 119, 120, 1000, 123457]) {
  const b = crypto.randomBytes(n), h = new W.Sha1();
  const cut = Math.floor(n / 3); h.update(b.subarray(0, cut)); h.update(b.subarray(cut));
  ok(h.hex() === crypto.createHash('sha1').update(b).digest('hex'), 'sha1 len ' + n);
}
// 큰 입력 속도
{ const b = crypto.randomBytes(64 << 20), h = new W.Sha1(), t = Date.now(); h.update(b);
  const hx = h.hex(); console.log('sha1 64MB', Date.now() - t, 'ms'); ok(hx === crypto.createHash('sha1').update(b).digest('hex'), 'sha1 64MB'); }
// adler32
{ const b = Buffer.from('Wikipedia'); ok(W.adler32(b, b.length) === 0x11E60398, 'adler32 Wikipedia'); }

function MemHandle(buf) { this.b = buf; }
MemHandle.prototype.read = function (v, o) { const n = Math.min(v.length, this.b.length - o.at); if (n <= 0) return 0; v.set(this.b.subarray(o.at, o.at + n)); return n; };
MemHandle.prototype.write = function (v, o) { if (o.at + v.length > this.b.length) { const nb = new Uint8Array(o.at + v.length); nb.set(this.b); this.b = nb; } this.b.set(v, o.at); return v.length; };

function mk(sizeA, seed) {
  // 소스 a: 의사난수 + 반복 구간 / 목표 b: a 의 조각들 + 삽입 + 런 + 이동
  let s = seed >>> 0; const rnd = () => (s = (s * 1664525 + 1013904223) >>> 0);
  const a = Buffer.alloc(sizeA); for (let i = 0; i < sizeA; i++) a[i] = (rnd() >>> 24);
  for (let i = 0; i < sizeA / 8; i++) a[(rnd() % sizeA)] = 0;
  const parts = [];
  let pos = 0;
  while (pos < sizeA * 1.2) {
    const k = rnd() % 6, len = 1 + (rnd() % 3000);
    if (k === 0) { parts.push(Buffer.alloc(len, rnd() & 255)); }            // 런
    else if (k === 1) { const x = Buffer.alloc(len); for (let i = 0; i < len; i++) x[i] = rnd() >>> 24; parts.push(x); } // 새 데이터
    else if (k === 2) { const st = rnd() % (sizeA - 4000); parts.push(a.subarray(st, st + len)); }   // 소스 복사
    else if (k === 3) { const st = rnd() % (sizeA - 4000); const x = Buffer.from(a.subarray(st, st + len)); x[rnd() % len] ^= 1; parts.push(x); } // 한 바이트 다른 복사
    else if (k === 4) { const t = Buffer.concat(parts); if (t.length > 100) { const st = rnd() % (t.length - 50); parts.push(t.subarray(st, st + 50)); } } // 목표 내 복사
    else { parts.push(Buffer.from('ABCABCABCABCABCABCABCABC'.repeat(1 + len % 9))); }
    pos += len;
  }
  return [a, Buffer.concat(parts)];
}

const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'vcd_'));
for (const [sa, seed, B] of [[300000, 1, '8388608'], [3000000, 7, '8388608'], [20000000, 11, '2097152']]) {
  const [a, b] = mk(sa, seed);
  const pa = path.join(tmp, 'a'), pb = path.join(tmp, 'b'), pd = path.join(tmp, 'd');
  fs.writeFileSync(pa, a); fs.writeFileSync(pb, b);
  cp.execFileSync(XD, ['-e', '-f', '-S', 'none', '-B', B, '-s', pa, pb, pd]);
  const d = fs.readFileSync(pd);
  const out = new MemHandle(new Uint8Array(0));
  let err = null, n = 0;
  try { n = W.applyVcdiff(new Uint8Array(d), new MemHandle(new Uint8Array(a)), out); } catch (e) { err = e; }
  ok(!err && n === b.length && Buffer.compare(Buffer.from(out.b), b) === 0, 'vcdiff src ' + sa + ' → tgt ' + b.length + ' delta ' + d.length + (err ? ' ' + err.message : ''));
}
fs.rmSync(tmp, { recursive: true, force: true });
console.log(fail ? 'UNIT FAIL ' + fail : 'UNIT PASS');
process.exit(fail ? 1 : 0);
