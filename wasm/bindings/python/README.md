# codec-share (python)

Python binding of [codec-share](https://github.com/lcmonteiro/codec-share), running its
WebAssembly module with wasmtime: split a file in coded shares, any `needed` of the
`total` shares with the same pin or stamp give it back.

```python
from codec_share import Stamp, split, join

stamp  = Stamp.from_pin('4821')
shares = split(data, stamp, total=3, needed=2)
data   = join(shares[1:], stamp)
```

```bash
codec-share split config.yml        # config.yml.1.share ... config.yml.3.share
codec-share edit config.yml.1.share config.yml.3.share
```

The coding hides the file from a casual reader, it is **not** encryption.
