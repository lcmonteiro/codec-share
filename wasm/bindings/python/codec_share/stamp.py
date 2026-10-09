# =======================================================================================
# stamp: the secret shaping how shares are coded, the same one splits and joins
#
#   Stamp.from_pin('4821')        derived from a pin (PBKDF2-SHA256)
#   Stamp.random().save(path)     random, kept in a stamp file
#   Stamp.load(path)
# =======================================================================================
import os
from hashlib import pbkdf2_hmac
from pathlib import Path

from .codec import Codec, WrongStamp
from .files import write_private

PIN_SALT = b'codec-share/stamp'
PIN_ROUNDS = 200_000


class Stamp(bytes):
    """the raw stamp bytes, as the codec uses them"""

    def __new__(cls, raw):
        size = Codec.default().stamp_size
        if len(raw) != size:
            raise WrongStamp(f'a stamp has {size} bytes, got {len(raw)}')
        return super().__new__(cls, raw)

    @classmethod
    def from_seed(cls, seed):
        return cls(Codec.default().stamp(seed))

    @classmethod
    def from_pin(cls, pin):
        if not pin:
            raise WrongStamp('empty pin')
        seed = pbkdf2_hmac('sha256', pin.encode(), PIN_SALT, PIN_ROUNDS)[:8]
        return cls.from_seed(int.from_bytes(seed, 'little'))

    @classmethod
    def random(cls):
        return cls.from_seed(int.from_bytes(os.urandom(8), 'little'))

    @classmethod
    def load(cls, path):
        try:
            return cls(Path(path).read_bytes())
        except WrongStamp:
            raise WrongStamp(f'{path} is not a stamp file') from None

    def save(self, path):
        """writes the stamp readable only by the user"""
        return write_private(path, self)

    def __repr__(self):
        return f'<Stamp {self[:4].hex()}…>'
