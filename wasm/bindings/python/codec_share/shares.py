# =======================================================================================
# shares: a file coded in `total` shares, any `needed` of them with the stamp give it back
#
#   shares = split(data, stamp, total=3, needed=2)
#   paths  = save(shares, 'config.yml')           # config.yml.1.share ... config.yml.3.share
#   data   = join(load(paths[1:]), stamp)
#
#   share file: magic | needed | total | index | 0 | split id (8 bytes) | coded frame
#
#   the coding hides the file from a casual reader, it is NOT encryption: keep the shares
#   apart and private, a share alone does not open, all together with a weak pin can be
#   brute forced.
# =======================================================================================
import os
import re
import struct
from dataclasses import dataclass
from itertools import combinations
from math import comb
from pathlib import Path

from .codec import Codec, CodecError, NotEnoughShares, WrongStamp
from .files import write_private

MAGIC = b'CSS1'
HEADER = struct.Struct('<4sBBBx8s')
MAX_CHECKS = 500
MAX_RETRIES = 10
SHARE_NAME = re.compile(r'^(?P<prefix>.+)\.(?P<index>\d+)\.share$')


class InvalidShare(CodecError):
    """not a share, or shares of different splits"""


# =======================================================================================
# share
# =======================================================================================
@dataclass(frozen=True)
class Share:
    needed: int     # shares needed to join
    total: int      # shares of the split
    index: int      # 1 .. total
    split_id: bytes  # same for all the shares of a split
    frame: bytes    # coded frame

    @staticmethod
    def is_share(data):
        return data[:len(MAGIC)] == MAGIC

    @classmethod
    def parse(cls, data):
        if not cls.is_share(data) or len(data) <= HEADER.size:
            raise InvalidShare('not a share')
        _, needed, total, index, split_id = HEADER.unpack_from(data)
        return cls(needed, total, index, split_id, bytes(data[HEADER.size:]))

    @classmethod
    def load(cls, path):
        try:
            return cls.parse(Path(path).read_bytes())
        except InvalidShare:
            raise InvalidShare(f'{path} is not a share') from None

    def save(self, path):
        """writes the share readable only by the user"""
        return write_private(path, bytes(self))

    def __bytes__(self):
        return HEADER.pack(MAGIC, self.needed, self.total, self.index, self.split_id) + self.frame


# =======================================================================================
# split and join
# =======================================================================================
def split(data, stamp, total=3, needed=2):
    """codes data in total shares, any needed of them join it back"""
    if not 1 <= needed <= total <= Codec.MAX_TOTAL or needed > Codec.MAX_NEEDED:
        raise CodecError(f'needs 1 <= needed <= {Codec.MAX_NEEDED} '
                         f'and needed <= total <= {Codec.MAX_TOTAL}')
    codec = Codec.default()
    for _ in range(MAX_RETRIES):
        frames = codec.encode(data, stamp, needed, total)
        if _independent(codec, data, stamp, frames, needed):
            break
    else:
        raise CodecError('could not code independent shares, try another stamp')
    split_id = os.urandom(8)
    return [Share(needed, total, i, split_id, f) for i, f in enumerate(frames, 1)]


def join(shares, stamp):
    """data back from the shares of a split"""
    shares = {share.index: share for share in shares}
    if not shares:
        raise NotEnoughShares('no shares')
    first = next(iter(shares.values()))
    if any((s.needed, s.total, s.split_id) != (first.needed, first.total, first.split_id)
           for s in shares.values()):
        raise InvalidShare('shares of different splits')
    if len(shares) < first.needed:
        raise NotEnoughShares(f'{first.needed} of {first.total} shares are needed, '
                              f'got {len(shares)}')
    try:
        return Codec.default().decode([s.frame for s in shares.values()], stamp, first.needed)
    except (NotEnoughShares, WrongStamp):
        # split checks the shares are independent, so both come from a wrong stamp
        raise WrongStamp('shares do not open, wrong pin or stamp') from None


def _independent(codec, data, stamp, frames, needed):
    groups = (combinations(frames, needed)
              if comb(len(frames), needed) <= MAX_CHECKS else [frames])
    try:
        return all(codec.decode(group, stamp, needed) == data for group in groups)
    except CodecError:
        return False


# =======================================================================================
# share files
# =======================================================================================
def save(shares, prefix):
    """writes the shares in <prefix>.<index>.share, returns the paths"""
    return [share.save(f'{prefix}.{share.index}.share') for share in shares]


def load(paths):
    return [Share.load(path) for path in paths]


def siblings(paths, shares):
    """index -> path of every share of the split: the given ones, and the ones named
    <prefix>.<index>.share next to them"""
    found, prefixes = {}, set()
    for path, share in zip(paths, shares):
        found[share.index] = Path(path)
        match = SHARE_NAME.match(str(path))
        if match and int(match['index']) == share.index:
            prefixes.add(match['prefix'])
    if len(prefixes) == 1 and shares:
        prefix = prefixes.pop()
        for index in range(1, shares[0].total + 1):
            found.setdefault(index, Path(f'{prefix}.{index}.share'))
    return found
