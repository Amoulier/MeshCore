// Regeneration tool only. Requires Sharp; it is not a firmware dependency.
const fs = require('node:fs');
const path = require('node:path');
const sharp = require('sharp');
const [sourceDir, outputDir] = process.argv.slice(2);
if (!sourceDir || !outputDir) throw new Error('Usage: node rasterize-openmoji.cjs SVG_DIR PNG_DIR');
fs.mkdirSync(outputDir, { recursive: true });
const files = fs.readdirSync(sourceDir).filter(name => /^[0-9A-F-]+\.svg$/.test(name));
let index = 0;
async function worker() {
  while (index < files.length) {
    const name = files[index++];
    await sharp(path.join(sourceDir, name)).resize(72, 72)
      .png().toFile(path.join(outputDir, name.replace('.svg', '.png')));
  }
}
Promise.all(Array.from({ length: 6 }, worker)).then(() => process.stdout.write(`Rendered ${files.length} SVGs\n`))
  .catch(error => { process.stderr.write(String(error)); process.exitCode = 1; });
