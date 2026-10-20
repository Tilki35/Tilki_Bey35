"""Generate a self-contained fallback; external catalogs can be added without rebuilding."""
import hashlib
import json
from pathlib import Path

def embed(root, build):
    manifest = json.loads((root/'third_party/sources.json').read_text())
    for name, source in manifest.items():
        if hashlib.sha256((root/name).read_bytes()).hexdigest() != source['sha256']:
            raise RuntimeError('Vendored source checksum mismatch: ' + name)
    locales = root/'locales'
    resource = {
        'catalogs': {p.stem: json.loads(p.read_text(encoding='utf-8'))
                     for p in locales.glob('*.json') if p.name != 'names.json'},
        'names': json.loads((locales/'names.json').read_text(encoding='utf-8')),
        'aliases': json.loads((locales/'cldr/aliases.json').read_text()),
        'likely': json.loads((locales/'cldr/likelySubtags.json').read_text()),
        'scripts': json.loads((locales/'cldr/scriptMetadata.json').read_text()),
    }
    text = json.dumps(resource, ensure_ascii=False, separators=(',', ':'))
    assert ')UF_RESOURCE"' not in text
    (build/'resources.hpp').write_text(
        '#pragma once\ninline constexpr const char* embeddedResources = R"UF_RESOURCE('
        + text + ')UF_RESOURCE";\n', encoding='utf-8')
