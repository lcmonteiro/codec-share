# =======================================================================================
# codec-share.wasm tests:  python -m unittest discover -s wasm
#   tests $CODEC_SHARE_WASM, by default the committed wasm/codec_share/codec-share.wasm
# =======================================================================================
import os
import unittest
from itertools import combinations

from codec_share import WASM as DEFAULT_WASM
from codec_share import Codec, CodecError, SharesError, StampError

WASM = os.environ.get('CODEC_SHARE_WASM', DEFAULT_WASM)


class CodecShareTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.codec = Codec(WASM)
        cls.stamp = cls.codec.stamp(1234)

    def test_stamp_is_deterministic(self):
        self.assertEqual(self.codec.stamp(1234), self.stamp)
        self.assertNotEqual(self.codec.stamp(1235), self.stamp)

    def test_any_split_frames_open(self):
        data = os.urandom(1000)
        for split, count in [(1, 1), (1, 3), (2, 3), (3, 5), (5, 5), (16, 16)]:
            frames = self.codec.seal(self.stamp, data, split, count)
            self.assertEqual(len(frames), count)
            for subset in combinations(frames, split):
                try:
                    self.assertEqual(self.codec.open(self.stamp, subset, split), data)
                except SharesError:
                    pass  # dependent frames are possible, but never all of them
            self.assertEqual(self.codec.open(self.stamp, frames, split), data)

    def test_sizes(self):
        for size in [0, 1, 3, 4, 5, 11, 12, 13, 4096]:
            data = os.urandom(size)
            frames = self.codec.seal(self.stamp, data, 3, 4)
            self.assertEqual(self.codec.open(self.stamp, frames, 3), data)

    def test_frames_merge_all_data(self):
        data = os.urandom(120)
        for frame in self.codec.seal(self.stamp, data, 3, 6):
            self.assertNotIn(frame[12:44], data)

    def test_missing_frames(self):
        frames = self.codec.seal(self.stamp, b'settings', 3, 4)
        with self.assertRaises(SharesError):
            self.codec.open(self.stamp, frames[:2], 3)

    def test_wrong_stamp(self):
        # a wrong stamp may still produce the same coefficients, but never wrong data
        data = os.urandom(160)
        frames = self.codec.seal(self.stamp, data, 2, 2)
        failures = 0
        for seed in range(1, 50):
            try:
                self.assertEqual(self.codec.open(self.codec.stamp(seed), frames, 2), data)
            except CodecError:  # StampError, or SharesError for singular coefficients
                failures += 1
        self.assertGreater(failures, 0)

    def test_corrupted_frame(self):
        frames = self.codec.seal(self.stamp, b'settings' * 20, 2, 2)
        frames[0] = bytes([frames[0][0] ^ 1]) + frames[0][1:]
        with self.assertRaises(StampError):
            self.codec.open(self.stamp, frames, 2)

    def test_invalid(self):
        with self.assertRaises(StampError):
            self.codec.seal(b'\0' * 512, b'x', 1, 1)
        with self.assertRaises(StampError):
            self.codec.seal(b'\0' * 10, b'x', 1, 1)
        with self.assertRaises(CodecError):
            self.codec.seal(self.stamp, b'x', 3, 2)
        with self.assertRaises(CodecError):
            self.codec.seal(self.stamp, b'x', 17, 17)


if __name__ == '__main__':
    unittest.main()
