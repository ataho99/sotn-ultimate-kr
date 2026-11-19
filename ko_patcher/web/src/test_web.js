// 헤드리스 크롬 종단 시험: node test_web.js <index.html> <재료폴더> <다운로드 저장 폴더> [chrome 경로]
const http = require('http'), fs = require('fs'), path = require('path');
const { chromium } = require('playwright-core');
const [html, matDir, saveDir, chromePath] = process.argv.slice(2);
const NAMES = ['SOTN_Ultimate_v1.4.bin', 'SOTN_Ultimate_v1.4.cue', 'SOTN_JP_PS1_v1.2.cue', 'SOTN_JP_PS1_v1.2_T1.bin', 'SOTN_JP_PS1_v1.2_T2.ape'];
const srv = http.createServer((req, res) => { res.setHeader('content-type', 'text/html; charset=utf-8'); fs.createReadStream(html).pipe(res); });
(async () => {
  await new Promise(r => srv.listen(8123, '127.0.0.1', r));
  fs.mkdirSync(saveDir, { recursive: true });
  const br = await chromium.launch({ executablePath: chromePath || undefined, headless: true, args: ['--no-sandbox', '--disable-dev-shm-usage'] });
  console.log('browser', br.version());
  const ctx = await br.newContext({ acceptDownloads: true });
  const page = await ctx.newPage();
  const reqs = [];
  page.on('request', r => { const u = r.url(); if (!u.startsWith('http://127.0.0.1:8123') && !u.startsWith('blob:') && !u.startsWith('data:')) reqs.push(u); });
  page.on('console', m => console.log('console:', m.text()));
  page.on('pageerror', e => console.log('pageerror:', e.message));
  await page.goto('http://127.0.0.1:8123/');
  await page.setInputFiles('#fFiles', NAMES.map(n => path.join(matDir, n)));
  await page.waitForFunction(() => !document.getElementById('go').disabled, null, { timeout: 60000 });
  const t0 = Date.now();
  await page.click('#go');
  let last = '';
  const tick = setInterval(async () => { try { const p = await page.textContent('#phase'); if (p !== last && Date.now() % 5 === 0) { last = p; } console.log(((Date.now() - t0) / 1000).toFixed(0) + 's', p); } catch (e) { } }, 15000);
  await page.waitForFunction(() => ['done', 'error'].includes(document.body.dataset.state), null, { timeout: 40 * 60 * 1000, polling: 2000 });
  clearInterval(tick);
  const state = await page.evaluate(() => document.body.dataset.state);
  console.log('log:\n' + await page.textContent('#log'));
  console.log('STATE', state, 'external requests:', JSON.stringify(reqs));
  if (state === 'done') {
    const r = await page.evaluate(() => window.__result); console.log('RESULT', JSON.stringify(r));
    const [dl] = await Promise.all([page.waitForEvent('download', { timeout: 600000 }), page.click('#dlBin')]);
    const bin = path.join(saveDir, dl.suggestedFilename()); await dl.saveAs(bin); console.log('SAVED', bin, fs.statSync(bin).size);
    const [dl2] = await Promise.all([page.waitForEvent('download', { timeout: 60000 }), page.click('#dlCue')]);
    const cue = path.join(saveDir, dl2.suggestedFilename()); await dl2.saveAs(cue); console.log('SAVED', cue, fs.statSync(cue).size);
  }
  // 거부 시험은 별도 실행에서
  await br.close(); srv.close();
  process.exit(state === 'done' ? 0 : 3);
})().catch(e => { console.log('TESTERR', e); process.exit(4); });
