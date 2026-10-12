#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the public browser preview; hardware candidates remain local builds."""
import argparse
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('out', type=Path)
out = p.parse_args().out.resolve()
if out.exists():
    p.error('Output must be a new directory.')
out.mkdir(parents=True)
web = ROOT / 'web'
for source in web.iterdir():
    if source.is_file() and (source.suffix in ('.js', '.css', '.svg', '.png', '.ttf', '.txt') or
                            source.name in ('index.html', 'editor.html', 'simulator.html')):
        shutil.copy2(source, out / source.name)
for name in ('audio', 'fonts', 'screenshots'):
    shutil.copytree(web / name, out / name)
for name in ('LICENSE', 'LICENSING.md'):
    shutil.copy2(ROOT / name, out / name)
shutil.copytree(ROOT / 'LICENSES', out / 'LICENSES')
# Do not silently promote the untested hardware candidate with the browser site.
template = (web / 'installer.html').read_text(encoding='utf-8')
head = template.split('<body', 1)[0].replace('<script type="module" src="installer/controller.js"></script>', '').replace('<link rel="stylesheet" href="installer/installer.css">', '')
(out / 'installer.html').write_text(head + '''<body class="landing-page"><main>
<header class="masthead"><h1 class="melodee-wordmark">melodee <span>SMK</span></h1></header>
<section class="landing-section"><span class="release-badge">HARDWARE PORT / IN DEVELOPMENT</span>
<h2>Play Melodee in your browser.</h2>
<p>The Studio and Simulator are available now. The SMK hardware build remains a development candidate awaiting device testing; this site does not offer a public firmware installation.</p>
<p><a href="editor.html?audio=1">Open Studio →</a> · <a href="simulator.html">Open Simulator →</a> · <a href="index.html">Home</a></p>
<p>Developers can find the local build and installer instructions in <a href="https://github.com/ChanceTheMaker/melodee-smk/blob/main/README_SMK.md">the source repository</a>.</p>
</section></main></body></html>''', encoding='utf-8')
(out / '.nojekyll').write_text('')
print('Staged browser preview:', out)
