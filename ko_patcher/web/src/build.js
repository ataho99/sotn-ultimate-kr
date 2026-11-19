// 웹 패처 빌드: node build.js <꾸러미폴더> <template.html> <worker.js> <출력 html>
// 꾸러미(7개 파일)를 [u32 개수]{[u16 이름길이][이름][u32 길이][내용]} 로 묶어 gzip → base64 로 HTML 에 박는다.
const fs = require('fs'), path = require('path'), zlib = require('zlib');
const [pkgDir, tplPath, workerPath, outPath] = process.argv.slice(2);
const names = ['fingerprints.json', 'move_runs.tsv', 'subs.bin', 'recipe_b.json', 'recipe_mono.json', 'runs_span.json', 'u2c_to_t45.vcdiff'];
const parts = [];
const cnt = Buffer.alloc(4); cnt.writeUInt32LE(names.length); parts.push(cnt);
for (const n of names) {
  const data = fs.readFileSync(path.join(pkgDir, n));
  const nb = Buffer.from(n, 'utf8');
  const h = Buffer.alloc(2); h.writeUInt16LE(nb.length);
  const l = Buffer.alloc(4); l.writeUInt32LE(data.length);
  parts.push(h, nb, l, data);
}
const raw = Buffer.concat(parts);
const gz = zlib.gzipSync(raw, { level: 9 });
const b64 = gz.toString('base64');
let tpl = fs.readFileSync(tplPath, 'utf8');
const worker = fs.readFileSync(workerPath, 'utf8');
if (/<\/script/i.test(worker)) throw new Error('worker.js 에 </script 가 있음');
tpl = tpl.replace('/*__WORKER__*/', () => worker).replace('/*__PKG__*/', () => b64);
fs.writeFileSync(outPath, tpl);
console.log('raw', raw.length, 'gz', gz.length, 'b64', b64.length, 'html', fs.statSync(outPath).size);
