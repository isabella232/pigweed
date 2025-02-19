# Copyright 2020 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Functions for building code during presubmit checks."""

import collections
import contextlib
import itertools
import json
import logging
import os
from pathlib import Path
import re
import subprocess
from typing import (Collection, Container, Dict, Iterable, List, Mapping, Set,
                    Tuple, Union)

from pw_package import package_manager
from pw_presubmit import call, log_run, plural, PresubmitFailure, tools

_LOG = logging.getLogger(__name__)


def install_package(root: Path, name: str) -> None:
    """Install package with given name in given path."""
    mgr = package_manager.PackageManager(root)

    if not mgr.list():
        raise PresubmitFailure(
            'no packages configured, please import your pw_package '
            'configuration module')

    if not mgr.status(name):
        mgr.install(name)


def gn_args(**kwargs) -> str:
    """Builds a string to use for the --args argument to gn gen.

    Currently supports bool, int, and str values. In the case of str values,
    quotation marks will be added automatically, unless the string already
    contains one or more double quotation marks, or starts with a { or [
    character, in which case it will be passed through as-is.
    """
    transformed_args = []
    for arg, val in kwargs.items():
        if isinstance(val, bool):
            transformed_args.append(f'{arg}={str(val).lower()}')
            continue
        if (isinstance(val, str) and '"' not in val and not val.startswith("{")
                and not val.startswith("[")):
            transformed_args.append(f'{arg}="{val}"')
            continue
        # Fall-back case handles integers as well as strings that already
        # contain double quotation marks, or look like scopes or lists.
        transformed_args.append(f'{arg}={val}')
    return '--args=' + ' '.join(transformed_args)


def gn_gen(gn_source_dir: Path,
           gn_output_dir: Path,
           *args: str,
           gn_check: bool = True,
           gn_fail_on_unused: bool = True,
           **gn_arguments) -> None:
    """Runs gn gen in the specified directory with optional GN args."""
    args_option = (gn_args(**gn_arguments), ) if gn_arguments else ()

    # Delete args.gn to ensure this is a clean build.
    args_gn = gn_output_dir / 'args.gn'
    if args_gn.is_file():
        args_gn.unlink()

    call('gn',
         'gen',
         gn_output_dir,
         '--color=always',
         *(['--fail-on-unused-args'] if gn_fail_on_unused else []),
         *args,
         *args_option,
         cwd=gn_source_dir)

    if gn_check:
        call('gn',
             'check',
             gn_output_dir,
             '--check-generated',
             '--check-system',
             cwd=gn_source_dir)


def ninja(directory: Path, *args, **kwargs) -> None:
    """Runs ninja in the specified directory."""
    call('ninja', '-C', directory, *args, **kwargs)


def cmake(source_dir: Path,
          output_dir: Path,
          *args: str,
          env: Mapping['str', 'str'] = None) -> None:
    """Runs CMake for Ninja on the given source and output directories."""
    call('cmake',
         '-B',
         output_dir,
         '-S',
         source_dir,
         '-G',
         'Ninja',
         *args,
         env=env)


def env_with_clang_vars() -> Mapping[str, str]:
    """Returns the environment variables with CC, CXX, etc. set for clang."""
    env = os.environ.copy()
    env['CC'] = env['LD'] = env['AS'] = 'clang'
    env['CXX'] = 'clang++'
    return env


def _get_paths_from_command(source_dir: Path, *args, **kwargs) -> Set[Path]:
    """Runs a command and reads Bazel or GN //-style paths from it."""
    process = log_run(args, capture_output=True, cwd=source_dir, **kwargs)

    if process.returncode:
        _LOG.error('Build invocation failed with return code %d!',
                   process.returncode)
        _LOG.error('[COMMAND] %s\n%s\n%s', *tools.format_command(args, kwargs),
                   process.stderr.decode())
        raise PresubmitFailure

    files = set()

    for line in process.stdout.splitlines():
        path = line.strip().lstrip(b'/').replace(b':', b'/').decode()
        path = source_dir.joinpath(path)
        if path.is_file():
            files.add(path)

    return files


# Finds string literals with '.' in them.
_MAYBE_A_PATH = re.compile(r'"([^\n"]+\.[^\n"]+)"')


def _search_files_for_paths(build_files: Iterable[Path]) -> Iterable[Path]:
    for build_file in build_files:
        directory = build_file.parent

        for string in _MAYBE_A_PATH.finditer(build_file.read_text()):
            path = directory / string.group(1)
            if path.is_file():
                yield path


def _read_compile_commands(compile_commands: Path) -> dict:
    with compile_commands.open('rb') as fd:
        return json.load(fd)


def compiled_files(compile_commands: Path) -> Iterable[Path]:
    for command in _read_compile_commands(compile_commands):
        file = Path(command['file'])
        if file.is_absolute():
            yield file
        else:
            yield file.joinpath(command['directory']).resolve()


def check_compile_commands_for_files(
        compile_commands: Union[Path, Iterable[Path]],
        files: Iterable[Path],
        extensions: Collection[str] = ('.c', '.cc', '.cpp'),
) -> List[Path]:
    """Checks for paths in one or more compile_commands.json files.

    Only checks C and C++ source files by default.
    """
    if isinstance(compile_commands, Path):
        compile_commands = [compile_commands]

    compiled = frozenset(
        itertools.chain.from_iterable(
            compiled_files(cmds) for cmds in compile_commands))
    return [f for f in files if f not in compiled and f.suffix in extensions]


def check_builds_for_files(
        bazel_extensions_to_check: Container[str],
        gn_extensions_to_check: Container[str],
        files: Iterable[Path],
        bazel_dirs: Iterable[Path] = (),
        gn_dirs: Iterable[Tuple[Path, Path]] = (),
        gn_build_files: Iterable[Path] = (),
) -> Dict[str, List[Path]]:
    """Checks that source files are in the GN and Bazel builds.

    Args:
        bazel_extensions_to_check: which file suffixes to look for in Bazel
        gn_extensions_to_check: which file suffixes to look for in GN
        files: the files that should be checked
        bazel_dirs: directories in which to run bazel query
        gn_dirs: (source_dir, output_dir) tuples with which to run gn desc
        gn_build_files: paths to BUILD.gn files to directly search for paths

    Returns:
        a dictionary mapping build system ('Bazel' or 'GN' to a list of missing
        files; will be empty if there were no missing files
    """

    # Collect all paths in the Bazel builds.
    bazel_builds: Set[Path] = set()
    for directory in bazel_dirs:
        bazel_builds.update(
            _get_paths_from_command(directory, 'bazel', 'query',
                                    'kind("source file", //...:*)'))

    # Collect all paths in GN builds.
    gn_builds: Set[Path] = set()

    for source_dir, output_dir in gn_dirs:
        gn_builds.update(
            _get_paths_from_command(source_dir, 'gn', 'desc', output_dir, '*'))

    gn_builds.update(_search_files_for_paths(gn_build_files))

    missing: Dict[str, List[Path]] = collections.defaultdict(list)

    if bazel_dirs:
        for path in (p for p in files
                     if p.suffix in bazel_extensions_to_check):
            if path not in bazel_builds:
                # TODO(pwbug/176) Replace this workaround for fuzzers.
                if 'fuzz' not in str(path):
                    missing['Bazel'].append(path)

    if gn_dirs or gn_build_files:
        for path in (p for p in files if p.suffix in gn_extensions_to_check):
            if path not in gn_builds:
                missing['GN'].append(path)

    for builder, paths in missing.items():
        _LOG.warning('%s missing from the %s build:\n%s',
                     plural(paths, 'file', are=True), builder,
                     '\n'.join(str(x) for x in paths))

    return missing


@contextlib.contextmanager
def test_server(executable: str, output_dir: Path):
    """Context manager that runs a test server executable.

    Args:
        executable: name of the test server executable
        output_dir: path to the output directory (for logs)
    """

    with open(output_dir / 'test_server.log', 'w') as outs:
        try:
            proc = subprocess.Popen(
                [executable, '--verbose'],
                stdout=outs,
                stderr=subprocess.STDOUT,
            )

            yield

        finally:
            proc.terminate()
