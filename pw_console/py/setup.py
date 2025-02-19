# Copyright 2021 The Pigweed Authors
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
"""pw_console"""

import setuptools  # type: ignore

setuptools.setup(
    name='pw_console',
    version='0.0.1',
    author='Pigweed Authors',
    author_email='pigweed-developers@googlegroups.com',
    description='Pigweed interactive console',
    packages=setuptools.find_packages(),
    package_data={
        'pw_console': [
            'py.typed',
            'templates/keybind_list.jinja',
        ]
    },
    zip_safe=False,
    entry_points={
        'console_scripts': [
            'pw-console = pw_console.__main__:main',
        ]
    },
    install_requires=[
        'ipdb',
        'ipython',
        'jinja2',
        'prompt_toolkit',
        'ptpython',
        'pw_cli',
        'pw_tokenizer',
        'pygments',
    ],
)
