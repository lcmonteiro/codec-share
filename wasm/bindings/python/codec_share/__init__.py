# =======================================================================================
# codec-share python binding, on the codec-share WebAssembly module
#
#   from codec_share import Stamp, split, join
#
#   stamp  = Stamp.from_pin('4821')                 # or Stamp.random(), Stamp.load(path)
#   shares = split(data, stamp, total=3, needed=2)  # any 2 of the 3 shares ...
#   data   = join(shares[1:], stamp)                # ... give the data back
#
#   split_file('config.yml', stamp)                # config.yml/ 1.share 2.share 3.share
#   join(load(['config.yml']), stamp)               # a folder stands for its shares
#   edit(['config.yml'], stamp)
#
#   the codec-share command (cli) does the same from a shell
# =======================================================================================
from .codec import Codec, CodecError, NotEnoughShares, WrongStamp
from .edit import edit
from .shares import InvalidShare, Share, expand, join, load, save, split, split_file
from .stamp import Stamp

__all__ = [
    'Codec', 'Stamp', 'Share',
    'split', 'join', 'split_file', 'save', 'load', 'expand', 'edit',
    'CodecError', 'NotEnoughShares', 'WrongStamp', 'InvalidShare',
]
