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
"""The pw_tokenizer package.

Installing pw_tokenizer with this setup.py does not include the
pw_tokenizer.proto package, since it contains a generated protobuf module. To
access pw_tokenizer.proto, install pw_tokenizer from GN.
"""

import setuptools  # type: ignore

setuptools.setup(
    name='pw_tokenizer',
    version='0.0.1',
    author='Pigweed Authors',
    author_email='pigweed-developers@googlegroups.com',
    description='Tools for working with tokenized strings',
    packages=['pw_tokenizer'],
    package_data={'pw_tokenizer': ['py.typed']},
    zip_safe=False,
    extra_requires=['serial'],
)
