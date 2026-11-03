# =======================================================================================
# codec-share command tests
# =======================================================================================
import os
import shlex
import sys
import tempfile
import unittest
from pathlib import Path

from click.testing import CliRunner

from codec_share import Stamp, join, load
from codec_share.cli import Options, main

DATA = b'credentials:\n  aa:\n    pass: aa-pass\n'


def editor(code):
    script = Path(tempfile.mkdtemp()) / 'editor.py'
    script.write_text(f'import sys\npath = sys.argv[1]\n{code}\n')
    return shlex.join([sys.executable, str(script)])


class CommandTest(unittest.TestCase):
    def setUp(self):
        self.folder = Path(tempfile.mkdtemp())
        self.file = self.folder / 'config.yml'
        self.file.write_bytes(DATA)
        self.runner = CliRunner()

    def run_ok(self, *args, **kwargs):
        result = self.runner.invoke(main, list(map(str, args)), **kwargs)
        self.assertEqual(result.exit_code, 0, result.output)
        return result

    def share(self, index):
        return self.file / f'{index}.share'

    def test_stamp_split_edit_join(self):
        stamp = ['--stamp', self.folder / 'my.stamp']
        self.run_ok('stamp', self.folder / 'my.stamp')
        self.run_ok('split', self.file, '-n', 4, '-k', 3, *stamp)
        self.run_ok('edit', self.share(1), self.share(2), self.share(4),
                    '-e', editor("open(path, 'ab').write(b'more: 2\\n')"), *stamp)
        result = self.run_ok('join', self.share(4), self.share(3), self.share(2), *stamp)
        self.assertEqual(result.stdout_bytes, DATA + b'more: 2\n')
        self.run_ok('join', self.share(1), self.share(2), self.share(3),
                    '-o', self.folder / 'out.yml', *stamp)
        self.assertEqual((self.folder / 'out.yml').read_bytes(), DATA + b'more: 2\n')

    def test_pin_prompt_and_envvar(self):
        self.run_ok('split', self.file, input='1234\n1234\n')
        self.assertEqual(join(load([self.share(1), self.share(3)]), Stamp.from_pin('1234')), DATA)
        result = self.run_ok('join', self.share(2), self.share(3),
                             env={'CODEC_SHARE_PIN': '1234'})
        self.assertEqual(result.stdout_bytes, DATA)

    def test_wrong_pin(self):
        self.run_ok('split', self.file, env={'CODEC_SHARE_PIN': '1234'})
        results = [self.runner.invoke(main, ['join', str(self.share(1)), str(self.share(2))],
                                      env={'CODEC_SHARE_PIN': pin}) for pin in '0123456']
        failed = [r for r in results if r.exit_code]
        self.assertGreater(len(failed), 3)  # a wrong pin opens now and then, never wrongly
        for result in failed:
            self.assertEqual(result.exit_code, 1)
            self.assertIn('wrong pin or stamp', result.output)
        for result in results:
            if not result.exit_code:
                self.assertEqual(result.stdout_bytes, DATA)

    def test_tool_options(self):
        def validate(data):
            if not data.startswith(b'credentials'):
                raise ValueError('not settings')
        tool = {'obj': Options(pin_envvar='TOOL_PIN', validate=validate),
                'prog_name': 'tool-share'}
        self.assertIn('Usage: tool-share', self.run_ok('--help', **tool).output)
        self.run_ok('split', self.file, env={'TOOL_PIN': '9'}, **tool)
        self.assertEqual(join(load([self.share(1), self.share(2)]), Stamp.from_pin('9')), DATA)
        bad = self.folder / 'bad.yml'
        bad.write_bytes(b'nope')
        result = self.runner.invoke(main, ['split', str(bad)], env={'TOOL_PIN': '9'}, **tool)
        self.assertEqual(result.exit_code, 1)
        self.assertIn('invalid file: not settings', result.output)

    def test_split_replaces_the_file_by_a_folder(self):
        result = self.run_ok('split', self.file, env={'CODEC_SHARE_PIN': '1'})
        self.assertTrue(self.file.is_dir())
        self.assertEqual(sorted(p.name for p in self.file.iterdir()),
                         ['1.share', '2.share', '3.share'])
        self.assertEqual(result.output.split(), [str(self.share(i)) for i in (1, 2, 3)])
        self.assertEqual(os.stat(self.file).st_mode & 0o777, 0o700)
        self.assertEqual([p.name for p in self.folder.iterdir()], ['config.yml'])
        joined = self.run_ok('join', self.file, env={'CODEC_SHARE_PIN': '1'})
        self.assertEqual(joined.stdout_bytes, DATA)

    def test_split_to_a_folder_keeps_the_file(self):
        out = self.folder / 'out'
        self.run_ok('split', self.file, '-o', out, env={'CODEC_SHARE_PIN': '1'})
        self.assertEqual(self.file.read_bytes(), DATA)
        self.assertEqual(join(load([out]), Stamp.from_pin('1')), DATA)
        result = self.runner.invoke(main, ['split', str(self.file), '-o', str(out)],
                                    env={'CODEC_SHARE_PIN': '1'})
        self.assertEqual(result.exit_code, 1)
        self.assertIn('already exists', result.output)

    def test_split_twice(self):
        self.run_ok('split', self.file, env={'CODEC_SHARE_PIN': '1'})
        result = self.runner.invoke(main, ['split', str(self.file)], env={'CODEC_SHARE_PIN': '1'})
        self.assertNotEqual(result.exit_code, 0)
        self.assertEqual(len(list(self.file.iterdir())), 3)

    def test_edit_a_folder(self):
        pin = {'CODEC_SHARE_PIN': '1'}
        self.run_ok('split', self.file, '-n', 4, '-k', 3, env=pin)
        self.run_ok('edit', self.file, '-e', editor("open(path, 'ab').write(b'x: 1\\n')"), env=pin)
        self.assertEqual(join(load([self.share(4), self.share(1), self.share(2)]),
                              Stamp.from_pin('1')), DATA + b'x: 1\n')

    def test_python_module(self):
        import subprocess
        result = subprocess.run([sys.executable, '-m', 'codec_share', '--help'],
                                capture_output=True, text=True, env={**os.environ})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('split', result.stdout)


if __name__ == '__main__':
    unittest.main()
