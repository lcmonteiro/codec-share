# =======================================================================================
# edit: the file of some shares, edited in place
#
#   edit(['config.yml.1.share', 'config.yml.3.share'], stamp)
#
#   the file is joined in a private temporary folder (memory backed when there is one) and
#   opened in an editor. When the editor closes with changes, the file is split again over
#   all the shares of the split (same needed and total), and the plain file is wiped and
#   removed in any case. The editor must only return once the file is closed (code --wait).
# =======================================================================================
import os
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path

from . import shares as _shares
from .files import wipe, write_private


def edit(paths, stamp, editor=None, validate=None, echo=print, ask=None):
    """
    edits the file of the shares at paths, returns the rewritten share paths (empty when
    nothing changed)
      editor    editor command, default $VISUAL, $EDITOR, else notepad / nano / vi
      validate  validate(data) raises when the edited file must not be saved
      ask       ask(question) -> bool, to edit again an invalid file (default: input)
    """
    shares = _shares.load(paths)
    data = _shares.join(shares, stamp)
    first = shares[0]
    targets = _shares.siblings(paths, shares)

    folder = Path(tempfile.mkdtemp(prefix='codec-share-', dir=_private_tmp()))
    plain = folder / _plain_name(paths)
    try:
        write_private(plain, data)
        while True:
            _open_editor(editor, plain)
            edited = plain.read_bytes()
            if edited == data:
                echo('no changes, shares kept')
                return []
            try:
                if validate:
                    validate(edited)
                break
            except Exception as error:
                if not (ask or _ask)(f'invalid file: {error}\nedit again?'):
                    echo('changes dropped, shares kept')
                    return []
        written = [share.save(targets[share.index])
                   for share in _shares.split(edited, stamp, first.total, first.needed)
                   if share.index in targets]
        stale = sorted(set(range(1, first.total + 1)) - set(targets))
        if stale:
            echo(f'shares {", ".join(map(str, stale))} were not given and are now stale: '
                 f'split the file again to replace them')
        return written
    finally:
        wipe(plain)
        shutil.rmtree(folder, ignore_errors=True)


def _plain_name(paths):
    match = _shares.SHARE_NAME.match(Path(paths[0]).name)
    return match['prefix'] if match else 'shares.txt'


def _private_tmp():
    """memory backed temporary folder when there is one"""
    for candidate in (os.environ.get('XDG_RUNTIME_DIR'), '/dev/shm'):
        if candidate and os.path.isdir(candidate) and os.access(candidate, os.W_OK):
            return candidate
    return None


def _open_editor(editor, path):
    command = editor or os.environ.get('VISUAL') or os.environ.get('EDITOR')
    if not command:
        command = 'notepad' if os.name == 'nt' else next(
            (name for name in ('nano', 'vi') if shutil.which(name)), 'vi')
    subprocess.run([*shlex.split(command, posix=os.name != 'nt'), str(path)], check=True)


def _ask(question):
    try:
        return input(f'{question} [Y/n] ').strip().lower() in ('', 'y', 'yes')
    except EOFError:
        return False
