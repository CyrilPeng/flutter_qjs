"""Verify the reviewed core's complete source inventory without using the network."""

import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'CORE_PATCHES.json').read_text(encoding='utf-8'))
assert manifest['schema'] == 1
records = manifest['files']
expected = {record['path'] for record in records}
assert len(expected) == len(records), 'Duplicate core source record'
actual = {file.relative_to(root).as_posix()
          for file in (root / 'cxx/quickjs-ng').iterdir() if file.is_file()}
assert actual == expected, f'Unreviewed or missing core files: {actual ^ expected}'
for record in records:
    source = (root / record['path']).read_bytes().replace(b'\r\n', b'\n')
    assert hashlib.sha256(source).hexdigest() == record['patched_sha256'], record['path']
assert (root / manifest['patch_record']).is_file(), 'Missing core patch explanation'
print(f'Reviewed native core source verified: {len(records)} files')
