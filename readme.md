# Codec Share

![fusion](doc/codec-share.svg)

## WebAssembly

`wasm/codec_share.cpp` exposes the codec through a flat C interface, built as a WASI
reactor so any WebAssembly runtime can load it. `wasm/codec_share.py` is the python
binding, running the module with [wasmtime](https://pypi.org/project/wasmtime/), and
`wasm/codec_share/shares.py` the share files on top of it (with the `codec-share` command).

The built module, **`wasm/codec_share/codec-share.wasm`, is committed**: other projects
take it as is (or the python package below), no toolchain needed. It must be rebuilt with
every change of the sources, and CI rebuilds it and fails when the committed one differs
(the build is reproducible with the pinned wasi-sdk). The run also keeps the rebuilt
module as the `codec-share-wasm` artifact.

```bash
WASI_SDK_PATH=/path/to/wasi-sdk-25.0 ./wasm/build.sh   # -> wasm/codec_share/codec-share.wasm
pip install wasmtime
python -m unittest discover -s wasm                    # binding and shares tests
```

[wasi-sdk 25](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-25) is the only
build requirement.

### Share files

A file is coded in `n` share files, any `k` of them together with a pin, or a stamp file,
give it back (`codec_share.shares`, or the `codec-share` command):

```bash
codec-share split config.yml                      # asks a pin, config.yml.{1,2,3}.share, any 2 open
codec-share split config.yml -n 4 -k 3            # 4 shares, any 3 open
codec-share stamp my.stamp                        # random stamp file, instead of a pin
codec-share join config.yml.1.share config.yml.3.share > config.yml
codec-share edit config.yml.1.share config.yml.3.share
```

`edit` opens the file in an editor (`-e`, else `$VISUAL`, `$EDITOR`) in a private
temporary folder (memory backed when there is one). When the editor closes with changes,
the file is split again over all the shares (the siblings named `<prefix>.<index>.share`
too, same `k` and `n`), and the plain file is overwritten and removed in any case. The
editor must only return once the file is closed (`code --wait`, `subl -w`...). The pin is
read from `$CODEC_SHARE_PIN` when set. Tools wrap the command with their own name, pin
variable and file validation: `shares.main(prog=..., pin_env=..., validate=...)`.

### Python package

`pyproject.toml` packages `wasm/codec_share` (binding, committed module, share files and
the `codec-share` command). Each
`v<version>` tag (matching the version in `pyproject.toml`) publishes the wheel as a
GitHub release asset, so projects depend on it without git or a wasm toolchain:

```toml
# pyproject.toml of the consumer (uv)
dependencies = ["codec-share"]

[tool.uv.sources]
codec-share = { url = "https://github.com/lcmonteiro/codec-share/releases/download/v0.1.0/codec_share-0.1.0-py3-none-any.whl" }
```

To release: bump `version` in `pyproject.toml`, merge, then
`git tag v<version> && git push origin v<version>`.

```python
from codec_share import Codec

codec  = Codec()                           # wasm/codec_share/codec-share.wasm
stamp  = codec.stamp(seed)                 # 512 bytes, or any stamp saved before
frames = codec.seal(stamp, data, 2, 3)     # 3 coded frames, any 2 open the data
data   = codec.open(stamp, frames[1:], 2)
```

| function | description |
| --- | --- |
| `cs_stamp_generate(type, seed, out)` | stamp (256 densities) generated from a seed |
| `cs_frame_size(size, k)` | size of each coded frame |
| `cs_seal(stamp, data, size, k, n, out, cap)` | `data` split in `k` frames and coded in `n >= k` frames, each one merging all `k` |
| `cs_open(stamp, frames, frame, count, k, out, cap)` | data back from any `k` independent frames |

Sealed data carries its length and a checksum, so frames opened with a different stamp
fail instead of returning garbage. Mind that the coding is linear and the stamp only
shapes the coefficients: it hides the data from a casual reader, it is **not**
encryption. Encrypt first when the data must stay secret.
