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

from codec_share import (InvalidShare, NotEnoughShares, Share, Stamp, WrongStamp, edit, join,
                         load, save, split)

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
        # a wrong stamp opens now and then (the coding is not encryption), never wrongly
        shares = split(DATA, Stamp.from_pin('1234'), 2, 2)
        failures = 0
        for seed in range(20):
            try:
                self.assertEqual(join(shares, Stamp.from_seed(seed)), DATA)
            except WrongStamp as error:
                self.assertIn('wrong pin or stamp', str(error))
                failures += 1
        self.assertGreater(failures, 10)

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
