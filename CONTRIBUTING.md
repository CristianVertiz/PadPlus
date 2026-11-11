# Contributing to PadPlus

PadPlus is an independent macOS editor based on the Notepad++ source code. It is a personal project, not affiliated with the Notepad++ project.
Changes to Notepad++ itself (the Windows application) belong to [notepad-plus-plus/notepad-plus-plus](https://github.com/notepad-plus-plus/notepad-plus-plus) and follow
[its contribution rules](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/CONTRIBUTING.md), not this file.

## Before you start

- Open an issue first for anything larger than a small fix, so the approach can be agreed.
- Check the [known gaps](macos/README.md#known-gaps): some things are missing on purpose (plugins cannot work on macOS).

## Making a change

- Branch from `macos` and open the pull request against `macos`. The `master` branch only mirrors upstream.
- Keep the change small and focused. No unrelated refactoring or reformatting.
- The macOS code lives in `macos/`. Do not modify `scintilla/` or `lexilla/`.
- Edit other upstream files (`PowerEditor/`, `boostregex/`) only when there is no other way. Keep such edits minimal, keep the file's line endings
  (several use CRLF), and add a dated "Modified ... for PadPlus" comment under the licence header.
- New source files start with the two header lines used throughout `macos/` (`SPDX-License-Identifier: GPL-3.0-or-later` and a copyright line).
- For code in the shared core, follow the style of the surrounding code.

## Checking it

The [developer guide](macos/README.md) explains how to build and test. Before opening a pull request:

- all test programs print `OK`;
- if you touched packaging or the bundle, `package_app` and `macos/tests/package_check.sh macos/build/PadPlus.app --packaged` pass.

The same checks run on GitHub for every pull request.

## Licence and disclosure

Contributions are accepted under the GNU General Public License, version 3 or later, the licence of the project.
If AI tools wrote part of your change, say so in the pull request.
