# =======================================================================================
# shares: a file coded in `total` shares, any `needed` of them with the stamp give it back
#
#   shares = split(data, stamp, total=3, needed=2)
#   data   = join(shares[1:], stamp)
#
#   split_file('config.yml', stamp)                # config.yml/ 1.share 2.share 3.share
#   data   = join(load(['config.yml']), stamp)     # a folder stands for its shares
#
#   share file: magic | needed | total | index | 0 | split id (8 bytes) | coded frame
#
#   the coding hides the file from a casual reader, it is NOT encryption: keep the shares
#   apart and private, a share alone does not open, all together with a weak pin can be
#   brute forced.
# =======================================================================================
import os
import re
import shutil
import struct
from dataclasses import dataclass
from itertools import combinations
from math import comb
from pathlib import Path

from .codec import Codec, CodecError, NotEnoughShares, WrongStamp
from .files import wipe, write_private

MAGIC = b'CSS1'
HEADER = struct.Struct('<4sBBBx8s')
MAX_CHECKS = 500
MAX_RETRIES = 10
SHARE_NAME = re.compile(r'^(?:(?P<prefix>.+)\.)?(?P<index>\d+)\.share$')


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
def split_file(path, stamp, total=3, needed=2, folder=None):
    """
    splits the file at path in a folder of shares <index>.share, and returns their paths
      folder  where the shares go, the file is kept; by default the file is replaced by a
              folder of the same name
    the shares are written aside, joined back from disk and compared with the file first:
    the file is only removed once every group of needed shares gives it back
    """
    path = Path(path)
    target = Path(folder) if folder else path
    if path.is_dir():
        raise InvalidShare(f'{path} is a folder, already split?')
    if folder and target.exists():
        raise FileExistsError(f'{target} already exists')
    data = path.read_bytes()
    shares = split(data, stamp, total, needed)

    staging = target.with_name(f'.{target.name}.{os.getpid()}.shares')
    staging.mkdir(mode=0o700)
    try:
        paths = [share.save(staging / f'{share.index}.share') for share in shares]
        _check(data, load(paths), stamp, needed)
    except BaseException:
        shutil.rmtree(staging, ignore_errors=True)
        raise
    if not folder:
        wipe(path)
    try:
        staging.rename(target)
    except OSError as error:
        raise OSError(f'shares kept in {staging}: {error}') from None
    return [target / p.name for p in paths]


def expand(paths):
    """share paths, a folder standing for the shares in it"""
    found = []
    for path in map(Path, paths):
        if path.is_dir():
            inside = sorted(path.glob('*.share'), key=lambda p: (len(p.name), p.name))
            if not inside:
                raise InvalidShare(f'no shares in {path}')
            found.extend(inside)
        else:
            found.append(path)
    return found


def save(shares, prefix):
    """writes the shares in <prefix>.<index>.share, returns the paths"""
    return [share.save(f'{prefix}.{share.index}.share') for share in shares]


def load(paths):
    """shares of files, or of folders of shares"""
    return [Share.load(path) for path in expand(paths)]


def siblings(paths, shares):
    """index -> path of every share of the split: the given ones, and the ones named alike
    next to them (<prefix>.<index>.share, or <index>.share in a folder)"""
    found, patterns = {}, set()
    for path, share in zip(expand(paths), shares):
        path = Path(path)
        found[share.index] = path
        match = SHARE_NAME.match(path.name)
        if match and int(match['index']) == share.index:
            patterns.add((path.parent, match['prefix']))
    if len(patterns) == 1 and shares:
        parent, prefix = patterns.pop()
        for index in range(1, shares[0].total + 1):
            name = f'{prefix}.{index}.share' if prefix else f'{index}.share'
            found.setdefault(index, parent / name)
    return found


def _check(data, shares, stamp, needed):
    """every group of needed shares (up to MAX_CHECKS groups) joins back to data"""
    groups = (combinations(shares, needed)
              if comb(len(shares), needed) <= MAX_CHECKS else [shares])
    for group in groups:
        if join(group, stamp) != data:
            raise CodecError('shares do not join back to the file, nothing removed')
