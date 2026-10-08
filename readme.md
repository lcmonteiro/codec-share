# Codec Share

![fusion](doc/codec-share.svg)

## WebAssembly

`wasm/codec_share.cpp` exposes the codec through a flat C interface, built as a WASI
reactor so any WebAssembly runtime can load it. `wasm/codec_share.py` is the python
binding, running the module with [wasmtime](https://pypi.org/project/wasmtime/).

The built module, **`wasm/codec-share.wasm`, is committed**: other projects take it (with
`wasm/codec_share.py` for python) as is, no toolchain needed. It must be rebuilt with
every change of the sources, and CI rebuilds it and fails when the committed one differs
(the build is reproducible with the pinned wasi-sdk). The run also keeps the rebuilt
module as the `codec-share-wasm` artifact.

```bash
WASI_SDK_PATH=/path/to/wasi-sdk-25.0 ./wasm/build.sh   # -> wasm/codec-share.wasm
pip install wasmtime
python -m unittest discover -s wasm                    # binding tests
```

[wasi-sdk 25](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-25) is the only
build requirement.

### Python package

`pyproject.toml` packages the binding with the committed module as `codec_share`. Each
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

codec  = Codec()                           # wasm/codec-share.wasm
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
