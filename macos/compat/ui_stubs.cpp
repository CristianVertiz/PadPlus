// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Link-time stand-ins for Win32 UI classes/functions that the shared core references but never needs to
// actually run on macOS. Phase 2/3 replace these with real Qt implementations; until then they do nothing.
#include <atomic>
#include "Parameters.h"
#include "StaticDialog.h"
#include "ContextMenu.h"
#include "ControlsTab.h"
#include "CustomFileDialog.h"
#include "AboutDlg.h"
#include "pluginsAdmin.h"
#include "preferenceDlg.h"
#include "verifySignedfile.h"
#include "Notepad_plus.h"
#include "NppDarkMode.h"
#include "host_hooks.h"
#include "URLCtrl.h"
#include "ColourPicker.h"
#include "TabBar.h"

std::atomic<bool> g_bNppExitFlag{ false };

// --- dialogs / windows ---
StaticDialog::~StaticDialog() {}
void StaticDialog::create(int, bool, bool, WORD, const std::wstring&) {}
void StaticDialog::display(bool) const {}
void StaticDialog::destroy() {}
void StaticDialog::goToCenter(UINT) {}
void NetworkPathWarningBox::doDialog(bool) {}
intptr_t CALLBACK NetworkPathWarningBox::run_dlgProc(UINT, WPARAM, LPARAM) { return 0; }
bool PreferenceDlg::renameDialogTitle(const wchar_t*, const wchar_t*) { return false; }
bool PreferenceDlg::setListSelection(size_t) const { return false; }
void PluginsAdminDlg::changeTabName(LIST_TYPE, wchar_t*) {}
void PluginsAdminDlg::changeColumnName(COLUMN_TYPE, const wchar_t*) {}
void ControlsTab::renameTab(size_t, const wchar_t*) {}
void TabBar::getCurrentTitle(wchar_t* t, int) { if (t) t[0] = 0; }
void ContextMenu::destroy() {}
MenuItemUnit::MenuItemUnit(unsigned long id, const wchar_t* name, const wchar_t* parent)
	: _cmdID(id), _itemName(name ? name : L""), _parentFolderName(parent ? parent : L"") {}
struct CustomFileDialog::Impl {};
CustomFileDialog::CustomFileDialog(HWND) {}
CustomFileDialog::~CustomFileDialog() {}
void CustomFileDialog::setTitle(const wchar_t*) {}
void CustomFileDialog::setFolder(const wchar_t*) {}
std::wstring CustomFileDialog::pickFolder() { return L""; }
SecurityGuard::SecurityGuard() {}

void StaticDialog::getMappedChildRect(int, RECT&) const {}
INT_PTR StaticDialog::myCreateDialogBoxIndirectParam(int, bool, WORD) { return 0; }
void TabBar::destroy() {}
void TabBar::init(HINSTANCE, HWND, bool, bool) {}
void TabBar::reSizeTo(RECT&) {}
void TabBarPlus::init(HINSTANCE, HWND, bool, bool, unsigned char) {}
void TabBarPlus::destroy() {}
void ControlsTab::createTabs(WindowVector&) {}
void ControlsTab::reSizeToWH(RECT&) {}
void ControlsTab::activateWindowAt(int) {}
void TabBar::activateAt(int) const {}
int TabBar::getNextOrPrevTabIdx(bool) const { return 0; }
void URLCtrl::create(HWND, const wchar_t*, COLORREF) {}
void URLCtrl::destroy() {}
void ColourPicker::init(HINSTANCE, HWND) {}
void ColourPicker::destroy() {}
std::wstring CustomFileDialog::doSaveDlg() { return L""; }
std::wstring CustomFileDialog::doOpenSingleFileDlg() { return L""; }
void CustomFileDialog::setExtFilter(const wchar_t*, const wchar_t*) {}
void CustomFileDialog::setExtIndex(int) {}

// --- Notepad_plus (the application window) ---
HostHooks& hostHooks() { static HostHooks h; return h; }
void Notepad_plus::saveCurrentSession() { if (hostHooks().onSaveSession) hostHooks().onSaveSession(); }
void Notepad_plus::notifyBufferChanged(Buffer* b, int mask) { if (hostHooks().onBufferChanged) hostHooks().onBufferChanged(b, mask); }
void Notepad_plus::command(int) {}
bool Notepad_plus::viewVisible(int) { return false; }
void Notepad_plus::loadBufferIntoView(BufferID, int, bool) {}

// --- dark mode: core only needs flags/colours; real theming comes from Qt ---
namespace NppDarkMode {
bool isEnabled() { return false; }
bool isWindows10() { return false; }
void setAdvancedOptions() {}
Colors getDarkModeDefaultColors(ColorTone) { return Colors{}; }
void autoSubclassAndThemeChildControls(HWND, bool, bool) {}
void autoThemeChildControls(HWND) {}
void setDarkScrollBar(HWND) {}
void setDarkTooltips(HWND, ToolTipsType) {}
void setDarkAutoCompletion() {}
void setDarkTitleBar(HWND) {}
void autoSubclassAndThemeWindowNotify(HWND) {}
LRESULT onCtlColor(HDC) { return 0; }
LRESULT onCtlColorCtrl(HDC) { return 0; }
LRESULT onCtlColorDlg(HDC) { return 0; }
LRESULT onCtlColorListbox(WPARAM, LPARAM) { return 0; }
int darkMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT) { return 0; }
}
