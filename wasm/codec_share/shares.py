# =======================================================================================
# codec-share share files
#
#   a file is coded in n share files, any k of them together with the stamp (a stamp
#   file, or derived from a pin) give the file back.
#
#   share file: magic | k | n | index | 0 | split id (8 bytes) | coded frame
#
#   codec-share stamp my.stamp                  random stamp file
#   codec-share split file [-n 3] [-k 2]        file.1.share ... file.n.share
#   codec-share join file.1.share file.3.share  the file, on stdout
#   codec-share edit file.1.share file.3.share  opens the file in an editor, and when the
#                                               editor closes with changes, splits it again
#                                               over the shares; the plain file is removed
#
#   the pin is asked, or read from $CODEC_SHARE_PIN, unless --stamp is given.
#
#   the coding hides the file from a casual reader, it is NOT encryption: keep the shares
#   apart and private, a share alone does not open, all together with a weak pin can be
#   brute forced.
# =======================================================================================
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
from argparse import ArgumentParser
from getpass import getpass
from hashlib import pbkdf2_hmac
from itertools import combinations
from math import comb
from pathlib import Path

from codec_share import Codec, CodecError, SharesError, StampError

# =======================================================================================
# definitions
# =======================================================================================
MAGIC = b'CSS1'
HEADER = struct.Struct('<4sBBBx8s')
PIN_SALT = b'codec-share/stamp'
PIN_ROUNDS = 200_000
PIN_ENV = 'CODEC_SHARE_PIN'
MAX_CHECKS = 500
MAX_RETRIES = 10
SHARE_PATH = re.compile(r'^(?P<prefix>.+)\.(?P<index>\d+)\.share$')

__all__ = [
    'ShareError', 'CodecError', 'SharesError', 'StampError',
    'stamp_from_pin', 'stamp_from_file', 'new_stamp',
    'is_share', 'split', 'join', 'read_shares', 'write_shares', 'edit',
    'load_stamp', 'add_stamp_arguments', 'main',
]

_codec = None


def codec():
    global _codec
    if _codec is None:
        _codec = Codec()
    return _codec


class ShareError(Exception):
    pass


# =======================================================================================
# stamps
# =======================================================================================
def stamp_from_pin(pin):
    seed = pbkdf2_hmac('sha256', pin.encode(), PIN_SALT, PIN_ROUNDS)[:8]
    return codec().stamp(int.from_bytes(seed, 'little'))


def stamp_from_file(path):
    stamp = Path(path).read_bytes()
    if len(stamp) != codec().stamp_size:
        raise ShareError(f'{path} is not a stamp file')
    return stamp


def new_stamp():
    return codec().stamp(int.from_bytes(os.urandom(8), 'little'))


# =======================================================================================
# shares
# =======================================================================================
def is_share(data):
    return data[:len(MAGIC)] == MAGIC


def header(share):
    """(needed, count, index, split id) of a share"""
    if not is_share(share) or len(share) <= HEADER.size:
        raise ShareError('not a share file')
    return HEADER.unpack_from(share)[1:]


def split(data, stamp, count=3, needed=2):
    """codes data in count shares, any needed of them open it"""
    if not 1 <= needed <= count <= Codec.MAX_FRAMES or needed > Codec.MAX_SPLIT:
        raise ShareError(
            f'needs 1 <= needed <= {Codec.MAX_SPLIT} and needed <= shares <= {Codec.MAX_FRAMES}')
    for _ in range(MAX_RETRIES):
        frames = codec().seal(stamp, data, needed, count)
        if _verify(data, stamp, frames, needed):
            break
    else:
        raise ShareError('could not code independent shares, try another stamp')
    uid = os.urandom(8)
    return [HEADER.pack(MAGIC, needed, count, i, uid) + f for i, f in enumerate(frames, 1)]


def join(shares, stamp):
    """opens the data from shares"""
    frames, split_id = {}, None
    for share in shares:
        needed, count, index, uid = header(share)
        if split_id and split_id != (needed, count, uid):
            raise ShareError('shares from different splits')
        split_id = (needed, count, uid)
        frames[index] = share[HEADER.size:]
    if not split_id:
        raise ShareError('no shares')
    needed, count, _ = split_id
    if len(frames) < needed:
        raise SharesError(f'{needed} of {count} shares are needed, got {len(frames)}')
    try:
        return codec().open(stamp, frames.values(), needed)
    except (SharesError, StampError):
        # split verifies the shares are independent, so both come from a wrong stamp
        raise StampError('shares do not open, wrong pin or stamp') from None


def _verify(data, stamp, frames, needed):
    groups = (combinations(frames, needed)
              if comb(len(frames), needed) <= MAX_CHECKS else [frames])
    try:
        return all(codec().open(stamp, group, needed) == data for group in groups)
    except CodecError:
        return False


# =======================================================================================
# share files
# =======================================================================================
def read_shares(paths):
    return [Path(path).read_bytes() for path in paths]


def write_shares(prefix, shares):
    """writes the shares in <prefix>.<index>.share, returns the paths"""
    paths = [f'{prefix}.{i}.share' for i in range(1, len(shares) + 1)]
    for path, share in zip(paths, shares):
        _write_private(path, share)
    return paths


def edit(paths, stamp, editor=None, validate=None, log=print):
    """
    opens the file of the shares in an editor, and once the editor is closed, when the
    file changed, splits it again over the shares (same k and n). The plain file is
    removed in any case. Returns the written share paths, empty when nothing changed.
    """
    shares = read_shares(paths)
    data = join(shares, stamp)
    needed, count, _, _ = header(shares[0])
    targets = _share_paths(paths, shares, count)

    folder = Path(tempfile.mkdtemp(prefix='codec-share-', dir=_private_tmp()))
    plain = folder / _plain_name(paths)
    try:
        _write_private(plain, data)
        while True:
            _run_editor(editor, plain)
            changed = plain.read_bytes()
            if changed == data:
                log('no changes, shares kept')
                return []
            try:
                if validate:
                    validate(changed)
                break
            except Exception as error:
                if not _ask(f'invalid file: {error}\nedit again? [Y/n] '):
                    log('changes dropped, shares kept')
                    return []
        written = []
        for index, share in enumerate(split(changed, stamp, count, needed), 1):
            if index in targets:
                _write_private(targets[index], share)
                written.append(str(targets[index]))
        missing = sorted(set(range(1, count + 1)) - set(targets))
        if missing:
            log(f'shares {", ".join(map(str, missing))} were not given and are now stale: '
                f'split the file again to replace them')
        return written
    finally:
        _wipe(plain)
        shutil.rmtree(folder, ignore_errors=True)


def _share_paths(paths, shares, count):
    """index -> path, for the given shares and the siblings named <prefix>.<index>.share"""
    targets, prefixes = {}, set()
    for path, share in zip(paths, shares):
        index = header(share)[2]
        targets[index] = Path(path)
        match = SHARE_PATH.match(str(path))
        if match and int(match['index']) == index:
            prefixes.add(match['prefix'])
    if len(prefixes) == 1:
        prefix = prefixes.pop()
        for index in range(1, count + 1):
            targets.setdefault(index, Path(f'{prefix}.{index}.share'))
    return targets


def _plain_name(paths):
    match = SHARE_PATH.match(Path(paths[0]).name)
    return match['prefix'] if match else 'shares.txt'


def _private_tmp():
    """memory backed temporary folder when there is one"""
    for candidate in (os.environ.get('XDG_RUNTIME_DIR'), '/dev/shm'):
        if candidate and os.path.isdir(candidate) and os.access(candidate, os.W_OK):
            return candidate
    return None


def _run_editor(editor, path):
    command = editor or os.environ.get('VISUAL') or os.environ.get('EDITOR')
    if not command:
        command = 'notepad' if os.name == 'nt' else next(
            (e for e in ('nano', 'vi') if shutil.which(e)), 'vi')
    # the editor must not return before the file is closed (e.g. "code --wait")
    subprocess.run([*shlex.split(command, posix=os.name != 'nt'), str(path)], check=True)


def _ask(question):
    try:
        return input(question).strip().lower() in ('', 'y', 'yes')
    except EOFError:
        return False


def _wipe(path):
    """best effort: overwrite the plain file before removing it"""
    try:
        size = path.stat().st_size
        with open(path, 'r+b') as ss:
            ss.write(b'\0' * size)
            ss.flush()
            os.fsync(ss.fileno())
        path.unlink()
    except FileNotFoundError:
        pass


def _write_private(path, data):
    """writes data readable only by the user, replacing the file at once"""
    path = Path(path)
    temp = path.with_name(f'.{path.name}.{os.getpid()}.tmp')
    fd = os.open(temp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        with os.fdopen(fd, 'wb') as ss:
            ss.write(data)
        os.replace(temp, path)
    except BaseException:
        temp.unlink(missing_ok=True)
        raise


# =======================================================================================
# command line
# =======================================================================================
def load_stamp(arguments, confirm=False, pin_env=PIN_ENV):
    """stamp from --stamp, else from the pin in $pin_env, else asked"""
    if arguments.stamp:
        return stamp_from_file(arguments.stamp)
    pin = os.environ.get(pin_env)
    if pin is None:
        pin = getpass('pin: ')
        if confirm and getpass('pin (again): ') != pin:
            raise ShareError('pins do not match')
    if not pin:
        raise ShareError('empty pin')
    return stamp_from_pin(pin)


def add_stamp_arguments(parser, pin_env=PIN_ENV, prog='codec-share'):
    parser.add_argument(
        '--stamp', metavar='FILE',
        help=f'stamp file of the shares (see {prog} stamp), '
             f'by default a pin is asked (or read from ${pin_env}).')


def main(args=None, prog='codec-share', pin_env=PIN_ENV, validate=None, what='file'):
    """
    command line, reusable by tools of a given kind of file:
      validate(data) raises when data is not a valid file, before split and edit save
    """
    parser = ArgumentParser(prog=prog, description=f'split a {what} in coded shares.')
    commands = parser.add_subparsers(dest='command', required=True)

    stamp_cmd = commands.add_parser('stamp', help='create a random stamp file.')
    stamp_cmd.add_argument('output', help='stamp file path.')

    split_cmd = commands.add_parser('split', help=f'split a {what} in shares.')
    split_cmd.add_argument('file', help=f'{what} path.')
    split_cmd.add_argument(
        '-n', '--shares', type=int, default=3, help='number of shares (default 3).')
    split_cmd.add_argument(
        '-k', '--needed', type=int, default=2,
        help='number of shares needed to open (default 2).')
    split_cmd.add_argument(
        '-o', '--output', help=f'shares path prefix (default the {what} path).')
    add_stamp_arguments(split_cmd, pin_env, prog)

    join_cmd = commands.add_parser('join', help=f'print the {what} opened from shares.')
    join_cmd.add_argument('shares', nargs='+', help='share file paths.')
    add_stamp_arguments(join_cmd, pin_env, prog)

    edit_cmd = commands.add_parser(
        'edit', help=f'edit the {what} of shares, then split it again over them.')
    edit_cmd.add_argument(
        'shares', nargs='+',
        help='share file paths, the other shares named <prefix>.<index>.share are updated too.')
    edit_cmd.add_argument(
        '-e', '--editor',
        help='editor command, waiting for the file to be closed (default $VISUAL, $EDITOR).')
    add_stamp_arguments(edit_cmd, pin_env, prog)

    arguments = parser.parse_args(args=args)
    try:
        if arguments.command == 'stamp':
            _write_private(arguments.output, new_stamp())
            print(arguments.output)
        elif arguments.command == 'split':
            data = Path(arguments.file).read_bytes()
            if validate:
                validate(data)
            shares = split(
                data, load_stamp(arguments, True, pin_env), arguments.shares, arguments.needed)
            for path in write_shares(arguments.output or arguments.file, shares):
                print(path)
        elif arguments.command == 'join':
            data = join(read_shares(arguments.shares), load_stamp(arguments, False, pin_env))
            sys.stdout.buffer.write(data)
            sys.stdout.flush()
        elif arguments.command == 'edit':
            stamp = load_stamp(arguments, False, pin_env)
            for path in edit(arguments.shares, stamp, arguments.editor, validate):
                print(path)
    except Exception as error:
        parser.exit(1, f'{prog}: {error}\n')
