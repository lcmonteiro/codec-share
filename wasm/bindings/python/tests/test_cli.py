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
        return self.folder / f'config.yml.{index}.share'

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
        self.file.write_bytes(b'nope')
        result = self.runner.invoke(main, ['split', str(self.file)], env={'TOOL_PIN': '9'},
                                    **tool)
        self.assertEqual(result.exit_code, 1)
        self.assertIn('invalid file: not settings', result.output)

    def test_python_module(self):
        import subprocess
        result = subprocess.run([sys.executable, '-m', 'codec_share', '--help'],
                                capture_output=True, text=True, env={**os.environ})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('split', result.stdout)


if __name__ == '__main__':
    unittest.main()
