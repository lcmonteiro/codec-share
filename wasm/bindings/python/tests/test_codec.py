# =======================================================================================
# codec tests:  python -m unittest discover -s tests   (in wasm/bindings/python)
#   tests $CODEC_SHARE_WASM, by default the committed wasm/codec-share.wasm
# =======================================================================================
import os
import unittest
from itertools import combinations

from codec_share.codec import WASM, Codec, CodecError, NotEnoughShares, WrongStamp


class CodecTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.codec = Codec(os.environ.get('CODEC_SHARE_WASM', WASM))
        cls.stamp = cls.codec.stamp(1234)

    def test_stamp_is_deterministic(self):
        self.assertEqual(self.codec.stamp(1234), self.stamp)
        self.assertNotEqual(self.codec.stamp(1235), self.stamp)

    def test_any_needed_frames_decode(self):
        data = os.urandom(1000)
        for needed, total in [(1, 1), (1, 3), (2, 3), (3, 5), (5, 5), (16, 16)]:
            frames = self.codec.encode(data, self.stamp, needed, total)
            self.assertEqual(len(frames), total)
            for group in combinations(frames, needed):
                try:
                    self.assertEqual(self.codec.decode(group, self.stamp, needed), data)
                except NotEnoughShares:
                    pass  # dependent frames are possible, but never all of them
            self.assertEqual(self.codec.decode(frames, self.stamp, needed), data)

    def test_sizes(self):
        for size in [0, 1, 3, 4, 5, 11, 12, 13, 4096]:
            data = os.urandom(size)
            frames = self.codec.encode(data, self.stamp, 3, 4)
            self.assertEqual(self.codec.decode(frames, self.stamp, 3), data)

    def test_frames_merge_all_data(self):
        data = os.urandom(120)
        for frame in self.codec.encode(data, self.stamp, 3, 6):
            self.assertNotIn(frame[12:44], data)

    def test_missing_frames(self):
        frames = self.codec.encode(b'settings', self.stamp, 3, 4)
        with self.assertRaises(NotEnoughShares):
            self.codec.decode(frames[:2], self.stamp, 3)

    def test_wrong_stamp(self):
        # a wrong stamp may still produce the same coefficients, but never wrong data
        data = os.urandom(160)
        frames = self.codec.encode(data, self.stamp, 2, 2)
        failures = 0
        for seed in range(1, 50):
            try:
                self.assertEqual(self.codec.decode(frames, self.codec.stamp(seed), 2), data)
            except CodecError:
                failures += 1
        self.assertGreater(failures, 0)

    def test_corrupted_frame(self):
        frames = self.codec.encode(b'settings' * 20, self.stamp, 2, 2)
        frames[0] = bytes([frames[0][0] ^ 1]) + frames[0][1:]
        with self.assertRaises(WrongStamp):
            self.codec.decode(frames, self.stamp, 2)

    def test_invalid(self):
        with self.assertRaises(WrongStamp):
            self.codec.encode(b'x', b'\0' * 512, 1, 1)
        with self.assertRaises(WrongStamp):
            self.codec.encode(b'x', b'\0' * 10, 1, 1)
        with self.assertRaises(CodecError):
            self.codec.encode(b'x', self.stamp, 3, 2)
        with self.assertRaises(CodecError):
            self.codec.encode(b'x', self.stamp, 17, 17)


if __name__ == '__main__':
    unittest.main()
