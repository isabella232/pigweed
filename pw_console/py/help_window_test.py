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
"""Tests for pw_console.console_app"""

import unittest
from inspect import cleandoc
from unittest.mock import Mock

from prompt_toolkit.key_binding import KeyBindings

from pw_console.console_app import ConsoleApp
from pw_console.help_window import HelpWindow, KEYBIND_TEMPLATE


class TestHelpWindow(unittest.TestCase):
    """Tests for ConsoleApp."""
    def setUp(self):
        self.maxDiff = None  # pylint: disable=invalid-name

    def test_instantiate(self) -> None:
        help_window = HelpWindow(ConsoleApp())
        self.assertIsNotNone(help_window)

    def test_template_loads(self) -> None:
        self.assertIn('{%', KEYBIND_TEMPLATE)

    # pylint: disable=unused-variable,unused-argument
    def test_add_keybind_help_text(self) -> None:
        bindings = KeyBindings()

        @bindings.add('f1')
        def show_help(event):
            """Toggle help window."""

        @bindings.add('c-w')
        @bindings.add('c-q')
        def exit_(event):
            """Quit the application."""

        app = Mock()

        help_window = HelpWindow(app)
        help_window.add_keybind_help_text('Global', bindings)

        self.assertEqual(
            help_window.help_text_sections,
            {
                'Global': {
                    'Quit the application.': ['ControlQ', 'ControlW'],
                    'Toggle help window.': ['F1'],
                }
            },
        )

    def test_generate_help_text(self) -> None:
        """Test keybind list template generation."""
        global_bindings = KeyBindings()

        @global_bindings.add('f1')
        def show_help(event):
            """Toggle help window."""

        @global_bindings.add('c-w')
        @global_bindings.add('c-q')
        def exit_(event):
            """Quit the application."""

        focus_bindings = KeyBindings()

        @focus_bindings.add('s-tab')
        @focus_bindings.add('c-right')
        @focus_bindings.add('c-down')
        def app_focus_next(event):
            """Move focus to the next widget."""

        @focus_bindings.add('c-left')
        @focus_bindings.add('c-up')
        def app_focus_previous(event):
            """Move focus to the previous widget."""

        app = Mock()

        help_window = HelpWindow(app)
        help_window.add_keybind_help_text('Global', global_bindings)
        help_window.add_keybind_help_text('Focus', focus_bindings)

        help_text = help_window.generate_help_text()

        self.assertIn(
            cleandoc("""
            Toggle help window. -----------------  F1
            Quit the application. ---------------  ControlQ, ControlW
            """),
            help_text,
        )
        self.assertIn(
            cleandoc("""
            Move focus to the next widget. ------  BackTab, ControlDown, ControlRight
            Move focus to the previous widget. --  ControlLeft, ControlUp
            """),
            help_text,
        )


if __name__ == '__main__':
    unittest.main()
