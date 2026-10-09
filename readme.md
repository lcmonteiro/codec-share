# Codec Share

![fusion](doc/codec-share.svg)

## WebAssembly

```
wasm/
  codec_share.cpp        C interface of the codec, built as a WASI reactor
  build.sh               builds codec-share.wasm (wasi-sdk 25)
  codec-share.wasm       the committed build, shared by every binding
  bindings/
    python/              codec_share python package and codec-share command
```

The module exposes a flat C interface, so any WebAssembly runtime can load it:

| function | description |
| --- | --- |
| `cs_stamp(kind, seed, out)` | stamp (256 densities) generated from a seed |
| `cs_frame_size(size, needed)` | size of each coded frame |
| `cs_encode(stamp, data, size, needed, total, out, cap)` | `data` split in `needed` frames, coded in `total >= needed` frames, each one merging all of them |
| `cs_decode(stamp, frames, frame, count, needed, out, cap)` | data back from any `needed` independent frames |
| `cs_alloc(size)`, `cs_free(ptr)` | memory to pass data in and out |

Encoded data carries its length and a checksum, so frames decoded with a different stamp
fail instead of returning garbage.

**`wasm/codec-share.wasm` is committed**: bindings and other projects take it as is, no
toolchain needed. It must be rebuilt with every change of the sources: CI rebuilds it and
fails when the committed one differs (the build is reproducible with the pinned wasi-sdk),
and keeps the rebuilt module as the `codec-share-wasm` artifact.

```bash
WASI_SDK_PATH=/path/to/wasi-sdk-25.0 ./wasm/build.sh   # -> wasm/codec-share.wasm
```

### Python binding

`wasm/bindings/python` is the `codec-share` python package: the module run with
[wasmtime](https://pypi.org/project/wasmtime/), share files on top of it, and the
`codec-share` command ([click](https://click.palletsprojects.com)).

```python
from codec_share import Stamp, split, join, save, load, edit

stamp  = Stamp.from_pin('4821')                  # or Stamp.random(), Stamp.load(path)
shares = split(data, stamp, total=3, needed=2)   # any 2 of the 3 shares ...
data   = join(shares[1:], stamp)                 # ... give the data back

paths  = save(shares, 'config.yml')              # config.yml.1.share ... config.yml.3.share
data   = join(load(paths[:2]), stamp)
edit(paths[:2], stamp)                           # edit in place, see below
```

| name | what it is |
| --- | --- |
| `Stamp` | the secret shaping the coding: `from_pin`, `random`, `load`, `save` |
| `Share` | one share: `needed`, `total`, `index`, `split_id`, `frame`; `parse`, `load`, `save` |
| `split`, `join` | data to shares, shares to data |
| `save`, `load` | share files, named `<prefix>.<index>.share` |
| `edit` | the file of some shares, edited in place |
| `Codec` | the WebAssembly module: `stamp`, `encode`, `decode` |
| `NotEnoughShares`, `WrongStamp`, `InvalidShare` | errors, all `CodecError` |

```bash
codec-share split config.yml                 # asks a pin, config.yml.{1,2,3}.share, any 2 join
codec-share split config.yml -n 4 -k 3       # 4 shares, any 3 join
codec-share stamp my.stamp                   # random stamp file, instead of a pin (--stamp)
codec-share join config.yml.1.share config.yml.3.share > config.yml
codec-share edit config.yml.1.share config.yml.3.share
```

`edit` joins the file in a private temporary folder (memory backed when there is one) and
opens it in an editor (`-e`, else `$VISUAL`, `$EDITOR`). When the editor closes with
changes, the file is split again over all its shares (the siblings named
`<prefix>.<index>.share` too, same `needed` and `total`), and the plain file is wiped and
removed in any case. The editor must only return once the file is closed (`code --wait`,
`subl -w`...). The pin is read from `$CODEC_SHARE_PIN` when set.

Tools reuse the commands under their own name, pin variable and file validation:

```python
from codec_share.cli import commands

main = commands('mytool-share', pin_envvar='MYTOOL_PIN', validate=check, what='settings file')
```

```bash
cd wasm/bindings/python
pip install wasmtime click
python -m unittest discover -s tests
```

### Release

Each `v<version>` tag (matching `wasm/bindings/python/pyproject.toml`) publishes the python
wheel as a GitHub release asset, so projects depend on it without git or a wasm toolchain:

```toml
# pyproject.toml of the consumer (uv)
dependencies = ["codec-share"]

[tool.uv.sources]
codec-share = { url = "https://github.com/lcmonteiro/codec-share/releases/download/v0.1.0/codec_share-0.1.0-py3-none-any.whl" }
```

To release: bump `version`, merge, then `git tag v<version> && git push origin v<version>`.

Mind that the coding is linear and the stamp only shapes the coefficients: it hides the
data from a casual reader, it is **not** encryption (a wrong stamp even opens the shares
now and then). Encrypt first when the data must stay secret.
