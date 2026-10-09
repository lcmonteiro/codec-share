# =======================================================================================
# codec-share python binding, on the codec-share WebAssembly module
#
#   from codec_share import Stamp, split, join
#
#   stamp  = Stamp.from_pin('4821')                 # or Stamp.random(), Stamp.load(path)
#   shares = split(data, stamp, total=3, needed=2)  # any 2 of the 3 shares ...
#   data   = join(shares[1:], stamp)                # ... give the data back
#
#   save(shares, 'config.yml')                      # config.yml.1.share ... .3.share
#   edit(['config.yml.1.share', 'config.yml.3.share'], stamp)
#
#   the codec-share command (cli) does the same from a shell
# =======================================================================================
from .codec import Codec, CodecError, NotEnoughShares, WrongStamp
from .edit import edit
from .shares import InvalidShare, Share, join, load, save, split
from .stamp import Stamp

__all__ = [
    'Codec', 'Stamp', 'Share',
    'split', 'join', 'save', 'load', 'edit',
    'CodecError', 'NotEnoughShares', 'WrongStamp', 'InvalidShare',
]
