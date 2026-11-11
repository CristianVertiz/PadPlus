# AGENTS.md

This branch (`macos`) is PadPlus: an independent macOS editor based on the Notepad++ source code. The upstream rule "support Windows only" applies to
Notepad++ itself (`master`), not to this branch.

## Before making changes

- Read [CONTRIBUTING.md](CONTRIBUTING.md) and the [developer guide](macos/README.md).
- Check existing code patterns before editing. Keep changes focused and small; no unrelated refactoring or formatting.

## Project constraints

- Do not modify `scintilla/` or `lexilla/`.
- The macOS code lives in `macos/`. Edit other upstream files only when unavoidable: keep the edit minimal, preserve the file's line endings
  (several are CRLF, so edit byte-exact), and add a dated "Modified ... for PadPlus" comment under the licence header.
- Do not use the Notepad++ name or logo as PadPlus's branding. "Based on the Notepad++ source code" is the wording; the app ships its own icon.
- New source files start with the SPDX and copyright header lines used in `macos/`.
- Never open pull requests against the upstream Notepad++ repository from this work.

## Validation

- Build and run every test program (developer guide, section "Test"); each must print `OK`.
- After packaging changes, run `package_app` and `macos/tests/package_check.sh macos/build/PadPlus.app --packaged`.
- Tests and manual runs must use a temporary `HOME` and `NPP_SETTINGS_DIR`; never touch the real profile in `~/Library/Application Support/Notepad++`.

## Pull requests

- Target the `macos` branch and use the [pull request template](.github/pull_request_template.md).
- Disclose AI assistance, including the model, and add the agent as a co-author with a `Co-Authored-By` trailer.
