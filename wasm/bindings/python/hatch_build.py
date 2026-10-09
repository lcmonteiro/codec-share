# ships the committed wasm/codec-share.wasm inside the codec_share package
from pathlib import Path

from hatchling.builders.hooks.plugin.interface import BuildHookInterface

PACKAGED = 'codec_share/codec-share.wasm'


class WasmHook(BuildHookInterface):
    def initialize(self, version, build_data):
        root = Path(self.root)
        committed = root.parents[1] / 'codec-share.wasm'  # wasm/bindings/python -> wasm/
        if committed.exists():
            build_data['force_include'][str(committed)] = PACKAGED
        elif not (root / PACKAGED).exists():  # built from an sdist, it is already there
            raise FileNotFoundError(f'{committed} not found, build it with wasm/build.sh')
