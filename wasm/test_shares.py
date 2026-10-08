# =======================================================================================
# share files tests:  python -m unittest discover -s wasm
# =======================================================================================
import os
import shlex
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from itertools import combinations
from pathlib import Path
from unittest import mock

from codec_share import shares

DATA = b'credentials:\n  aa:\n    user: aa-user\n    pass: aa-pass\n'


def editor(code):
    """editor command running python code on the file path (sys.argv[1])"""
    script = Path(tempfile.mkdtemp()) / 'editor.py'
    script.write_text(f'import sys\npath = sys.argv[1]\n{code}\n')
    return shlex.join([sys.executable, str(script)])


class SharesTest(unittest.TestCase):
    def test_any_needed_shares_open(self):
        stamp = shares.stamp_from_pin('1234')
        parts = shares.split(DATA, stamp, 4, 2)
        for group in combinations(parts, 2):
            self.assertEqual(shares.join(group, stamp), DATA)

    def test_shares_hide_the_data(self):
        for part in shares.split(DATA, shares.new_stamp(), 3, 2):
            self.assertNotIn(b'pass', part)

    def test_pin(self):
        self.assertEqual(shares.stamp_from_pin('1234'), shares.stamp_from_pin('1234'))
        parts = shares.split(DATA, shares.stamp_from_pin('1234'), 2, 2)
        for pin in ('4321', '0', 'wrong'):
            with self.assertRaisesRegex(shares.StampError, 'wrong pin or stamp'):
                shares.join(parts, shares.stamp_from_pin(pin))

    def test_missing_shares(self):
        parts = shares.split(DATA, shares.new_stamp(), 3, 3)
        with self.assertRaises(shares.SharesError):
            shares.join(parts[:2] + parts[:1], shares.new_stamp())

    def test_different_splits(self):
        stamp = shares.new_stamp()
        first = shares.split(DATA, stamp, 2, 2)
        second = shares.split(DATA, stamp, 2, 2)
        with self.assertRaises(shares.ShareError):
            shares.join([first[0], second[1]], stamp)

    def test_invalid_split(self):
        for count, needed in [(2, 3), (3, 0), (20, 17)]:
            with self.assertRaises(shares.ShareError):
                shares.split(DATA, shares.new_stamp(), count, needed)


class EditTest(unittest.TestCase):
    def setUp(self):
        self.folder = Path(tempfile.mkdtemp())
        self.stamp = shares.new_stamp()
        self.paths = shares.write_shares(
            self.folder / 'config.yml', shares.split(DATA, self.stamp, 3, 2))
        self.logs = []

    def edit(self, code, paths=None, validate=None):
        return shares.edit(paths or self.paths[:2], self.stamp, editor(code), validate,
                           self.logs.append)

    def test_edit_updates_all_shares(self):
        written = self.edit("open(path, 'ab').write(b'extra: 1\\n')")
        self.assertEqual(sorted(written), sorted(self.paths))
        for group in combinations(self.paths, 2):
            self.assertEqual(shares.join(shares.read_shares(group), self.stamp),
                             DATA + b'extra: 1\n')
        self.assertEqual(sorted(p.name for p in self.folder.iterdir()),
                         ['config.yml.1.share', 'config.yml.2.share', 'config.yml.3.share'])

    def test_edit_removes_the_plain_file(self):
        seen = self.folder / 'seen'
        self.edit(f"open({str(seen)!r}, 'w').write(path); open(path, 'ab').write(b'x')")
        plain = Path(seen.read_text())
        self.assertEqual(plain.name, 'config.yml')
        self.assertFalse(plain.exists())
        self.assertFalse(plain.parent.exists())

    def test_edit_without_changes_keeps_shares(self):
        before = shares.read_shares(self.paths)
        self.assertEqual(self.edit('pass'), [])
        self.assertEqual(shares.read_shares(self.paths), before)
        self.assertIn('no changes, shares kept', self.logs)

    def test_edit_invalid_is_dropped(self):
        def validate(data):
            if b'bad' in data:
                raise ValueError('bad content')
        before = shares.read_shares(self.paths)
        with mock.patch('builtins.input', return_value='n'):
            self.assertEqual(self.edit("open(path, 'ab').write(b'bad')", validate=validate), [])
        self.assertEqual(shares.read_shares(self.paths), before)

    def test_edit_renamed_shares_warns_stale(self):
        renamed = [self.folder / 'a', self.folder / 'b']
        for src, dst in zip(self.paths[:2], renamed):
            os.rename(src, dst)
        written = self.edit("open(path, 'ab').write(b'x')", paths=renamed)
        self.assertEqual(sorted(written), sorted(map(str, renamed)))
        self.assertTrue(any('3 were not given' in log for log in self.logs))
        self.assertEqual(shares.join(shares.read_shares(renamed), self.stamp), DATA + b'x')

    def test_edit_wrong_stamp(self):
        with self.assertRaises(shares.StampError):
            shares.edit(self.paths[:2], shares.new_stamp(), editor('pass'))


class CommandLineTest(unittest.TestCase):
    def test_command_line(self):
        folder = Path(tempfile.mkdtemp())
        (folder / 'config.yml').write_bytes(DATA)
        stamp = ['--stamp', str(folder / 'my.stamp')]
        with redirect_stdout(StringIO()):
            shares.main(['stamp', str(folder / 'my.stamp')])
            shares.main(['split', str(folder / 'config.yml'), *stamp])
            shares.main(['edit', str(folder / 'config.yml.2.share'), str(folder / 'config.yml.3.share'),
                         '-e', editor("open(path, 'ab').write(b'more: 2\\n')"), *stamp])
        self.assertEqual(os.stat(folder / 'my.stamp').st_mode & 0o777, 0o600)
        self.assertEqual(os.stat(folder / 'config.yml.1.share').st_mode & 0o777, 0o600)
        out = StringIO()
        out.buffer = mock.Mock()
        with redirect_stdout(out):
            shares.main(['join', str(folder / 'config.yml.1.share'),
                         str(folder / 'config.yml.3.share'), *stamp])
        out.buffer.write.assert_called_once_with(DATA + b'more: 2\n')
        with mock.patch.dict(os.environ, {'CODEC_SHARE_PIN': '99'}), self.assertRaises(SystemExit):
            shares.main(['join', str(folder / 'config.yml.1.share'),
                         str(folder / 'config.yml.2.share')])


if __name__ == '__main__':
    unittest.main()
