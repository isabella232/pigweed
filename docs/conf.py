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
"""Pigweed's Sphinx configuration."""

import sphinx
import sphinx_rtd_theme

# TODO(pwbug/376): Remove this when this PR is merged:
# https://github.com/mgaitan/sphinxcontrib-mermaid/pull/71
# Needed for sphinxcontrib-mermaid compatibility with sphinx 4.0.0.
if sphinx.version_info[0] >= 4:
    import errno
    import sphinx.util.osutil
    sphinx.util.osutil.ENOENT = errno.ENOENT

# The suffix of source filenames.
source_suffix = ['.rst']

# The master toctree document.  # inclusive-language: ignore
master_doc = 'index'

# General information about the project.
project = 'Pigweed'
copyright = '2020 The Pigweed Authors'  # pylint: disable=redefined-builtin

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = '0.1'
# The full version, including alpha/beta/rc tags.
release = '0.1.0'

# The name of the Pygments (syntax highlighting) style to use.
pygm = 'sphinx'

extensions = [
    'sphinx.ext.autodoc',  # Automatic documentation for Python code
    'sphinx.ext.napoleon',  # Parses Google-style docstrings

    # Blockdiag suite of diagram generators.
    'sphinxcontrib.blockdiag',
    'sphinxcontrib.nwdiag',
    'sphinxcontrib.seqdiag',
    'sphinxcontrib.actdiag',
    'sphinxcontrib.rackdiag',
    'sphinxcontrib.packetdiag',
    'sphinxcontrib.mermaid',
]

_DIAG_HTML_IMAGE_FORMAT = 'SVG'
blockdiag_html_image_format = _DIAG_HTML_IMAGE_FORMAT
nwdiag_html_image_format = _DIAG_HTML_IMAGE_FORMAT
seqdiag_html_image_format = _DIAG_HTML_IMAGE_FORMAT
actdiag_html_image_format = _DIAG_HTML_IMAGE_FORMAT
rackdiag_html_image_format = _DIAG_HTML_IMAGE_FORMAT
packetdiag_html_image_format = _DIAG_HTML_IMAGE_FORMAT

# Tell m2r to parse links to .md files and add them to the build.
m2r_parse_relative_links = True

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = [
    '_themes',
]

html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
html_title = 'Pigweed'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
html_use_smartypants = True

# If false, no module index is generated.
html_domain_indices = True

# If false, no index is generated.
html_use_index = True

# If true, the index is split into individual pages for each letter.
html_split_index = False

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = False

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# These folders are copied to the documentation's HTML output
html_static_path = ['docs/_static']

# These paths are either relative to html_static_path
# or fully qualified paths (eg. https://...)
html_css_files = [
    'css/pigweed.css',

    # Needed for Inconsolata font.
    'https://fonts.googleapis.com/css2?family=Inconsolata&display=swap',
]

# Output file base name for HTML help builder.
htmlhelp_basename = 'Pigweeddoc'

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [('index', 'pigweed', 'Pigweed', ['Google'], 1)]

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    ('index', 'Pigweed', 'Pigweed', 'Google', 'Pigweed', 'Firmware framework',
     'Miscellaneous'),
]


def do_not_skip_init(app, what, name, obj, would_skip, options):
    if name == "__init__":
        return False  # never skip __init__ functions
    return would_skip


# Problem: CSS files aren't copied after modifying them. Solution:
# https://github.com/sphinx-doc/sphinx/issues/2090#issuecomment-572902572
def env_get_outdated(app, env, added, changed, removed):
    return ['index']


def setup(app):
    app.add_css_file('css/pigweed.css')
    app.connect('env-get-outdated', env_get_outdated)
    app.connect("autodoc-skip-member", do_not_skip_init)
