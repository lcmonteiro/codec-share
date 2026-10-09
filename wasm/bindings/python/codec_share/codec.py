# =======================================================================================
# codec: the codec-share WebAssembly module (wasm/codec-share.wasm), run with wasmtime
#
#   codec  = Codec.default()
#   frames = codec.encode(data, stamp, needed=2, total=3)   # any 2 of the 3 frames ...
#   data   = codec.decode(frames[1:], stamp, needed=2)      # ... give the data back
# =======================================================================================
from pathlib import Path

from wasmtime import Engine, Linker, Module, Store, WasiConfig

_HERE = Path(__file__).resolve().parent

# packaged next to this file, or the committed module when running from the sources
WASM = next(
    (path for path in (_HERE / 'codec-share.wasm', _HERE.parents[2] / 'codec-share.wasm')
     if path.exists()),
    _HERE / 'codec-share.wasm')


# =======================================================================================
# errors
# =======================================================================================
class CodecError(Exception):
    """base of the codec-share errors"""


class NotEnoughShares(CodecError):
    """fewer independent frames (shares) than needed to decode"""


class WrongStamp(CodecError):
    """the frames do not decode with this stamp (or pin), or are corrupted"""


# =======================================================================================
# codec
# =======================================================================================
class Codec:
    """the WebAssembly codec: stamps, encode and decode"""

    # stamp kinds
    SPARSE, STREAM, MESSAGE, FULL = range(4)
    # limits
    MAX_NEEDED, MAX_TOTAL = 16, 255

    _ERRORS = {
        -1: (CodecError, 'invalid arguments'),
        -2: (NotEnoughShares, 'not enough independent frames'),
        -3: (WrongStamp, 'frames do not decode with this stamp'),
        -4: (CodecError, 'output buffer too small'),
    }
    _default = None

    @classmethod
    def default(cls):
        """codec of the packaged module, created once"""
        if cls._default is None:
            cls._default = cls()
        return cls._default

    def __init__(self, wasm=WASM):
        engine = Engine()
        self._store = Store(engine)
        self._store.set_wasi(WasiConfig())  # only random_get is used, for the coding seeds
        linker = Linker(engine)
        linker.define_wasi()
        instance = linker.instantiate(self._store, Module.from_file(engine, str(wasm)))
        exports = instance.exports(self._store)
        exports['_initialize'](self._store)
        self._memory = exports['memory']
        self._fn = {name: exports[f'cs_{name}'] for name in (
            'alloc', 'free', 'stamp_size', 'stamp', 'frame_size', 'encode', 'decode')}
        self.stamp_size = self._call('stamp_size')

    # -----------------------------------------------------------------------------------
    # interface
    # -----------------------------------------------------------------------------------
    def stamp(self, seed, kind=MESSAGE):
        """stamp (raw bytes) generated from a 64 bits seed"""
        with self._buffer(self.stamp_size) as out:
            self._check(self._call('stamp', kind, seed & (2**64 - 1), out.ptr))
            return out.read(self.stamp_size)

    def encode(self, data, stamp, needed, total):
        """codes data in total frames, any needed independent frames decode it"""
        stamp = self._stamp(stamp)
        frame = self._check(self._call('frame_size', len(data), needed))
        size = frame * total
        with self._buffer(stamp) as s, self._buffer(data) as d, self._buffer(size) as out:
            frame = self._check(
                self._call('encode', s.ptr, d.ptr, len(data), needed, total, out.ptr, size))
            raw = out.read(size)
        return [raw[i:i + frame] for i in range(0, size, frame)]

    def decode(self, frames, stamp, needed):
        """data back from frames encoded with the same stamp"""
        stamp = self._stamp(stamp)
        frames = list(frames)
        if not frames or len({len(f) for f in frames}) != 1:
            raise CodecError('frames must have the same size')
        data = b''.join(frames)
        with self._buffer(stamp) as s, self._buffer(data) as d, self._buffer(len(data)) as out:
            size = self._check(self._call(
                'decode', s.ptr, d.ptr, len(frames[0]), len(frames), needed, out.ptr, len(data)))
            return out.read(size)

    # -----------------------------------------------------------------------------------
    # helpers
    # -----------------------------------------------------------------------------------
    def _stamp(self, stamp):
        stamp = bytes(stamp)
        if len(stamp) != self.stamp_size:
            raise WrongStamp(f'a stamp has {self.stamp_size} bytes, got {len(stamp)}')
        return stamp

    def _call(self, name, *args):
        return self._fn[name](self._store, *args)

    def _check(self, result):
        if result < 0:
            kind, message = self._ERRORS.get(result, (CodecError, f'error {result}'))
            raise kind(message)
        return result

    def _buffer(self, init):
        return _Buffer(self, init)


class _Buffer:
    """block of the wasm memory, initialized with bytes or sized with an int"""

    def __init__(self, codec, init):
        self._codec = codec
        size = init if isinstance(init, int) else len(init)
        self.ptr = codec._call('alloc', size)
        if not self.ptr:
            raise MemoryError('wasm allocation failed')
        if not isinstance(init, int) and init:
            codec._memory.write(codec._store, bytes(init), self.ptr)

    def read(self, size):
        return bytes(self._codec._memory.read(self._codec._store, self.ptr, self.ptr + size))

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self._codec._call('free', self.ptr)
