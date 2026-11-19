// 거부(REJECT) 문구 시험: node test_rej.js <index.html> <재료폴더> <작업폴더> <chrome>
const http = require('http'), fs = require('fs'), path = require('path');
const { chromium } = require('playwright-core');
const [html, mat, wd, chromePath] = process.argv.slice(2);
const N = ['SOTN_Ultimate_v1.4.bin', 'SOTN_Ultimate_v1.4.cue', 'SOTN_JP_PS1_v1.2.cue', 'SOTN_JP_PS1_v1.2_T1.bin', 'SOTN_JP_PS1_v1.2_T2.ape'];
function mk(name, over) {
  const d = path.join(wd, name); fs.rmSync(d, { recursive: true, force: true }); fs.mkdirSync(d, { recursive: true });
  for (const n of N) { const dst = path.join(d, n); if (over[n]) over[n](dst, path.join(mat, n)); else fs.symlinkSync(path.join(mat, n), dst); }
  return d;
}
function trunc(bytes) { return (dst, src) => { const fd = fs.openSync(src, 'r'), b = Buffer.alloc(bytes); fs.readSync(fd, b, 0, bytes, 0); fs.closeSync(fd); fs.writeFileSync(dst, b); }; }
function flip(off) { return (dst, src) => { fs.copyFileSync(src, dst); const fd = fs.openSync(dst, 'r+'), b = Buffer.alloc(1); fs.readSync(fd, b, 0, 1, off); b[0] ^= 1; fs.writeSync(fd, b, 0, 1, off); fs.closeSync(fd); }; }
function cue2(dst, src) { fs.writeFileSync(dst, fs.readFileSync(src, 'utf8') + '  TRACK 02 AUDIO\r\n    INDEX 01 00:00:00\r\n'); }
const cases = [
  ['A_작은1번', mk('A', { [N[0]]: trunc(2352 * 1000) }), 'REJECT 지문다름: 1번 이미지 섹터 수'],
  ['B_T2한바이트', mk('B', { [N[4]]: flip(1000) }), 'REJECT 지문다름: 2번 T2 sha1'],
  ['C_cue트랙', mk('C', { [N[1]]: cue2 }), 'REJECT 지문다름: SOTN_Ultimate_v1.4.cue 트랙 수'],
  ['D_파일없음', (() => { const d = mk('D', {}); fs.unlinkSync(path.join(d, N[3])); return d; })(), 'REJECT 없음: SOTN_JP_PS1_v1.2_T1.bin이(가) 없습니다'],
];
const srv = http.createServer((q, r) => { r.setHeader('content-type', 'text/html; charset=utf-8'); fs.createReadStream(html).pipe(r); });
(async () => {
  await new Promise(r => srv.listen(8124, '127.0.0.1', r));
  const br = await chromium.launch({ executablePath: chromePath || undefined, headless: true, args: ['--no-sandbox'] });
  let bad = 0;
  for (const [nm, dir, want] of cases) {
    const ctx = await br.newContext(); const page = await ctx.newPage();
    await page.goto('http://127.0.0.1:8124/');
    const files = fs.readdirSync(dir).map(f => path.join(dir, f));
    await page.setInputFiles('#fFiles', files);
    const enabled = await page.evaluate(() => !document.getElementById('go').disabled);
    if (nm.startsWith('D')) { console.log(nm, '시작 버튼 활성:', enabled, '(파일 4개 → 비활성이어야 함)'); if (enabled) bad++; await ctx.close(); continue; }
    await page.click('#go');
    await page.waitForFunction(() => ['done', 'error'].includes(document.body.dataset.state), null, { timeout: 300000, polling: 1000 });
    const err = await page.evaluate(() => window.__error);
    const hit = err && err.startsWith(want);
    console.log(nm, hit ? 'OK ' : 'BAD', err); if (!hit) bad++;
    await ctx.close();
  }
  // 파일 없음 문구는 워커에서 직접(파일 목록 4개로 실행) 시험
  {
    const dir = cases[3][1]; const ctx = await br.newContext(); const page = await ctx.newPage();
    await page.goto('http://127.0.0.1:8124/');
    const r = await page.evaluate(async () => {
      const src = document.getElementById('wsrc').textContent;
      const w = new Worker(URL.createObjectURL(new Blob([src], { type: 'text/javascript' })));
      return await new Promise(res => { w.onmessage = e => { if (e.data.t === 'err') res(e.data.text); }; w.postMessage({ t: 'run', files: [new File(['x'], 'SOTN_Ultimate_v1.4.cue')], pkg: document.getElementById('pkg').textContent.trim() }); });
    });
    const ok = r.startsWith('REJECT 없음: SOTN_Ultimate_v1.4.bin이(가) 없습니다'); console.log('D2_워커파일없음', ok ? 'OK ' : 'BAD', r); if (!ok) bad++;
    await ctx.close();
  }
  await br.close(); srv.close();
  console.log(bad ? 'REJ FAIL ' + bad : 'REJ PASS');
  process.exit(bad ? 1 : 0);
})().catch(e => { console.log('TESTERR', e); process.exit(4); });
