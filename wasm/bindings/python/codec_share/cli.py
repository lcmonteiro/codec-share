# =======================================================================================
# cli: the codec-share command (click)
#
#   codec-share stamp my.stamp                            random stamp file
#   codec-share split config.yml [-n 3] [-k 2]            config.yml.1.share ...
#   codec-share join config.yml.1.share config.yml.3.share
#   codec-share edit config.yml.1.share config.yml.3.share
#
#   the pin is asked, or read from $CODEC_SHARE_PIN, unless --stamp is given.
#
#   tools run the same command under their own name, pin variable and file validation:
#   main(prog_name='mytool-share', obj=Options(pin_envvar='MYTOOL_PIN', validate=check))
# =======================================================================================
import os
import sys
from dataclasses import dataclass
from functools import wraps
from pathlib import Path
from typing import Callable, Optional

import click

from . import shares as _shares
from .codec import CodecError
from .edit import edit as _edit
from .stamp import Stamp

PIN_ENVVAR = 'CODEC_SHARE_PIN'


@dataclass
class Options:
    """what a tool running the command can change"""
    pin_envvar: str = PIN_ENVVAR
    # validate(data) raises when data is not a valid file, before split and edit save it
    validate: Optional[Callable[[bytes], None]] = None


def ask_stamp(stamp_file=None, pin_envvar=PIN_ENVVAR, confirm=False):
    """the stamp of a stamp file, else of the pin in $pin_envvar, else of an asked pin"""
    if stamp_file:
        return Stamp.load(stamp_file)
    pin = os.environ.get(pin_envvar)
    if pin is None:
        pin = click.prompt('pin', hide_input=True, confirmation_prompt=confirm, err=True)
    return Stamp.from_pin(pin)


# =======================================================================================
# helpers
# =======================================================================================
def _errors(command):
    """codec, file and validation errors as clean command errors"""
    @wraps(command)
    def wrapper(*args, **kwargs):
        try:
            return command(*args, **kwargs)
        except click.ClickException:
            raise
        except (CodecError, OSError) as error:
            raise click.ClickException(str(error)) from None
    return wrapper


def _validate(options, data):
    if options.validate:
        try:
            options.validate(data)
        except Exception as error:
            raise click.ClickException(f'invalid file: {error}') from None


def _stamp(options, stamp_file, confirm=False):
    return ask_stamp(stamp_file, options.pin_envvar, confirm)


_stamp_option = click.option(
    '--stamp', 'stamp_file', metavar='FILE',
    type=click.Path(exists=True, dir_okay=False, path_type=Path),
    help='Stamp file of the shares, instead of the pin.')

_shares_argument = click.argument(
    'paths', metavar='SHARES...', nargs=-1, required=True,
    type=click.Path(exists=True, dir_okay=False, path_type=Path))


# =======================================================================================
# commands
# =======================================================================================
@click.group(context_settings={'help_option_names': ['-h', '--help']})
@click.pass_context
def main(context):
    """Split a file in coded shares: any NEEDED of the TOTAL shares, with the same pin or
    stamp, give it back."""
    context.ensure_object(Options)


@main.command()
@click.argument('output', type=click.Path(dir_okay=False, path_type=Path))
@_errors
def stamp(output):
    """Create a random stamp file (to use instead of a pin)."""
    click.echo(Stamp.random().save(output))


@main.command()
@click.argument('file', type=click.Path(exists=True, dir_okay=False, path_type=Path))
@click.option('-n', '--total', default=3, show_default=True, help='Number of shares.')
@click.option('-k', '--needed', default=2, show_default=True,
              help='Number of shares needed to join.')
@click.option('-o', '--output', 'prefix', metavar='PREFIX',
              help='Shares path prefix, PREFIX.<index>.share  [default: FILE]')
@_stamp_option
@click.pass_obj
@_errors
def split(options, file, total, needed, prefix, stamp_file):
    """Split a file in shares."""
    data = file.read_bytes()
    _validate(options, data)
    stamp = _stamp(options, stamp_file, confirm=True)
    for path in _shares.save(_shares.split(data, stamp, total, needed), prefix or file):
        click.echo(path)


@main.command()
@_shares_argument
@click.option('-o', '--output', type=click.Path(dir_okay=False, path_type=Path),
              help='Write to a file, instead of the standard output.')
@_stamp_option
@click.pass_obj
@_errors
def join(options, paths, output, stamp_file):
    """Join shares back in the file."""
    data = _shares.join(_shares.load(paths), _stamp(options, stamp_file))
    if output:
        output.write_bytes(data)
    else:
        sys.stdout.buffer.write(data)
        sys.stdout.flush()


@main.command(short_help='Edit the file of shares in place.')
@_shares_argument
@click.option('-e', '--editor', help='Editor command, returning once the file is closed '
                                     '(e.g. "code --wait")  [default: $VISUAL, $EDITOR]')
@_stamp_option
@click.pass_obj
@_errors
def edit(options, paths, editor, stamp_file):
    """Edit the file of shares in place: once the editor closes with changes, it is split
    again over all its shares (siblings named <prefix>.<index>.share too); the plain file
    is always wiped."""
    written = _edit(paths, _stamp(options, stamp_file), editor, options.validate,
                    echo=click.echo,
                    ask=lambda question: click.confirm(question, default=True, err=True))
    for path in written:
        click.echo(path)
