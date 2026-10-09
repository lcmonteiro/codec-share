# =======================================================================================
# files: writing secrets readable only by the user
# =======================================================================================
import os
from pathlib import Path


def write_private(path, data):
    """writes data readable only by the user, replacing the file at once"""
    path = Path(path)
    temp = path.with_name(f'.{path.name}.{os.getpid()}.tmp')
    fd = os.open(temp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        with os.fdopen(fd, 'wb') as file:
            file.write(data)
        os.replace(temp, path)
    except BaseException:
        temp.unlink(missing_ok=True)
        raise
    return path


def wipe(path):
    """best effort: overwrites the file before removing it"""
    path = Path(path)
    try:
        size = path.stat().st_size
        with open(path, 'r+b') as file:
            file.write(b'\0' * size)
            file.flush()
            os.fsync(file.fileno())
        path.unlink()
    except FileNotFoundError:
        pass
