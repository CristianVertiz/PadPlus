# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
# macOS (Qt 6) build of the Notepad++ fork. Vendored scintilla/ and lexilla/ are NOT modified:
# generated files go to the build dir.
cmake_minimum_required(VERSION 3.21)
project(NotepadPP_mac LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)   # Apple Silicon only (plan decision 13)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Core5Compat PrintSupport)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

get_filename_component(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
set(SCI ${ROOT}/scintilla)
set(LEX ${ROOT}/lexilla)

# ---- generate ScintillaEdit.{cpp,h} in a mirror dir (generator uses ../.. relative paths) ----
set(GEN ${CMAKE_BINARY_DIR}/gen/scintilla)
file(MAKE_DIRECTORY ${GEN}/qt/ScintillaEdit)
file(CREATE_LINK ${SCI}/include ${GEN}/include SYMBOLIC)
file(CREATE_LINK ${SCI}/scripts ${GEN}/scripts SYMBOLIC)
foreach(f WidgetGen.py ScintillaEdit.h.template)
  configure_file(${SCI}/qt/ScintillaEdit/${f} ${GEN}/qt/ScintillaEdit/${f} COPYONLY)
endforeach()
# The Qt template predates this Scintilla's removal of the 32-bit TextToFind/TextRange/RangeToFormat
# APIs: use the *Full variants (patched copy only; vendored template untouched).
file(READ ${SCI}/qt/ScintillaEdit/ScintillaEdit.cpp.template TPL)
foreach(n Sci_TextToFind Sci_TextRange Sci_RangeToFormat)
  string(REGEX REPLACE "${n}([^A-Za-z])" "${n}Full\\1" TPL "${TPL}")
endforeach()
foreach(n SCI_FINDTEXT SCI_GETTEXTRANGE SCI_FORMATRANGE)
  string(REGEX REPLACE "${n}([^A-Za-z])" "${n}FULL\\1" TPL "${TPL}")
endforeach()
file(WRITE ${GEN}/qt/ScintillaEdit/ScintillaEdit.cpp.template "${TPL}")
execute_process(COMMAND ${Python3_EXECUTABLE} -I -B WidgetGen.py
                WORKING_DIRECTORY ${GEN}/qt/ScintillaEdit
                RESULT_VARIABLE gen_rc)
if(NOT gen_rc EQUAL 0 OR NOT EXISTS ${GEN}/qt/ScintillaEdit/ScintillaEdit.cpp)
  message(FATAL_ERROR "WidgetGen.py failed")
endif()

# ---- Scintilla (Qt backend) + Boost.Regex search (as on Windows) ----
file(GLOB SCI_CORE ${SCI}/src/*.cxx)
add_library(scintilla_qt STATIC
  ${SCI_CORE}
  ${SCI}/qt/ScintillaEditBase/PlatQt.cpp
  ${SCI}/qt/ScintillaEditBase/ScintillaQt.cpp
  ${SCI}/qt/ScintillaEditBase/ScintillaEditBase.cpp
  ${SCI}/qt/ScintillaEdit/ScintillaDocument.cpp
  ${GEN}/qt/ScintillaEdit/ScintillaEdit.cpp
  ${ROOT}/boostregex/BoostRegExSearch.cxx
  ${ROOT}/boostregex/UTF8DocumentIterator.cxx)
target_include_directories(scintilla_qt PUBLIC
  ${SCI}/include ${SCI}/src ${SCI}/qt/ScintillaEditBase ${SCI}/qt/ScintillaEdit
  ${GEN}/qt/ScintillaEdit ${ROOT}/boostregex)
target_compile_definitions(scintilla_qt PUBLIC SCINTILLA_QT=1 SCI_OWNREGEX BOOST_REGEX_STANDALONE
                           $<$<CONFIG:Release>:NDEBUG=1>)
target_link_libraries(scintilla_qt PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Core5Compat)
target_compile_options(scintilla_qt PRIVATE -w)

# ---- Lexilla (static) ----
file(GLOB LEX_SRC ${LEX}/lexers/*.cxx ${LEX}/lexlib/*.cxx)
add_library(lexilla STATIC ${LEX}/src/Lexilla.cxx ${LEX_SRC})
target_include_directories(lexilla PUBLIC ${LEX}/include ${SCI}/include)
target_include_directories(lexilla PRIVATE ${LEX}/lexlib ${LEX}/src)
target_compile_options(lexilla PRIVATE -w)
set_source_files_properties(${LEX}/lexers/LexUser.cxx PROPERTIES INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/shim)

# ---- app (Phase 2 target is defined at the end of this file, after the core libraries exist) ----

# =====================================================================================
# Phase 1: shared Notepad++ core, compiled UNMODIFIED (bar tiny portability edits) against a Win32 compat layer.
# compat/ supplies Win32 types/constants, real POSIX-backed implementations of the non-UI APIs the core needs
# (files, strings, code pages), and no-op stubs for UI/system calls the core merely mentions.
# =====================================================================================
set(NPP ${ROOT}/PowerEditor/src)
file(GLOB UCHARDET_SRC ${NPP}/uchardet/*.cpp)
set(NPP_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/compat
  ${SCI}/include ${LEX}/include ${NPP}
  ${NPP}/MISC/Common ${NPP}/WinControls ${NPP}/WinControls/ContextMenu ${NPP}/WinControls/DockingWnd ${NPP}/WinControls/ToolBar
  ${NPP}/WinControls/shortcut ${NPP}/ScintillaComponent ${NPP}/MISC ${NPP}/MISC/PluginsManager ${NPP}/WinControls/StaticDialog
  ${NPP}/WinControls/TabBar ${NPP}/WinControls/ImageListSet ${NPP}/WinControls/SplitterContainer ${NPP}/pugixml ${NPP}/uchardet
  ${NPP}/WinControls/StatusBar ${NPP}/MISC/Process ${NPP}/MISC/Exception ${NPP}/WinControls/ColourPicker ${NPP}/WinControls/Grid
  ${NPP}/WinControls/ToolTip ${NPP}/WinControls/TrayIcon ${NPP}/WinControls/AboutDlg ${NPP}/WinControls/TreeView
  ${NPP}/WinControls/Preference ${NPP}/MISC/RegExt ${NPP}/WinControls/TaskList ${NPP}/WinControls/WindowsDlg
  ${NPP}/WinControls/AnsiCharPanel ${NPP}/WinControls/ClipboardHistory ${NPP}/WinControls/FindCharsInRange
  ${NPP}/WinControls/VerticalFileSwitcher ${NPP}/WinControls/ProjectPanel ${NPP}/WinControls/DocumentMap
  ${NPP}/WinControls/FunctionList ${NPP}/WinControls/FileBrowser ${NPP}/WinControls/PluginsAdmin
  ${NPP}/WinControls/OpenSaveFileDialog ${NPP}/WinControls/ReadDirectoryChanges ${NPP}/WinControls/StaticDialog/RunDlg
  ${NPP}/MISC/hmac ${NPP}/MISC/md5 ${NPP}/MISC/sha1 ${NPP}/MISC/sha2 ${NPP}/MISC/sha512 ${NPP}/MISC/crc16)

add_library(npp_core STATIC
  compat/win_impl.cpp compat/ui_stubs.cpp
  ${NPP}/ScintillaComponent/UserDefineDialog.cpp
  ${NPP}/MISC/Common/Common.cpp ${NPP}/MISC/Common/FileInterface.cpp
  ${NPP}/Utf8_16.cpp ${NPP}/EncodingMapper.cpp ${NPP}/Parameters.cpp ${NPP}/localization.cpp
  ${NPP}/ScintillaComponent/Buffer.cpp ${NPP}/ScintillaComponent/ScintillaEditView.cpp
  ${NPP}/WinControls/FunctionList/functionParser.cpp
  ${NPP}/WinControls/shortcut/shortcut.cpp ${NPP}/dpiManagerV2.cpp ${NPP}/MISC/sha2/sha-256.cpp
  ${NPP}/pugixml/pugixml.cpp ${UCHARDET_SRC})
target_include_directories(npp_core PUBLIC ${NPP_INCLUDES})
target_compile_definitions(npp_core PUBLIC UNICODE _UNICODE)
target_compile_options(npp_core PRIVATE -w -fno-strict-aliasing -g -include windows.h)
target_link_libraries(npp_core PUBLIC lexilla)

add_executable(core_test tests/core_test.cpp)
target_link_libraries(core_test PRIVATE npp_core)
target_compile_options(core_test PRIVATE -include windows.h -w)
# model config files the core copies into the user profile on first run (Windows ships them next to notepad++.exe)

# ---- Phase 2: Qt <-> core bridge ----
add_library(qt_bridge STATIC app/qt_bridge.cpp)
target_include_directories(qt_bridge PUBLIC app ${CMAKE_CURRENT_SOURCE_DIR}/compat)
target_link_libraries(qt_bridge PUBLIC scintilla_qt)

add_library(app_core STATIC app/core_boot.cpp app/npp_host.cpp)
target_include_directories(app_core PUBLIC app ${NPP_INCLUDES})
target_compile_definitions(app_core PUBLIC UNICODE _UNICODE)
target_compile_options(app_core PRIVATE -w -include windows.h)
target_link_libraries(app_core PUBLIC npp_core)

add_executable(bridge_test tests/bridge_test.cpp)
target_include_directories(bridge_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(bridge_test PRIVATE UNICODE _UNICODE)
target_compile_options(bridge_test PRIVATE -w -include windows.h)
target_link_libraries(bridge_test PRIVATE qt_bridge app_core npp_core scintilla_qt)

add_executable(host_test tests/host_test.cpp)
target_include_directories(host_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(host_test PRIVATE UNICODE _UNICODE)
target_compile_options(host_test PRIVATE -w -include windows.h)
target_link_libraries(host_test PRIVATE qt_bridge app_core npp_core scintilla_qt)

# ---- PadPlus: the macOS app (Qt front end over the Notepad++ core) ----
add_library(app_ui STATIC app/main_window.cpp app/find_dialog.cpp app/keymap.cpp app/shortcut_mapper.cpp app/preferences_dialog.cpp
  app/style_dialog.cpp app/find_in_files.cpp app/column_editor.cpp app/panels.cpp app/chrome.cpp app/print_doc.cpp app/tools.cpp app/macro_ui.cpp app/extras.cpp app/main_window_extras.cpp app/udl_dialog.cpp)
target_include_directories(app_ui PUBLIC app ${NPP})   # ${NPP}: menuCmdID.h (IDM_* command ids)
target_link_libraries(app_ui PUBLIC qt_bridge Qt6::Widgets Qt6::PrintSupport)

# app icon: packaging/icon/PadPlus.icns (its sources, the SVGs and the iconset, sit next to it; rebuild with `iconutil -c icns PadPlus.iconset`)
set(APP_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/PadPlus.icns)
set_source_files_properties(${APP_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

# packaging/Info.plist.in is filled from these *variables* (a custom plist template does not read target properties)
set(MACOSX_BUNDLE_EXECUTABLE_NAME "PadPlus")
set(MACOSX_BUNDLE_BUNDLE_NAME "PadPlus")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.cristianvertiz.padplus")
set(PADPLUS_VERSION "0.2.0" CACHE STRING "PadPlus version (the release workflow passes the tag's)")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PADPLUS_VERSION}")
set(MACOSX_BUNDLE_ICON_FILE "PadPlus.icns")
qt_add_executable(NotepadPP MACOSX_BUNDLE app/main.cpp ${APP_ICNS})
set_target_properties(NotepadPP PROPERTIES OUTPUT_NAME "PadPlus" MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/Info.plist.in)
target_include_directories(NotepadPP PRIVATE app)
target_link_libraries(NotepadPP PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

# everything the core and the UI read at run time goes into Contents/Resources (the core treats it as its install dir)
set(APP_RES "$<TARGET_BUNDLE_CONTENT_DIR:NotepadPP>/Resources")
add_custom_command(TARGET NotepadPP POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_RES} ${APP_RES}/plugins   # plugins/: the core creates it at start-up if missing; it must already exist (no writes into a signed bundle)
  COMMAND ${CMAKE_COMMAND} -E copy ${NPP}/langs.model.xml ${NPP}/stylers.model.xml ${APP_RES}/
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT}/PowerEditor/installer/themes ${APP_RES}/themes
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT}/PowerEditor/installer/functionList ${APP_RES}/functionList
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT}/PowerEditor/installer/nativeLang ${APP_RES}/localization
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${NPP}/icons ${APP_RES}/icons
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/PadPlus.iconset/icon_128x128@2x.png" ${APP_RES}/PadPlus.png   # the About box (the bundle carries no .icns reader)
  COMMAND ${CMAKE_COMMAND} -E remove -f ${APP_RES}/icons/npp.ico)   # the Notepad++ logo is its project's mark: PadPlus neither uses nor ships it

# `cmake --build build --target package_app`: copy Qt into the bundle (macdeployqt), drop what PadPlus does not use, sign ad hoc -> a self-contained, relocatable PadPlus.app
find_program(MACDEPLOYQT macdeployqt HINTS "${Qt6_DIR}/../../../bin" /opt/homebrew/opt/qt/bin)
# macdeployqt deploys every plugin of the Qt install, including ones whose frameworks Homebrew keeps in other prefixes (it prints "Cannot resolve rpath"
# for those) and Qt's on-screen keyboard (which pulls in Qt Quick/QML). PadPlus needs only: cocoa, macstyle, ico images (+ Core/Gui/Widgets/PrintSupport/Core5Compat).
set(APP_BUNDLE "$<TARGET_BUNDLE_CONTENT_DIR:NotepadPP>")
set(PRUNE_DIRS PlugIns/platforminputcontexts PlugIns/iconengines PlugIns/networkinformation PlugIns/tls
  Frameworks/QtQuick.framework Frameworks/QtQml.framework Frameworks/QtQmlMeta.framework Frameworks/QtQmlModels.framework Frameworks/QtQmlWorkerScript.framework
  Frameworks/QtOpenGL.framework Frameworks/QtVirtualKeyboard.framework Frameworks/QtVirtualKeyboardQml.framework Frameworks/QtNetwork.framework
  Frameworks/QtSvg.framework Frameworks/QtPdf.framework)   # (QtDBus stays: QtGui links it)
set(PRUNE_IMAGE_PLUGINS gif icns jpeg macheif macjp2 pdf svg tga tiff wbmp webp)   # imageformats: keep only ico (the toolbar icons; PNG is built into QtGui)
# macdeployqt rewrites the executable's library paths in place, and cannot deploy from an executable it already rewrote:
# keep the linker's output aside and start every packaging run from it, so package_app can be run again without a relink.
add_custom_command(TARGET NotepadPP POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:NotepadPP>" "${CMAKE_BINARY_DIR}/PadPlus.linked")
set(PACKAGE_COMMANDS
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/PadPlus.linked" "$<TARGET_FILE:NotepadPP>"
  # start from a clean slate so repeated runs do not accumulate
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${APP_BUNDLE}/Frameworks"
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${APP_BUNDLE}/PlugIns"
  COMMAND ${MACDEPLOYQT} "$<TARGET_BUNDLE_DIR:NotepadPP>" -always-overwrite)
foreach(d ${PRUNE_DIRS})
  list(APPEND PACKAGE_COMMANDS COMMAND ${CMAKE_COMMAND} -E remove_directory "${APP_BUNDLE}/${d}")
endforeach()
foreach(i ${PRUNE_IMAGE_PLUGINS})
  list(APPEND PACKAGE_COMMANDS COMMAND ${CMAKE_COMMAND} -E remove -f "${APP_BUNDLE}/PlugIns/imageformats/libq${i}.dylib")
endforeach()
list(APPEND PACKAGE_COMMANDS COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/fix_homebrew_refs.sh "$<TARGET_BUNDLE_DIR:NotepadPP>")
list(APPEND PACKAGE_COMMANDS COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/set_min_macos.sh "$<TARGET_BUNDLE_DIR:NotepadPP>")
# licence texts of everything embedded, before signing (the signature seals Resources)
list(APPEND PACKAGE_COMMANDS COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/collect_licenses.sh "$<TARGET_BUNDLE_DIR:NotepadPP>" ${ROOT})
list(APPEND PACKAGE_COMMANDS COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:NotepadPP>")
add_custom_target(package_app ${PACKAGE_COMMANDS} DEPENDS NotepadPP COMMENT "Bundling Qt into PadPlus.app and signing it (ad hoc)")

add_executable(window_test tests/window_test.cpp)
target_include_directories(window_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(window_test PRIVATE UNICODE _UNICODE)
target_link_libraries(window_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

# themes (Notepad++ ships ~20): the core looks for <exe dir>/themes
add_executable(dialogs_test tests/dialogs_test.cpp)
target_include_directories(dialogs_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(dialogs_test PRIVATE UNICODE _UNICODE)
target_compile_options(dialogs_test PRIVATE -w -include windows.h)
target_link_libraries(dialogs_test PRIVATE qt_bridge app_core npp_core scintilla_qt)
# function list parsing rules (regex XML per language)

add_executable(settings_ui_test tests/settings_ui_test.cpp)
target_include_directories(settings_ui_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(settings_ui_test PRIVATE UNICODE _UNICODE)
target_link_libraries(settings_ui_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

add_executable(panels_test tests/panels_test.cpp)
target_include_directories(panels_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(panels_test PRIVATE UNICODE _UNICODE)
target_link_libraries(panels_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

# Notepad++'s own toolbar icons (dark/light sets), next to each executable that builds a MainWindow

add_executable(toolbar_test tests/toolbar_test.cpp)
target_include_directories(toolbar_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(toolbar_test PRIVATE UNICODE _UNICODE)
target_link_libraries(toolbar_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

add_executable(macro_test tests/macro_test.cpp)
target_include_directories(macro_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(macro_test PRIVATE UNICODE _UNICODE)
target_link_libraries(macro_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

add_executable(extras_test tests/extras_test.cpp)
target_include_directories(extras_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(extras_test PRIVATE UNICODE _UNICODE)
target_link_libraries(extras_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)
add_executable(more_test tests/more_test.cpp)
target_include_directories(more_test PRIVATE ${NPP_INCLUDES})
target_compile_definitions(more_test PRIVATE UNICODE _UNICODE)
target_link_libraries(more_test PRIVATE app_ui app_core npp_core scintilla_qt Qt6::Widgets)

# resources the test executables read from their own folder (themes, function-list rules, icons, config templates): staged once into build/.
# A single target, not per-test copies: parallel copies into the same folders raced each other.
add_custom_target(test_resources
  COMMAND ${CMAKE_COMMAND} -E copy ${NPP}/langs.model.xml ${NPP}/stylers.model.xml ${CMAKE_BINARY_DIR}/
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT}/PowerEditor/installer/themes ${CMAKE_BINARY_DIR}/themes
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT}/PowerEditor/installer/functionList ${CMAKE_BINARY_DIR}/functionList
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ROOT}/PowerEditor/installer/nativeLang ${CMAKE_BINARY_DIR}/localization
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${NPP}/icons ${CMAKE_BINARY_DIR}/icons)
foreach(t core_test host_test window_test dialogs_test settings_ui_test panels_test toolbar_test macro_test extras_test more_test)
  add_dependencies(${t} test_resources)
endforeach()
