# =======================================================================================
# stamp, shares and edit tests
# =======================================================================================
import os
import shlex
import sys
import tempfile
import unittest
from itertools import combinations
from pathlib import Path

from unittest import mock

from codec_share import (CodecError, InvalidShare, NotEnoughShares, Share, Stamp, WrongStamp,
                         edit, join, load, save, split, split_file)

DATA = b'credentials:\n  aa:\n    user: aa-user\n    pass: aa-pass\n'


def editor(code):
    """editor command running python code on the file path (sys.argv[1])"""
    script = Path(tempfile.mkdtemp()) / 'editor.py'
    script.write_text(f'import sys\npath = sys.argv[1]\n{code}\n')
    return shlex.join([sys.executable, str(script)])


class StampTest(unittest.TestCase):
    def test_pin(self):
        self.assertEqual(Stamp.from_pin('1234'), Stamp.from_pin('1234'))
        self.assertNotEqual(Stamp.from_pin('1234'), Stamp.from_pin('1235'))
        with self.assertRaises(WrongStamp):
            Stamp.from_pin('')

    def test_file(self):
        path = Path(tempfile.mkdtemp()) / 'my.stamp'
        stamp = Stamp.random()
        stamp.save(path)
        self.assertEqual(Stamp.load(path), stamp)
        self.assertEqual(os.stat(path).st_mode & 0o777, 0o600)
        path.write_bytes(b'nope')
        with self.assertRaises(WrongStamp):
            Stamp.load(path)


class SharesTest(unittest.TestCase):
    def test_any_needed_shares_join(self):
        stamp = Stamp.from_pin('1234')
        shares = split(DATA, stamp, total=4, needed=2)
        self.assertEqual([s.index for s in shares], [1, 2, 3, 4])
        for group in combinations(shares, 2):
            self.assertEqual(join(group, stamp), DATA)

    def test_share_bytes(self):
        share = split(DATA, Stamp.random())[0]
        self.assertEqual(Share.parse(bytes(share)), share)
        self.assertTrue(Share.is_share(bytes(share)))
        with self.assertRaises(InvalidShare):
            Share.parse(b'not a share')

    def test_shares_hide_the_data(self):
        for share in split(DATA, Stamp.random()):
            self.assertNotIn(b'pass', bytes(share))

    def test_wrong_pin(self):
        # a wrong stamp opens now and then (the coding is not encryption), never wrongly;
        # some splits open with many stamps, so count over several splits
        failures = tries = 0
        for _ in range(5):
            shares = split(DATA, Stamp.from_pin('1234'), 2, 2)
            for seed in range(20):
                tries += 1
                try:
                    self.assertEqual(join(shares, Stamp.from_seed(seed)), DATA)
                except WrongStamp as error:
                    self.assertIn('wrong pin or stamp', str(error))
                    failures += 1
        self.assertGreater(failures, tries // 2)

    def test_missing_shares(self):
        shares = split(DATA, Stamp.random(), 3, 3)
        with self.assertRaises(NotEnoughShares):
            join(shares[:2] + shares[:1], Stamp.random())
        with self.assertRaises(NotEnoughShares):
            join([], Stamp.random())

    def test_different_splits(self):
        stamp = Stamp.random()
        with self.assertRaises(InvalidShare):
            join([split(DATA, stamp, 2, 2)[0], split(DATA, stamp, 2, 2)[1]], stamp)

    def test_invalid_split(self):
        for total, needed in [(2, 3), (3, 0), (20, 17)]:
            with self.assertRaises(Exception):
                split(DATA, Stamp.random(), total, needed)

    def test_files(self):
        stamp = Stamp.random()
        paths = save(split(DATA, stamp), Path(tempfile.mkdtemp()) / 'config.yml')
        self.assertEqual([p.name for p in paths],
                         ['config.yml.1.share', 'config.yml.2.share', 'config.yml.3.share'])
        self.assertEqual(os.stat(paths[0]).st_mode & 0o777, 0o600)
        self.assertEqual(join(load(paths[1:]), stamp), DATA)


class SplitFileTest(unittest.TestCase):
    def setUp(self):
        self.folder = Path(tempfile.mkdtemp())
        self.file = self.folder / 'config.yml'
        self.file.write_bytes(DATA)
        self.stamp = Stamp.random()

    def test_file_becomes_a_folder(self):
        paths = split_file(self.file, self.stamp, total=4, needed=3)
        self.assertEqual(paths, [self.file / f'{i}.share' for i in (1, 2, 3, 4)])
        self.assertTrue(self.file.is_dir())
        self.assertEqual(join(load([self.file]), self.stamp), DATA)
        for group in combinations(paths, 3):
            self.assertEqual(join(load(group), self.stamp), DATA)
        self.assertEqual([p.name for p in self.folder.iterdir()], ['config.yml'])

    def test_failed_check_keeps_the_file(self):
        with mock.patch('codec_share.shares.join', return_value=b'not the data'):
            with self.assertRaisesRegex(CodecError, 'nothing removed'):
                split_file(self.file, self.stamp)
        self.assertEqual(self.file.read_bytes(), DATA)
        self.assertEqual([p.name for p in self.folder.iterdir()], ['config.yml'])

    def test_failed_write_keeps_the_file(self):
        with mock.patch.object(Share, 'save', side_effect=OSError('disk full')):
            with self.assertRaises(OSError):
                split_file(self.file, self.stamp)
        self.assertEqual(self.file.read_bytes(), DATA)
        self.assertEqual([p.name for p in self.folder.iterdir()], ['config.yml'])

    def test_original_is_wiped(self):
        wiped = []
        with mock.patch('codec_share.shares.wipe', side_effect=lambda p: (wiped.append(p),
                                                                      Path(p).unlink())):
            split_file(self.file, self.stamp)
        self.assertEqual(wiped, [self.file])

    def test_folder_keeps_the_file(self):
        out = self.folder / 'out'
        split_file(self.file, self.stamp, folder=out)
        self.assertEqual(self.file.read_bytes(), DATA)
        self.assertEqual(join(load([out]), self.stamp), DATA)

    def test_already_split(self):
        split_file(self.file, self.stamp)
        with self.assertRaisesRegex(InvalidShare, 'already split'):
            split_file(self.file, self.stamp)

    def test_empty_folder(self):
        (self.folder / 'empty').mkdir()
        with self.assertRaisesRegex(InvalidShare, 'no shares'):
            load([self.folder / 'empty'])

    def test_edit_a_folder(self):
        split_file(self.file, self.stamp)
        seen = self.folder / 'seen'
        written = edit([self.file / '2.share', self.file / '3.share'], self.stamp, editor(
            f"open({str(seen)!r}, 'w').write(path); open(path, 'ab').write(b'x')"),
            echo=lambda _: None)
        self.assertEqual(sorted(written), [self.file / f'{i}.share' for i in (1, 2, 3)])
        self.assertEqual(Path(seen.read_text()).name, 'config.yml')
        self.assertEqual(join(load([self.file]), self.stamp), DATA + b'x')


class EditTest(unittest.TestCase):
    def setUp(self):
        self.folder = Path(tempfile.mkdtemp())
        self.stamp = Stamp.random()
        self.paths = save(split(DATA, self.stamp), self.folder / 'config.yml')
        self.said = []

    def edit(self, code, paths=None, validate=None, again=False):
        return edit(paths or self.paths[:2], self.stamp, editor(code), validate,
                    echo=self.said.append, ask=lambda _: again)

    def test_edit_updates_all_shares(self):
        written = self.edit("open(path, 'ab').write(b'extra: 1\\n')")
        self.assertEqual(sorted(written), sorted(self.paths))
        for group in combinations(self.paths, 2):
            self.assertEqual(join(load(group), self.stamp), DATA + b'extra: 1\n')
        self.assertEqual(sorted(p.name for p in self.folder.iterdir()),
                         ['config.yml.1.share', 'config.yml.2.share', 'config.yml.3.share'])

    def test_edit_removes_the_plain_file(self):
        seen = self.folder / 'seen'
        self.edit(f"open({str(seen)!r}, 'w').write(path); open(path, 'ab').write(b'x')")
        plain = Path(seen.read_text())
        self.assertEqual(plain.name, 'config.yml')
        self.assertFalse(plain.exists())
        self.assertFalse(plain.parent.exists())
        seen.unlink()

    def test_edit_without_changes_keeps_shares(self):
        before = [bytes(s) for s in load(self.paths)]
        self.assertEqual(self.edit('pass'), [])
        self.assertEqual([bytes(s) for s in load(self.paths)], before)
        self.assertIn('no changes, shares kept', self.said)

    def test_edit_invalid_is_dropped(self):
        def validate(data):
            if b'bad' in data:
                raise ValueError('bad content')
        before = [bytes(s) for s in load(self.paths)]
        self.assertEqual(self.edit("open(path, 'ab').write(b'bad')", validate=validate), [])
        self.assertEqual([bytes(s) for s in load(self.paths)], before)
        self.assertIn('changes dropped, shares kept', self.said)

    def test_edit_renamed_shares_warns_stale(self):
        renamed = [self.folder / 'a', self.folder / 'b']
        for source, target in zip(self.paths[:2], renamed):
            os.rename(source, target)
        written = self.edit("open(path, 'ab').write(b'x')", paths=renamed)
        self.assertEqual(sorted(written), sorted(renamed))
        self.assertTrue(any('3 were not given' in said for said in self.said))
        self.assertEqual(join(load(renamed), self.stamp), DATA + b'x')

    def test_edit_wrong_stamp(self):
        before = [bytes(s) for s in load(self.paths)]
        failures = 0
        for seed in range(10):
            try:
                edit(self.paths[:2], Stamp.from_seed(seed), editor('pass'), echo=self.said.append)
            except WrongStamp:
                failures += 1
        self.assertGreater(failures, 5)
        self.assertEqual([bytes(s) for s in load(self.paths)], before)


if __name__ == '__main__':
    unittest.main()
