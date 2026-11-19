'use strict';
/* 월하의 야상곡 한글 베타 — 웹 패처 작업기(Worker).
 * 자체 구현(외부 라이브러리 없음): SHA-1, Adler-32, VCDIFF(RFC 3284) 디코더, ISO9660 최소 파서,
 * XA 스테레오→모노, U2c(바탕 이미지) 조립. 라이선스: AGPL-3.0 (프로젝트와 동일).
 * 재료는 이 브라우저 안에서만 읽고, 외부로 전송하지 않는다. */

var SECT = 2352;
var SYNC = [0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0];

function post(o) { self.postMessage(o); }
function log(s, cls) { post({ t: 'log', s: s, cls: cls || '' }); }
function Reject(kind, msg) { this.kind = kind; this.msg = msg; this.message = 'REJECT ' + kind + ': ' + msg; }
Reject.prototype = Object.create(Error.prototype);
function rej(kind, msg) { return new Reject(kind, msg); }

var lastProg = 0;
function prog(phase, done, total, force) {
  var n = Date.now();
  if (!force && n - lastProg < 120) return;
  lastProg = n;
  post({ t: 'prog', phase: phase, done: done, total: total });
}

/* ---------- SHA-1 ---------- */
function Sha1() {
  this.h = new Int32Array([0x67452301, 0xefcdab89 | 0, 0x98badcfe | 0, 0x10325476, 0xc3d2e1f0 | 0]);
  this.buf = new Uint8Array(64); this.bl = 0; this.len = 0; this.w = new Int32Array(80);
}
Sha1.prototype.block = function (u, o) {
  var w = this.w, h = this.h, i, x, t, a, b, c, d, e;
  for (i = 0; i < 16; i++, o += 4) w[i] = (u[o] << 24) | (u[o + 1] << 16) | (u[o + 2] << 8) | u[o + 3];
  for (i = 16; i < 80; i++) { x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]; w[i] = (x << 1) | (x >>> 31); }
  a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
  for (i = 0; i < 20; i++) {
    t = (((a << 5) | (a >>> 27)) + ((b & c) | (~b & d)) + e + 0x5a827999 + w[i]) | 0;
    e = d; d = c; c = (b << 30) | (b >>> 2); b = a; a = t;
  }
  for (; i < 40; i++) {
    t = (((a << 5) | (a >>> 27)) + (b ^ c ^ d) + e + 0x6ed9eba1 + w[i]) | 0;
    e = d; d = c; c = (b << 30) | (b >>> 2); b = a; a = t;
  }
  for (; i < 60; i++) {
    t = (((a << 5) | (a >>> 27)) + ((b & c) | (b & d) | (c & d)) + e + (0x8f1bbcdc | 0) + w[i]) | 0;
    e = d; d = c; c = (b << 30) | (b >>> 2); b = a; a = t;
  }
  for (; i < 80; i++) {
    t = (((a << 5) | (a >>> 27)) + (b ^ c ^ d) + e + (0xca62c1d6 | 0) + w[i]) | 0;
    e = d; d = c; c = (b << 30) | (b >>> 2); b = a; a = t;
  }
  h[0] = (h[0] + a) | 0; h[1] = (h[1] + b) | 0; h[2] = (h[2] + c) | 0; h[3] = (h[3] + d) | 0; h[4] = (h[4] + e) | 0;
};
Sha1.prototype.update = function (u) {
  var i = 0, n = u.length;
  this.len += n;
  if (this.bl > 0) {
    while (i < n && this.bl < 64) this.buf[this.bl++] = u[i++];
    if (this.bl === 64) { this.block(this.buf, 0); this.bl = 0; }
  }
  for (; i + 64 <= n; i += 64) this.block(u, i);
  while (i < n) this.buf[this.bl++] = u[i++];
};
Sha1.prototype.hex = function () {
  var bits = this.len * 8, hi = Math.floor(bits / 4294967296), lo = bits >>> 0;
  var pad = new Uint8Array(((this.bl < 56) ? 56 - this.bl : 120 - this.bl) + 8);
  pad[0] = 0x80;
  var k = pad.length - 8;
  pad[k] = hi >>> 24; pad[k + 1] = hi >>> 16; pad[k + 2] = hi >>> 8; pad[k + 3] = hi;
  pad[k + 4] = lo >>> 24; pad[k + 5] = lo >>> 16; pad[k + 6] = lo >>> 8; pad[k + 7] = lo;
  var keep = this.len; this.update(pad); this.len = keep;
  var s = '';
  for (var i = 0; i < 5; i++) s += ('00000000' + (this.h[i] >>> 0).toString(16)).slice(-8);
  return s;
};

/* ---------- Adler-32 ---------- */
function adler32(u, n) {
  var a = 1, b = 0, i = 0, m;
  while (i < n) {
    m = Math.min(n, i + 5552);
    for (; i < m; i++) { a += u[i]; b += a; }
    a %= 65521; b %= 65521;
  }
  return ((b << 16) | a) >>> 0;
}

/* ---------- 파일 읽기(블록 캐시) ---------- */
function Rd(file, bs) { this.f = file; this.bs = bs; this.base = -1; this.b = null; this.fr = new FileReaderSync(); }
Rd.prototype.read = function (off, len, dst, doff) {
  while (len > 0) {
    var bo = Math.floor(off / this.bs) * this.bs;
    if (bo !== this.base) {
      this.b = new Uint8Array(this.fr.readAsArrayBuffer(this.f.slice(bo, Math.min(bo + this.bs, this.f.size))));
      this.base = bo;
    }
    var s = off - bo, n = Math.min(len, this.b.length - s);
    if (n <= 0) throw new Error('파일 범위 밖 읽기 (' + off + ')');
    dst.set(this.b.subarray(s, s + n), doff);
    off += n; doff += n; len -= n;
  }
};
Rd.prototype.sector = function (lba) { var t = new Uint8Array(SECT); this.read(lba * SECT, SECT, t, 0); return t; };

function hashFile(file, label) {
  var fr = new FileReaderSync(), h = new Sha1(), CH = 8 << 20, off = 0;
  while (off < file.size) {
    var u = new Uint8Array(fr.readAsArrayBuffer(file.slice(off, Math.min(off + CH, file.size))));
    h.update(u); off += u.length;
    prog(label, off, file.size);
  }
  prog(label, file.size, file.size, true);
  return h.hex();
}

function u32(u, o) { return (u[o] | (u[o + 1] << 8) | (u[o + 2] << 16) | (u[o + 3] << 24)) >>> 0; }
function bcd(v) { return ((Math.floor(v / 10)) << 4) | (v % 10); }
function setHead(sec, lba, mode) {
  var f = lba + 150, i;
  for (i = 0; i < 12; i++) sec[i] = SYNC[i];
  sec[12] = bcd(Math.floor(f / 75 / 60)); sec[13] = bcd(Math.floor(f / 75) % 60); sec[14] = bcd(f % 75); sec[15] = mode;
}
function hexBytes(s) { var a = new Uint8Array(8); for (var i = 0; i < 8 && i * 2 < s.length; i++) a[i] = parseInt(s.substr(i * 2, 2), 16); return a; }

/* ---------- ISO9660 (최소) ---------- */
function isoRoot(rd) {
  var b = rd.sector(16), i;
  for (i = 0; i < 12; i++) if (b[i] !== SYNC[i]) return -1;
  if (b[15] !== 2) return -1;
  var u = b.subarray(24, 24 + 2048);
  if (u[0] !== 1 || String.fromCharCode(u[1], u[2], u[3], u[4], u[5]) !== 'CD001') return -1;
  return u32(u, 158);
}
function parseDir(b) {
  var u = b.subarray(24, 24 + 2048), out = [], off = 0;
  while (off < 2048) {
    var ln = u[off];
    if (ln === 0) break;
    var nl = u[off + 32], nm = '';
    for (var j = 0; j < nl; j++) nm += String.fromCharCode(u[off + 33 + j]);
    out.push({ name: nm.split(';')[0].toUpperCase(), lba: u32(u, off + 2), size: u32(u, off + 10), flags: u[off + 25] });
    off += ln;
  }
  return out;
}
function findExtent(rd, parts) {
  var root = isoRoot(rd);
  if (root < 0) return -1;
  var cur = null, i, k, r, list;
  list = parseDir(rd.sector(root));
  for (i = 0; i < list.length; i++) if (list[i].name === parts[0].toUpperCase()) { cur = list[i]; break; }
  if (!cur) return -1;
  for (var pi = 1; pi < parts.length; pi++) {
    if (cur.flags !== 2) return -1;
    var nsec = Math.max(1, Math.floor((cur.size + 2047) / 2048)), got = null;
    for (k = 0; k < nsec && !got; k++) {
      list = parseDir(rd.sector(cur.lba + k));
      for (i = 0; i < list.length; i++) if (list[i].name === parts[pi].toUpperCase()) { got = list[i]; break; }
    }
    if (!got) return -1;
    cur = got;
  }
  if (cur.flags === 2) return -1;
  return cur.lba;
}

/* ---------- 스테레오 → 모노 ---------- */
function monoSectors(payload) {
  var gsz = 128, n = Math.floor(payload.length / gsz), i, j, r;
  var pad = (36 - n % 36) % 36, total = n + pad;
  var zero = new Uint8Array(gsz);
  function grp(k) { return k < n ? payload.subarray(k * gsz, (k + 1) * gsz) : zero; }
  var mg = [];
  function p(a, b) { return (a & 15) | ((b & 15) << 4); }
  for (j = 0; j < Math.floor(total / 2); j++) {
    var g0 = grp(2 * j), g1 = grp(2 * j + 1), out = new Uint8Array(128);
    var lp0 = [g0[4], g0[6], g0[8], g0[10]], lp1 = [g1[4], g1[6], g1[8], g1[10]];
    for (i = 0; i < 4; i++) { out[i] = lp0[i]; out[4 + i] = lp0[i]; out[8 + i] = lp1[i]; out[12 + i] = lp1[i]; }
    for (r = 0; r < 28; r++) {
      var o = 16 + r * 4, d0 = 16 + 4 * r, d1 = 16 + 4 * r;
      out[o] = p(g0[d0], g0[d0 + 1]);
      out[o + 1] = p(g0[d0 + 2], g0[d0 + 3]);
      out[o + 2] = p(g1[d1], g1[d1 + 1]);
      out[o + 3] = p(g1[d1 + 2], g1[d1 + 3]);
    }
    mg.push(out);
  }
  var secs = [];
  for (i = 0; i + 18 <= mg.length; i += 18) {
    var s = new Uint8Array(2304);
    for (j = 0; j < 18; j++) s.set(mg[i + j], j * 128);
    secs.push(s);
  }
  return secs;
}

/* ---------- 꾸러미 풀기 ---------- */
async function loadPkg(b64) {
  var bin = atob(b64), n = bin.length, bytes = new Uint8Array(n);
  for (var i = 0; i < n; i++) bytes[i] = bin.charCodeAt(i);
  var buf = await new Response(new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer();
  var u = new Uint8Array(buf), dv = new DataView(buf), p = 0, cnt = dv.getUint32(p, true); p += 4;
  var out = {}, td = new TextDecoder();
  for (i = 0; i < cnt; i++) {
    var nl = dv.getUint16(p, true); p += 2;
    var name = td.decode(u.subarray(p, p + nl)); p += nl;
    var len = dv.getUint32(p, true); p += 4;
    out[name] = u.subarray(p, p + len); p += len;
  }
  return out;
}

/* ---------- 재료 검사 ---------- */
function resolveFiles(list, fp) {
  var byName = {}, i;
  for (i = 0; i < list.length; i++) { var k = list[i].name.toLowerCase(); if (!(k in byName)) byName[k] = list[i]; }
  var m = {};
  var keys = ['m1_cue', 'm1_bin', 'm2_cue', 'm2_t1', 'm2_t2'];
  for (i = 0; i < keys.length; i++) {
    var f = byName[fp.files[keys[i]].toLowerCase()];
    if (!f) throw rej('없음', fp.files[keys[i]] + '이(가) 없습니다. ' + fp.need[keys[i]]);
    m[keys[i]] = f;
  }
  return m;
}
function countTracks(file) {
  var t = new TextDecoder().decode(new FileReaderSync().readAsArrayBuffer(file)), c = 0, p = -1;
  while ((p = t.indexOf('TRACK ', p + 1)) >= 0) c++;
  return c;
}
function checkMaterials(m, fp) {
  var nsec = Math.floor(m.m1_bin.size / SECT);
  if (nsec !== fp.m1_sectors) throw rej('지문다름', '1번 이미지 섹터 수 ' + nsec + ' != 기대 ' + fp.m1_sectors + '. 얼티밋 공식 방법·' + fp.m1_version + '로 만든 이미지가 필요합니다.');
  log('1번 이미지 지문 확인 중…');
  var h1 = hashFile(m.m1_bin, '1번 이미지 지문');
  if (h1 !== fp.m1_sha1) throw rej('지문다름', '1번 이미지 sha1 ' + h1.slice(0, 12) + '… != 기대 ' + fp.m1_sha1.slice(0, 12) + '…. 얼티밋 공식 방법·' + fp.m1_version + '로 만든 이미지가 필요합니다.');
  if (countTracks(m.m1_cue) !== fp.m1_tracks) throw rej('지문다름', fp.files.m1_cue + ' 트랙 수 != 기대 ' + fp.m1_tracks + '. ' + fp.need.m1_cue);
  if (m.m2_t1.size !== fp.m2_t1_size) throw rej('지문다름', '2번 T1 크기 ' + m.m2_t1.size + ' != 기대 ' + fp.m2_t1_size + '. ' + fp.need.m2_t1);
  log('2번(일본 PS1) T1 지문 확인 중…');
  var h2 = hashFile(m.m2_t1, '2번 T1 지문');
  if (h2 !== fp.m2_t1_sha1) throw rej('지문다름', '2번 T1 sha1 ' + h2.slice(0, 12) + '… != 기대 ' + fp.m2_t1_sha1.slice(0, 12) + '…. v1.2 판만 받습니다. ' + fp.need.m2_t1);
  if (m.m2_t2.size !== fp.m2_t2_size) throw rej('지문다름', '2번 T2 크기 ' + m.m2_t2.size + ' != 기대 ' + fp.m2_t2_size + '. v1.2 판만 받습니다. ' + fp.need.m2_t2);
  var h3 = hashFile(m.m2_t2, '2번 T2 지문');
  if (h3 !== fp.m2_t2_sha1) throw rej('지문다름', '2번 T2 sha1 != 기대. v1.2 판만 받습니다. ' + fp.need.m2_t2);
  if (countTracks(m.m2_cue) !== fp.m2_tracks) throw rej('지문다름', fp.files.m2_cue + ' 트랙 수 != 기대 ' + fp.m2_tracks + '. v1.2 판만 받습니다. ' + fp.need.m2_cue);
}

/* ---------- 동기 핸들 쓰기 ---------- */
function writeAll(h, buf, at) {
  var k = 0;
  while (k < buf.length) {
    var n = h.write(buf.subarray(k), { at: at + k });
    if (!(n > 0)) throw new Error('임시 파일 쓰기 실패 (디스크 여유 공간 부족일 수 있습니다)');
    k += n;
  }
}
function readAll(h, buf, at) {
  var k = 0;
  while (k < buf.length) {
    var n = h.read(buf.subarray(k), { at: at + k });
    if (!(n > 0)) throw new Error('임시 파일 읽기 실패 (위치 ' + (at + k) + ')');
    k += n;
  }
}

/* ---------- 바탕(U2c) 만들기 ---------- */
function buildU2c(pkg, mU, mT1, hOut) {
  var td = new TextDecoder(), i;
  var lines = td.decode(pkg['move_runs.tsv']).replace(/\n+$/, '').split('\n'), runs = [];
  for (i = 0; i < lines.length; i++) {
    if (i === 0) { if (lines[i].indexOf('t45_lba') !== 0) throw new Error('move_runs.tsv 헤더 이상'); continue; }
    var p = lines[i].replace(/\r$/, '').split('\t');
    runs.push({ t: parseInt(p[0], 10), n: parseInt(p[1], 10), u: p[2] === '' ? -1 : parseInt(p[2], 10), rule: p[3], form: p[4] });
  }
  var subs = pkg['subs.bin'];
  var rb = JSON.parse(td.decode(pkg['recipe_b.json'])), bmap = new Map();
  for (i = 0; i < rb.pairs.length; i++) bmap.set(rb.pairs[i].t45, { src: rb.pairs[i].src_t1, sub: hexBytes(rb.pairs[i].sub_new) });
  var rm = JSON.parse(td.decode(pkg['recipe_mono.json'])), mmap = new Map();
  Object.keys(rm).forEach(function (k) { mmap.set(parseInt(k, 10), { cfg: rm[k][0], s: rm[k][1], sub: hexBytes(rm[k][2]) }); });
  var spans = JSON.parse(td.decode(pkg['runs_span.json']));

  var nU = Math.floor(mU.size / SECT), n1 = Math.floor(mT1.size / SECT);
  var rdU = new Rd(mU, 8 << 20), rd1 = new Rd(mT1, 256 << 10);
  var ext = findExtent(rd1, ['SD', 'XA_STR1.']);
  if (ext < 0) throw rej('지문다름', '2번 T1에서 SD/XA_STR1. extent를 못 찾았습니다. v1.2 판만 받습니다.');

  var monoCache = new Map();
  function getMono(cfg) {
    var c = monoCache.get(cfg);
    if (c) return c;
    var sp = spans[String(cfg)], off = sp[0], nsec = sp[1];
    if (ext + off + 8 * (nsec - 1) >= n1) throw new Error('런 ' + cfg + ' 범위 초과');
    var payload = new Uint8Array(nsec * 2304);
    for (var k = 0; k < nsec; k++) rd1.read((ext + off + 8 * k) * SECT + 24, 2304, payload, k * 2304);
    c = monoSectors(payload);
    monoCache.set(cfg, c);
    return c;
  }

  var totalSec = 0;
  for (i = 0; i < runs.length; i++) totalSec += runs[i].n;
  var OB = new Uint8Array(4096 * SECT), op = 0, outPos = 0;
  function flushOut() { if (op) { writeAll(hOut, OB.subarray(0, op), outPos); outPos += op; op = 0; } }
  var S = new Uint8Array(SECT), T = new Uint8Array(SECT);
  var si = 0, next = 0, nb = 0, nm = 0, done = 0;
  for (var ri = 0; ri < runs.length; ri++) {
    var r = runs[ri];
    if (r.t !== next) throw new Error('move_runs 연속성 오류 t=' + r.t + ' expected=' + next);
    for (var k = 0; k < r.n; k++) {
      var tl = r.t + k, sec = OB.subarray(op, op + SECT), bp = bmap.get(tl), mr;
      sec.fill(0);
      if (bp) {
        if (bp.src >= n1) throw new Error('b-copy 원천 범위 초과 ' + tl);
        rd1.read(bp.src * SECT, SECT, T, 0);
        setHead(sec, tl, 2);
        sec.set(bp.sub, 16);
        sec.set(T.subarray(24, 2348), 24);
        sec.set(T.subarray(2348, 2352), 2348);
        nb++;
      } else if ((mr = mmap.get(tl))) {
        var m = getMono(mr.cfg);
        setHead(sec, tl, 2);
        sec.set(mr.sub, 16);
        sec.set(m[mr.s], 24);
        nm++;
      } else if (r.u >= 0) {
        var ul = r.u + k;
        if (ul >= nU) throw new Error('U 범위 초과 ' + tl);
        rdU.read(ul * SECT, SECT, S, 0);
        setHead(sec, tl, S[15]);
        if (r.rule === 'same') {
          sec.set(S.subarray(16, 2352), 16);
        } else if (r.rule === 'data_only' && r.form === 'F2') {
          sec.set(subs.subarray(si, si + 8), 16); si += 8;
          sec.set(S.subarray(24, 2348), 24);
        } else {
          throw new Error('지원하지 않는 규칙 ' + r.rule + '/' + r.form + ' (t=' + tl + ')');
        }
      }
      op += SECT;
      if (op === OB.length) flushOut();
      done++;
    }
    next = r.t + r.n;
    prog('바탕 이미지 조립', done, totalSec);
  }
  if (si !== subs.length) throw new Error('subs 소비량 불일치 ' + si + ' != ' + subs.length);
  flushOut();
  prog('바탕 이미지 조립', totalSec, totalSec, true);
  return { sectors: next, nb: nb, nm: nm };
}

/* ---------- VCDIFF (RFC 3284) 디코더 ---------- */
function makeCodeTable() {
  var T = new Uint8Array(256 * 6), i = 0, m, a, c;
  function put(t1, s1, m1, t2, s2, m2) { var o = i * 6; T[o] = t1; T[o + 1] = s1; T[o + 2] = m1; T[o + 3] = t2; T[o + 4] = s2; T[o + 5] = m2; i++; }
  /* 타입: 0 없음, 1 ADD, 2 RUN, 3 COPY */
  put(2, 0, 0, 0, 0, 0);
  for (a = 0; a <= 17; a++) put(1, a, 0, 0, 0, 0);
  for (m = 0; m <= 8; m++) { put(3, 0, m, 0, 0, 0); for (c = 4; c <= 18; c++) put(3, c, m, 0, 0, 0); }
  for (m = 0; m <= 5; m++) for (a = 1; a <= 4; a++) for (c = 4; c <= 6; c++) put(1, a, 0, 3, c, m);
  for (m = 6; m <= 8; m++) for (a = 1; a <= 4; a++) put(1, a, 0, 3, 4, m);
  for (m = 0; m <= 8; m++) put(3, 4, m, 1, 1, 0);
  if (i !== 256) throw new Error('코드 표 구성 오류 ' + i);
  return T;
}

/* d: 델타 바이트열(Uint8Array), src/out: 동기 핸들 {read,write} — onProg(done,total) */
function applyVcdiff(d, src, out, onProg) {
  var CT = makeCodeTable();
  var p = 0;
  function vi() {
    var v = 0, b;
    do { b = d[p++]; if (b === undefined) throw new Error('델타가 중간에 끊겼습니다'); v = v * 128 + (b & 127); } while (b & 128);
    return v;
  }
  if (!(d[0] === 0xd6 && d[1] === 0xc3 && d[2] === 0xc4 && d[3] === 0)) throw new Error('델타 머리말 이상');
  p = 4;
  var hi = d[p++];
  if (hi & 1) throw new Error('보조 압축 델타는 지원하지 않습니다');
  if (hi & 2) throw new Error('사용자 코드 표는 지원하지 않습니다');
  if (hi & 4) { var al = vi(); p += al; }
  var tgt = new Uint8Array(8 << 20), outPos = 0, nWin = 0;
  var near = new Float64Array(4), same = new Float64Array(768);

  function readSrc(at, view) { readAll(src, view, at); }

  while (p < d.length) {
    var win = d[p++], srcLen = 0, srcPos = 0;
    if (win & 2) throw new Error('VCD_TARGET 창은 지원하지 않습니다');
    if (win & 1) { srcLen = vi(); srcPos = vi(); }
    var dlen = vi(), wstart = p, tlen = vi(), di = d[p++];
    if (di) throw new Error('보조 압축 델타는 지원하지 않습니다');
    var addLen = vi(), instLen = vi(), addrLen = vi(), adler = null;
    if (win & 4) { adler = ((d[p] << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]) >>> 0; p += 4; }
    var dp = p, ip = dp + addLen, ap = ip + instLen, iend = ap, aend = ap + addrLen;
    if (aend !== wstart + dlen) throw new Error('창 길이 불일치');
    if (tlen > tgt.length) tgt = new Uint8Array(tlen);
    near.fill(0); same.fill(0);
    var ns = 0, tp = 0;
    function vInst() { var v = 0, b; do { b = d[ip++]; v = v * 128 + (b & 127); } while (b & 128); return v; }
    function vAddr() { var v = 0, b; do { b = d[ap++]; v = v * 128 + (b & 127); } while (b & 128); return v; }
    while (ip < iend) {
      var idx = d[ip++];
      for (var part = 0; part < 2; part++) {
        var o = idx * 6 + part * 3, type = CT[o], size = CT[o + 1], mode = CT[o + 2];
        if (type === 0) continue;
        if (type !== 2 && type !== 1 && type !== 3) throw new Error('명령 종류 오류');
        if (size === 0) size = vInst();
        if (tp + size > tlen) throw new Error('창 목표 길이 초과');
        if (type === 1) {
          tgt.set(d.subarray(dp, dp + size), tp); dp += size;
        } else if (type === 2) {
          tgt.fill(d[dp++], tp, tp + size);
        } else {
          var here = srcLen + tp, addr;
          if (mode === 0) addr = vAddr();
          else if (mode === 1) addr = here - vAddr();
          else if (mode < 6) addr = near[mode - 2] + vAddr();
          else addr = same[(mode - 6) * 256 + d[ap++]];
          near[ns] = addr; ns = (ns + 1) & 3; same[addr % 768] = addr;
          var a = addr, n = size, q = tp;
          while (n > 0) {
            if (a < srcLen) {
              var mm = Math.min(n, srcLen - a);
              readSrc(srcPos + a, tgt.subarray(q, q + mm));
              a += mm; q += mm; n -= mm;
            } else {
              var s = a - srcLen;
              if (s >= q) throw new Error('복사 주소 오류');
              while (n > 0) { tgt[q++] = tgt[s++]; n--; }
            }
          }
        }
        tp += size;
      }
    }
    if (dp !== ip - instLen) throw new Error('ADD 데이터 소비량 불일치');
    if (tp !== tlen) throw new Error('창 목표 길이 불일치');
    if (adler !== null && adler32(tgt, tlen) !== adler) throw new Error('창 ' + nWin + ' adler32 불일치');
    writeAll(out, tgt.subarray(0, tlen), outPos);
    outPos += tlen; nWin++;
    p = aend;
    if (onProg) onProg(nWin, outPos);
  }
  return outPos;
}

/* ---------- 전체 흐름 ---------- */
async function run(msg) {
  var t0 = Date.now();
  var pkg = await loadPkg(msg.pkg);
  var fp = JSON.parse(new TextDecoder().decode(pkg['fingerprints.json']));
  var m = resolveFiles(msg.files, fp);
  checkMaterials(m, fp);
  log('OK 재료 확인', 'ok');

  if (!(self.navigator && navigator.storage && navigator.storage.getDirectory)) throw new Error('이 브라우저는 임시 저장소(OPFS)를 지원하지 않습니다. 크롬·엣지 최신판을 사용하세요.');
  var root = await navigator.storage.getDirectory();
  var outName = fp.files.m1_bin.replace('.bin', '_KR_v0.9.bin');
  var names = ['u2c.tmp', outName];
  for (var i = 0; i < names.length; i++) { try { await root.removeEntry(names[i]); } catch (e) { } }
  var hU, hO;
  try {
    hU = await (await root.getFileHandle('u2c.tmp', { create: true })).createSyncAccessHandle();
    hO = await (await root.getFileHandle(outName, { create: true })).createSyncAccessHandle();
    hU.truncate(0); hO.truncate(0);

    var r = buildU2c(pkg, m.m1_bin, m.m2_t1, hU);
    hU.flush();
    log('OK 바탕 이미지: ' + r.sectors + ' 섹터, b-삽입 ' + r.nb + ', mono-삽입 ' + r.nm, 'ok');

    log('델타 적용 중… (몇 분 걸릴 수 있습니다)');
    var total = fp.result_size;
    try {
      applyVcdiff(pkg['u2c_to_t45.vcdiff'], hU, hO, function (w, o) { prog('델타 적용', o, total); });
    } catch (e) {
      throw rej('적용실패', '델타 적용 실패(' + e.message + '): 어느 단계·구간에서 달랐는지 제보 양식으로 보내주십시오.');
    }
    hO.flush();
    prog('델타 적용', total, total, true);

    var size = hO.getSize();
    var h = new Sha1(), buf = new Uint8Array(8 << 20), off = 0;
    while (off < size) {
      var nn = Math.min(buf.length, size - off), v = buf.subarray(0, nn);
      readAll(hO, v, off); h.update(v); off += nn;
      prog('결과 지문 확인', off, size);
    }
    var hex = h.hex();
    if (hex !== fp.result_sha1) throw rej('불일치', '결과 sha1 ' + hex + ' != 기대 ' + fp.result_sha1 + '. 어느 단계·구간에서 달랐는지 제보 양식으로 보내주십시오.');
    hU.close(); hU = null; hO.close(); hO = null;
    try { await root.removeEntry('u2c.tmp'); } catch (e) { }
    post({ t: 'done', sha1: hex, size: size, outName: outName, sec: Math.round((Date.now() - t0) / 1000) });
  } finally {
    try { if (hU) hU.close(); } catch (e) { }
    try { if (hO) hO.close(); } catch (e) { }
  }
}

if (typeof self !== 'undefined' && typeof self.onmessage !== 'undefined' && typeof window === 'undefined') {
  self.onmessage = function (ev) {
    var msg = ev.data;
    if (msg.t === 'run') {
      run(msg).catch(function (e) {
        if (e instanceof Reject) post({ t: 'err', text: e.message });
        else {
          var s = (e && e.name === 'QuotaExceededError') ? '디스크 여유 공간이 부족합니다 (1.5GB 이상 필요).' : String(e && e.message || e);
          post({ t: 'err', text: 'REJECT 오류: ' + s });
        }
      });
    } else if (msg.t === 'clean') {
      (async function () {
        var root = await navigator.storage.getDirectory(), n = 0;
        for await (var k of root.keys()) { try { await root.removeEntry(k); n++; } catch (e) { } }
        post({ t: 'cleaned', n: n });
      })();
    }
  };
}
if (typeof module !== 'undefined') module.exports = { Sha1: Sha1, adler32: adler32, applyVcdiff: applyVcdiff, monoSectors: monoSectors };
