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
#   tools reuse the commands under their own name, pin variable and file validation:
#   commands(name='mytool-share', pin_envvar='MYTOOL_PIN', validate=check)
# =======================================================================================
import os
import sys
from functools import wraps
from pathlib import Path

import click

from . import shares as _shares
from .codec import CodecError
from .edit import edit as _edit
from .stamp import Stamp

PIN_ENVVAR = 'CODEC_SHARE_PIN'


def ask_stamp(stamp_file=None, pin_envvar=PIN_ENVVAR, confirm=False):
    """the stamp of a stamp file, else of the pin in $pin_envvar, else of an asked pin"""
    if stamp_file:
        return Stamp.load(stamp_file)
    pin = os.environ.get(pin_envvar)
    if pin is None:
        pin = click.prompt('pin', hide_input=True, confirmation_prompt=confirm, err=True)
    return Stamp.from_pin(pin)


def commands(name='codec-share', pin_envvar=PIN_ENVVAR, validate=None, what='file'):
    """
    the click group of the share commands
      validate  validate(data) raises when data is not a valid file, before split and save
      what      what the file is, for the help texts
    """
    stamp_option = click.option(
        '--stamp', 'stamp_file', metavar='FILE',
        type=click.Path(exists=True, dir_okay=False, path_type=Path),
        help=f'Stamp file of the shares, instead of a pin (asked, or ${pin_envvar}).')
    shares_argument = click.argument(
        'paths', metavar='SHARES...', nargs=-1, required=True,
        type=click.Path(exists=True, dir_okay=False, path_type=Path))

    @click.group(name=name, context_settings={'help_option_names': ['-h', '--help']},
                 help=f'Split a {what} in coded shares: any NEEDED of the TOTAL shares, '
                      f'with the same pin or stamp, give it back.')
    def group():
        pass

    @group.command()
    @click.argument('output', type=click.Path(dir_okay=False, path_type=Path))
    @_handle_errors
    def stamp(output):
        """Create a random stamp file (to use instead of a pin)."""
        click.echo(Stamp.random().save(output))

    @group.command(help=f'Split a {what} in shares.')
    @click.argument('file', type=click.Path(exists=True, dir_okay=False, path_type=Path))
    @click.option('-n', '--total', default=3, show_default=True, help='Number of shares.')
    @click.option('-k', '--needed', default=2, show_default=True,
                  help='Number of shares needed to join.')
    @click.option('-o', '--output', 'prefix', metavar='PREFIX',
                  help=f'Shares path prefix, PREFIX.<index>.share  [default: the {what}]')
    @stamp_option
    @_handle_errors
    def split(file, total, needed, prefix, stamp_file):
        data = file.read_bytes()
        _validate(validate, data, what)
        stamp = ask_stamp(stamp_file, pin_envvar, confirm=True)
        for path in _shares.save(_shares.split(data, stamp, total, needed), prefix or file):
            click.echo(path)

    @group.command(help=f'Join shares back in the {what}.')
    @shares_argument
    @click.option('-o', '--output', type=click.Path(dir_okay=False, path_type=Path),
                  help='Write to a file, instead of the standard output.')
    @stamp_option
    @_handle_errors
    def join(paths, output, stamp_file):
        data = _shares.join(_shares.load(paths), ask_stamp(stamp_file, pin_envvar))
        if output:
            output.write_bytes(data)
        else:
            sys.stdout.buffer.write(data)
            sys.stdout.flush()

    @group.command(short_help=f'Edit the {what} of shares in place.',
                   help=f'Edit the {what} of shares in place: once the editor closes with '
                        f'changes, it is split again over all its shares (siblings named '
                        f'<prefix>.<index>.share too); the plain file is always wiped.')
    @shares_argument
    @click.option('-e', '--editor', help='Editor command, returning once the file is closed '
                                         '(e.g. "code --wait")  [default: $VISUAL, $EDITOR]')
    @stamp_option
    @_handle_errors
    def edit(paths, editor, stamp_file):
        stamp = ask_stamp(stamp_file, pin_envvar)
        written = _edit(paths, stamp, editor, validate, echo=click.echo,
                        ask=lambda question: click.confirm(question, default=True, err=True))
        for path in written:
            click.echo(path)

    return group


def _validate(validate, data, what):
    if validate:
        try:
            validate(data)
        except Exception as error:
            raise click.ClickException(f'invalid {what}: {error}') from None


def _handle_errors(command):
    """codec and file errors as clean command errors"""
    @wraps(command)
    def wrapper(*args, **kwargs):
        try:
            return command(*args, **kwargs)
        except click.ClickException:
            raise
        except (CodecError, OSError) as error:
            raise click.ClickException(str(error)) from None
    return wrapper


main = commands()
