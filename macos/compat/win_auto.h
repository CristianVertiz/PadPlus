// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Placeholder constants (unique values) and no-op stubs for Win32 APIs the shared core mentions but never needs for real.
// Generated once by a throwaway script; now maintained by hand: when a new source fails to compile on a missing Win32 name, add it here
// (or, if behaviour matters, give it a real implementation in win_impl.{h,cpp} / real values in win_consts.h).
#pragma once
#include <windows.h>
#define NPP_STUB(name) template<class... A> inline LRESULT name(A&&...) { return 0; }
#define NPP_PSTUB(name) template<class... A> inline void* name(A&&...) { return nullptr; }
#ifndef ASSOCF_INIT_IGNOREUNKNOWN
#define ASSOCF_INIT_IGNOREUNKNOWN 100000
#endif
#ifndef ASSOCF_VERIFY
#define ASSOCF_VERIFY 100001
#endif
#ifndef ASSOCSTR_COMMAND
#define ASSOCSTR_COMMAND 100002
#endif
#ifndef BCRYPT_HASH_LENGTH
#define BCRYPT_HASH_LENGTH 100003
#endif
#ifndef BM_GETCHECK
#define BM_GETCHECK 100004
#endif
#ifndef BM_SETCHECK
#define BM_SETCHECK 100005
#endif
#ifndef BN_CLICKED
#define BN_CLICKED 100006
#endif
#ifndef BS_AUTOCHECKBOX
#define BS_AUTOCHECKBOX 100007
#endif
#ifndef BS_AUTORADIOBUTTON
#define BS_AUTORADIOBUTTON 100008
#endif
#ifndef BS_CHECKBOX
#define BS_CHECKBOX 100009
#endif
#ifndef BS_MULTILINE
#define BS_MULTILINE 100010
#endif
#ifndef BS_RADIOBUTTON
#define BS_RADIOBUTTON 100011
#endif
#ifndef BS_TYPEMASK
#define BS_TYPEMASK 100012
#endif
#ifndef CBN_SELCHANGE
#define CBN_SELCHANGE 100013
#endif
#ifndef CB_ADDSTRING
#define CB_ADDSTRING 100014
#endif
#ifndef CB_DELETESTRING
#define CB_DELETESTRING 100015
#endif
#ifndef CB_ERR
#define CB_ERR 100016
#endif
#ifndef CB_FINDSTRINGEXACT
#define CB_FINDSTRINGEXACT 100017
#endif
#ifndef CB_GETCOUNT
#define CB_GETCOUNT 100018
#endif
#ifndef CB_GETCURSEL
#define CB_GETCURSEL 100019
#endif
#ifndef CB_GETITEMDATA
#define CB_GETITEMDATA 100020
#endif
#ifndef CB_GETLBTEXT
#define CB_GETLBTEXT 100021
#endif
#ifndef CB_GETLBTEXTLEN
#define CB_GETLBTEXTLEN 100022
#endif
#ifndef CB_INSERTSTRING
#define CB_INSERTSTRING 100023
#endif
#ifndef CB_RESETCONTENT
#define CB_RESETCONTENT 100024
#endif
#ifndef CB_SETCURSEL
#define CB_SETCURSEL 100025
#endif
#ifndef CB_SETITEMDATA
#define CB_SETITEMDATA 100026
#endif
#ifndef CERT_FIND_SUBJECT_CERT
#define CERT_FIND_SUBJECT_CERT 100027
#endif
#ifndef CERT_NAME_SIMPLE_DISPLAY_TYPE
#define CERT_NAME_SIMPLE_DISPLAY_TYPE 100028
#endif
#ifndef CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED
#define CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED 100029
#endif
#ifndef CERT_QUERY_FORMAT_FLAG_BINARY
#define CERT_QUERY_FORMAT_FLAG_BINARY 100030
#endif
#ifndef CERT_QUERY_OBJECT_FILE
#define CERT_QUERY_OBJECT_FILE 100031
#endif
#ifndef CMSG_SIGNER_INFO_PARAM
#define CMSG_SIGNER_INFO_PARAM 100032
#endif
#ifndef CSTR_EQUAL
#define CSTR_EQUAL 100033
#endif
#ifndef DEFAULT_CHARSET
#define DEFAULT_CHARSET 100034
#endif
#ifndef DEFAULT_GUI_FONT
#define DEFAULT_GUI_FONT 100035
#endif
#ifndef DM_REPOSITION
#define DM_REPOSITION 100036
#endif
#ifndef DM_SETDEFID
#define DM_SETDEFID 100037
#endif
#ifndef DS_MODALFRAME
#define DS_MODALFRAME 100038
#endif
#ifndef DUPLICATE_CLOSE_SOURCE
#define DUPLICATE_CLOSE_SOURCE 100039
#endif
#ifndef EM_LIMITTEXT
#define EM_LIMITTEXT 100040
#endif
#ifndef EM_REPLACESEL
#define EM_REPLACESEL 100041
#endif
#ifndef EM_SETLIMITTEXT
#define EM_SETLIMITTEXT 100042
#endif
#ifndef EM_SETREADONLY
#define EM_SETREADONLY 100043
#endif
#ifndef EN_CHANGE
#define EN_CHANGE 100044
#endif
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 100045
#endif
#ifndef EXCEPTION_ACCESS_VIOLATION
#define EXCEPTION_ACCESS_VIOLATION 100046
#endif
#ifndef FILE_ATTRIBUTE_SYSTEM
#define FILE_ATTRIBUTE_SYSTEM 100047
#endif
#ifndef FILE_MAP_READ
#define FILE_MAP_READ 100048
#endif
#ifndef FILE_MAP_WRITE
#define FILE_MAP_WRITE 100049
#endif
#ifndef FILE_NAME_OPENED
#define FILE_NAME_OPENED 100050
#endif
#ifndef FOF_ALLOWUNDO
#define FOF_ALLOWUNDO 100051
#endif
#ifndef FOF_NOCONFIRMATION
#define FOF_NOCONFIRMATION 100052
#endif
#ifndef FOF_SILENT
#define FOF_SILENT 100053
#endif
#ifndef FORMAT_MESSAGE_ALLOCATE_BUFFER
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 100054
#endif
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 100055
#endif
#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS 100056
#endif
#ifndef FO_DELETE
#define FO_DELETE 100057
#endif
#ifndef GWL_EXSTYLE
#define GWL_EXSTYLE 100058
#endif
#ifndef GWL_STYLE
#define GWL_STYLE 100059
#endif
#ifndef HWND_BOTTOM
#define HWND_BOTTOM 100060
#endif
#ifndef IMAGE_FILE_MACHINE_AMD64
#define IMAGE_FILE_MACHINE_AMD64 100061
#endif
#ifndef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64 100062
#endif
#ifndef IMAGE_FILE_MACHINE_I386
#define IMAGE_FILE_MACHINE_I386 100063
#endif
#ifndef IMAGE_ICON
#define IMAGE_ICON 100064
#endif
#ifndef IMR_RECONVERTSTRING
#define IMR_RECONVERTSTRING 100065
#endif
#ifndef IS_TEXT_UNICODE_STATISTICS
#define IS_TEXT_UNICODE_STATISTICS 100066
#endif
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 100067
#endif
#ifndef LB_ADDSTRING
#define LB_ADDSTRING 100068
#endif
#ifndef LB_DELETESTRING
#define LB_DELETESTRING 100069
#endif
#ifndef LB_ERR
#define LB_ERR 100070
#endif
#ifndef LB_GETCURSEL
#define LB_GETCURSEL 100071
#endif
#ifndef LB_INSERTSTRING
#define LB_INSERTSTRING 100072
#endif
#ifndef LB_SETCURSEL
#define LB_SETCURSEL 100073
#endif
#ifndef LOCALE_IDEFAULTANSICODEPAGE
#define LOCALE_IDEFAULTANSICODEPAGE 100074
#endif
#ifndef LOCALE_RETURN_NUMBER
#define LOCALE_RETURN_NUMBER 100075
#endif
#ifndef LOCALE_USER_DEFAULT
#define LOCALE_USER_DEFAULT 100076
#endif
#ifndef LOGPIXELSX
#define LOGPIXELSX 100077
#endif
#ifndef LR_DEFAULTCOLOR
#define LR_DEFAULTCOLOR 100078
#endif
#ifndef LVCF_TEXT
#define LVCF_TEXT 100079
#endif
#ifndef LVIS_FOCUSED
#define LVIS_FOCUSED 100080
#endif
#ifndef LVIS_SELECTED
#define LVIS_SELECTED 100081
#endif
#ifndef LVS_EX_BORDERSELECT
#define LVS_EX_BORDERSELECT 100082
#endif
#ifndef LVS_EX_CHECKBOXES
#define LVS_EX_CHECKBOXES 100083
#endif
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 100084
#endif
#ifndef LVS_EX_FULLROWSELECT
#define LVS_EX_FULLROWSELECT 100085
#endif
#ifndef LVS_NOSORTHEADER
#define LVS_NOSORTHEADER 100086
#endif
#ifndef LVS_REPORT
#define LVS_REPORT 100087
#endif
#ifndef LVS_SHOWSELALWAYS
#define LVS_SHOWSELALWAYS 100088
#endif
#ifndef LVS_SINGLESEL
#define LVS_SINGLESEL 100089
#endif
#ifndef LWA_ALPHA
#define LWA_ALPHA 100090
#endif
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR 100091
#endif
#ifndef MB_APPLMODAL
#define MB_APPLMODAL 100092
#endif
#ifndef MB_ICONEXCLAMATION
#define MB_ICONEXCLAMATION 100093
#endif
#ifndef MB_RIGHT
#define MB_RIGHT 100094
#endif
#ifndef MB_RTLREADING
#define MB_RTLREADING 100095
#endif
#ifndef MB_SYSTEMMODAL
#define MB_SYSTEMMODAL 100096
#endif
#ifndef MF_POPUP
#define MF_POPUP 100097
#endif
#ifndef MF_SEPARATOR
#define MF_SEPARATOR 100098
#endif
#ifndef MK_RBUTTON
#define MK_RBUTTON 100099
#endif
#ifndef MK_SHIFT
#define MK_SHIFT 100100
#endif
#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONEAREST 100101
#endif
#ifndef MOVEFILE_COPY_ALLOWED
#define MOVEFILE_COPY_ALLOWED 100102
#endif
#ifndef MOVEFILE_WRITE_THROUGH
#define MOVEFILE_WRITE_THROUGH 100103
#endif
#ifndef PAGE_READWRITE
#define PAGE_READWRITE 100104
#endif
#ifndef PKCS_7_ASN_ENCODING
#define PKCS_7_ASN_ENCODING 100105
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 100106
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 100107
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA64
#define PROCESSOR_ARCHITECTURE_IA64 100108
#endif
#ifndef PROCESSOR_ARCHITECTURE_INTEL
#define PROCESSOR_ARCHITECTURE_INTEL 100109
#endif
#ifndef PRODUCT_DATACENTER_A_SERVER_CORE
#define PRODUCT_DATACENTER_A_SERVER_CORE 100110
#endif
#ifndef PRODUCT_DATACENTER_SERVER_CORE
#define PRODUCT_DATACENTER_SERVER_CORE 100111
#endif
#ifndef PRODUCT_DATACENTER_SERVER_CORE_V
#define PRODUCT_DATACENTER_SERVER_CORE_V 100112
#endif
#ifndef PRODUCT_ENTERPRISE_SERVER_CORE
#define PRODUCT_ENTERPRISE_SERVER_CORE 100113
#endif
#ifndef PRODUCT_ENTERPRISE_SERVER_CORE_V
#define PRODUCT_ENTERPRISE_SERVER_CORE_V 100114
#endif
#ifndef PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE 100115
#endif
#ifndef PRODUCT_STANDARD_A_SERVER_CORE
#define PRODUCT_STANDARD_A_SERVER_CORE 100116
#endif
#ifndef PRODUCT_STANDARD_SERVER_CORE
#define PRODUCT_STANDARD_SERVER_CORE 100117
#endif
#ifndef PRODUCT_STANDARD_SERVER_CORE_V
#define PRODUCT_STANDARD_SERVER_CORE_V 100118
#endif
#ifndef PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE
#define PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE 100119
#endif
#ifndef PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE
#define PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE 100120
#endif
#ifndef PRODUCT_STORAGE_EXPRESS_SERVER_CORE
#define PRODUCT_STORAGE_EXPRESS_SERVER_CORE 100121
#endif
#ifndef PRODUCT_STORAGE_STANDARD_SERVER_CORE
#define PRODUCT_STORAGE_STANDARD_SERVER_CORE 100122
#endif
#ifndef PRODUCT_STORAGE_WORKGROUP_SERVER_CORE
#define PRODUCT_STORAGE_WORKGROUP_SERVER_CORE 100123
#endif
#ifndef PRODUCT_WEB_SERVER_CORE
#define PRODUCT_WEB_SERVER_CORE 100124
#endif
#ifndef REPLACEFILE_IGNORE_ACL_ERRORS
#define REPLACEFILE_IGNORE_ACL_ERRORS 100125
#endif
#ifndef REPLACEFILE_IGNORE_MERGE_ERRORS
#define REPLACEFILE_IGNORE_MERGE_ERRORS 100126
#endif
#ifndef RRF_RT_REG_DWORD
#define RRF_RT_REG_DWORD 100127
#endif
#ifndef SB_LINEDOWN
#define SB_LINEDOWN 100128
#endif
#ifndef SB_LINELEFT
#define SB_LINELEFT 100129
#endif
#ifndef SB_LINERIGHT
#define SB_LINERIGHT 100130
#endif
#ifndef SB_LINEUP
#define SB_LINEUP 100131
#endif
#ifndef SB_PAGEDOWN
#define SB_PAGEDOWN 100132
#endif
#ifndef SB_PAGEUP
#define SB_PAGEUP 100133
#endif
#ifndef SB_THUMBPOSITION
#define SB_THUMBPOSITION 100134
#endif
#ifndef SB_THUMBTRACK
#define SB_THUMBTRACK 100135
#endif
#ifndef SB_VERT
#define SB_VERT 100136
#endif
#ifndef SDDL_REVISION_1
#define SDDL_REVISION_1 100137
#endif
#ifndef SEE_MASK_NOCLOSEPROCESS
#define SEE_MASK_NOCLOSEPROCESS 100138
#endif
#ifndef SIF_PAGE
#define SIF_PAGE 100139
#endif
#ifndef SIF_RANGE
#define SIF_RANGE 100140
#endif
#ifndef SPI_GETNONCLIENTMETRICS
#define SPI_GETNONCLIENTMETRICS 100141
#endif
#ifndef STN_CLICKED
#define STN_CLICKED 100142
#endif
#ifndef SWP_FRAMECHANGED
#define SWP_FRAMECHANGED 100143
#endif
#ifndef SWP_NOREDRAW
#define SWP_NOREDRAW 100144
#endif
#ifndef SWP_NOSENDCHANGING
#define SWP_NOSENDCHANGING 100145
#endif
#ifndef TBM_GETPOS
#define TBM_GETPOS 100146
#endif
#ifndef TBM_SETPOS
#define TBM_SETPOS 100147
#endif
#ifndef TBM_SETRANGE
#define TBM_SETRANGE 100148
#endif
#ifndef TBSTATE_CHECKED
#define TBSTATE_CHECKED 100149
#endif
#ifndef TB_CHECKBUTTON
#define TB_CHECKBUTTON 100150
#endif
#ifndef TB_ENABLEBUTTON
#define TB_ENABLEBUTTON 100151
#endif
#ifndef TB_GETSTATE
#define TB_GETSTATE 100152
#endif
#ifndef TB_SETDISABLEDIMAGELIST
#define TB_SETDISABLEDIMAGELIST 100153
#endif
#ifndef TB_SETIMAGELIST
#define TB_SETIMAGELIST 100154
#endif
#ifndef TCIF_TEXT
#define TCIF_TEXT 100155
#endif
#ifndef TCM_DELETEALLITEMS
#define TCM_DELETEALLITEMS 100156
#endif
#ifndef TCM_GETCURSEL
#define TCM_GETCURSEL 100157
#endif
#ifndef TCM_GETITEMCOUNT
#define TCM_GETITEMCOUNT 100158
#endif
#ifndef TCM_GETROWCOUNT
#define TCM_GETROWCOUNT 100159
#endif
#ifndef TCM_HITTEST
#define TCM_HITTEST 100160
#endif
#ifndef TCN_SELCHANGE
#define TCN_SELCHANGE 100161
#endif
#ifndef TPM_LEFTALIGN
#define TPM_LEFTALIGN 100162
#endif
#ifndef TTDT_AUTOPOP
#define TTDT_AUTOPOP 100163
#endif
#ifndef TTDT_INITIAL
#define TTDT_INITIAL 100164
#endif
#ifndef TTF_IDISHWND
#define TTF_IDISHWND 100165
#endif
#ifndef TTF_SUBCLASS
#define TTF_SUBCLASS 100166
#endif
#ifndef TTF_TRACK
#define TTF_TRACK 100167
#endif
#ifndef TTM_ACTIVATE
#define TTM_ACTIVATE 100168
#endif
#ifndef TTM_ADDTOOL
#define TTM_ADDTOOL 100169
#endif
#ifndef TTM_POPUP
#define TTM_POPUP 100170
#endif
#ifndef TTM_SETDELAYTIME
#define TTM_SETDELAYTIME 100171
#endif
#ifndef TTM_SETMAXTIPWIDTH
#define TTM_SETMAXTIPWIDTH 100172
#endif
#ifndef TTM_TRACKACTIVATE
#define TTM_TRACKACTIVATE 100173
#endif
#ifndef TTM_TRACKPOSITION
#define TTM_TRACKPOSITION 100174
#endif
#ifndef TTS_ALWAYSTIP
#define TTS_ALWAYSTIP 100175
#endif
#ifndef TTS_BALLOON
#define TTS_BALLOON 100176
#endif
#ifndef VER_NT_WORKSTATION
#define VER_NT_WORKSTATION 100177
#endif
#ifndef VOLUME_NAME_NT
#define VOLUME_NAME_NT 100178
#endif
#ifndef WH_GETMESSAGE
#define WH_GETMESSAGE 100179
#endif
#ifndef WM_ACTIVATE
#define WM_ACTIVATE 100180
#endif
#ifndef WM_CHAR
#define WM_CHAR 100181
#endif
#ifndef WM_CLOSE
#define WM_CLOSE 100182
#endif
#ifndef WM_CTLCOLORDLG
#define WM_CTLCOLORDLG 100183
#endif
#ifndef WM_CTLCOLOREDIT
#define WM_CTLCOLOREDIT 100184
#endif
#ifndef WM_CTLCOLORLISTBOX
#define WM_CTLCOLORLISTBOX 100185
#endif
#ifndef WM_CTLCOLORSTATIC
#define WM_CTLCOLORSTATIC 100186
#endif
#ifndef WM_DESTROY
#define WM_DESTROY 100187
#endif
#ifndef WM_GETTEXT
#define WM_GETTEXT 100188
#endif
#ifndef WM_GETTEXTLENGTH
#define WM_GETTEXTLENGTH 100189
#endif
#ifndef WM_HSCROLL
#define WM_HSCROLL 100190
#endif
#ifndef WM_IME_REQUEST
#define WM_IME_REQUEST 100191
#endif
#ifndef WM_INITDIALOG
#define WM_INITDIALOG 100192
#endif
#ifndef WM_KEYDOWN
#define WM_KEYDOWN 100193
#endif
#ifndef WM_KEYUP
#define WM_KEYUP 100194
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 100195
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 100196
#endif
#ifndef WM_PRINTCLIENT
#define WM_PRINTCLIENT 100197
#endif
#ifndef WM_RBUTTONDOWN
#define WM_RBUTTONDOWN 100198
#endif
#ifndef WM_SETREDRAW
#define WM_SETREDRAW 100199
#endif
#ifndef WM_SHOWWINDOW
#define WM_SHOWWINDOW 100200
#endif
#ifndef WM_SIZE
#define WM_SIZE 100201
#endif
#ifndef WM_VSCROLL
#define WM_VSCROLL 100202
#endif
#ifndef WS_BORDER
#define WS_BORDER 100203
#endif
#ifndef WS_CAPTION
#define WS_CAPTION 100204
#endif
#ifndef WS_CHILD
#define WS_CHILD 100205
#endif
#ifndef WS_EX_CLIENTEDGE
#define WS_EX_CLIENTEDGE 100206
#endif
#ifndef WS_EX_CONTROLPARENT
#define WS_EX_CONTROLPARENT 100207
#endif
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 100208
#endif
#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL 100209
#endif
#ifndef WS_EX_RTLREADING
#define WS_EX_RTLREADING 100210
#endif
#ifndef WS_HSCROLL
#define WS_HSCROLL 100211
#endif
#ifndef WS_VSCROLL
#define WS_VSCROLL 100212
#endif
#ifndef X509_ASN_ENCODING
#define X509_ASN_ENCODING 100213
#endif
#ifndef NPP_HAVE_AddClipboardFormatListener
NPP_STUB(AddClipboardFormatListener)
#endif
#ifndef NPP_HAVE_AssocQueryString
NPP_STUB(AssocQueryString)
#endif
#ifndef NPP_HAVE_BCryptCloseAlgorithmProvider
NPP_STUB(BCryptCloseAlgorithmProvider)
#endif
#ifndef NPP_HAVE_BCryptCreateHash
NPP_STUB(BCryptCreateHash)
#endif
#ifndef NPP_HAVE_BCryptDestroyHash
NPP_STUB(BCryptDestroyHash)
#endif
#ifndef NPP_HAVE_BCryptFinishHash
NPP_STUB(BCryptFinishHash)
#endif
#ifndef NPP_HAVE_BCryptGetProperty
NPP_STUB(BCryptGetProperty)
#endif
#ifndef NPP_HAVE_BCryptHashData
NPP_STUB(BCryptHashData)
#endif
#ifndef NPP_HAVE_BCryptOpenAlgorithmProvider
NPP_STUB(BCryptOpenAlgorithmProvider)
#endif
#ifndef NPP_HAVE_CertCloseStore
NPP_STUB(CertCloseStore)
#endif
#ifndef NPP_HAVE_CertFreeCertificateContext
NPP_STUB(CertFreeCertificateContext)
#endif
#ifndef NPP_HAVE_CertGetNameString
NPP_STUB(CertGetNameString)
#endif
#ifndef NPP_HAVE_CheckMenuItem
NPP_STUB(CheckMenuItem)
#endif
#ifndef NPP_HAVE_ClientToScreen
NPP_STUB(ClientToScreen)
#endif
#ifndef NPP_HAVE_CloseClipboard
NPP_STUB(CloseClipboard)
#endif
#ifndef NPP_HAVE_CoCreateGuid
NPP_STUB(CoCreateGuid)
#endif
#ifndef NPP_HAVE_CoTaskMemFree
NPP_STUB(CoTaskMemFree)
#endif
#ifndef NPP_HAVE_ColorHLSToRGB
NPP_STUB(ColorHLSToRGB)
#endif
#ifndef NPP_HAVE_ColorRGBToHLS
NPP_STUB(ColorRGBToHLS)
#endif
#ifndef NPP_HAVE_CompareFileTime
NPP_STUB(CompareFileTime)
#endif
#ifndef NPP_HAVE_ConvertStringSecurityDescriptorToSecurityDescriptorW
NPP_STUB(ConvertStringSecurityDescriptorToSecurityDescriptorW)
#endif
#ifndef NPP_HAVE_CreateAcceleratorTable
NPP_PSTUB(CreateAcceleratorTable)
#endif
#ifndef NPP_HAVE_CreateCompatibleBitmap
NPP_PSTUB(CreateCompatibleBitmap)
#endif
#ifndef NPP_HAVE_CreateCompatibleDC
NPP_PSTUB(CreateCompatibleDC)
#endif
#ifndef NPP_HAVE_CreateEvent
NPP_PSTUB(CreateEvent)
#endif
#ifndef NPP_HAVE_CreateFileMappingW
NPP_PSTUB(CreateFileMappingW)
#endif
#ifndef NPP_HAVE_CreateFontIndirectW
NPP_PSTUB(CreateFontIndirectW)
#endif
#ifndef NPP_HAVE_CreateMenu
NPP_PSTUB(CreateMenu)
#endif
#ifndef NPP_HAVE_CryptMsgClose
NPP_STUB(CryptMsgClose)
#endif
#ifndef NPP_HAVE_CryptMsgGetParam
NPP_STUB(CryptMsgGetParam)
#endif
#ifndef NPP_HAVE_CryptQueryObject
NPP_STUB(CryptQueryObject)
#endif
#ifndef NPP_HAVE_DefSubclassProc
NPP_STUB(DefSubclassProc)
#endif
#ifndef NPP_HAVE_DeleteDC
NPP_STUB(DeleteDC)
#endif
#ifndef NPP_HAVE_DeleteMenu
NPP_STUB(DeleteMenu)
#endif
#ifndef NPP_HAVE_DeleteObject
NPP_STUB(DeleteObject)
#endif
#ifndef NPP_HAVE_DestroyAcceleratorTable
NPP_STUB(DestroyAcceleratorTable)
#endif
#ifndef NPP_HAVE_DestroyIcon
NPP_STUB(DestroyIcon)
#endif
#ifndef NPP_HAVE_DestroyMenu
NPP_STUB(DestroyMenu)
#endif
#ifndef NPP_HAVE_DestroyWindow
NPP_STUB(DestroyWindow)
#endif
#ifndef NPP_HAVE_DrawMenuBar
NPP_STUB(DrawMenuBar)
#endif
#ifndef NPP_HAVE_DuplicateHandle
NPP_STUB(DuplicateHandle)
#endif
#ifndef NPP_HAVE_EmptyClipboard
NPP_STUB(EmptyClipboard)
#endif
#ifndef NPP_HAVE_EnableMenuItem
NPP_STUB(EnableMenuItem)
#endif
#ifndef NPP_HAVE_EnableWindow
NPP_STUB(EnableWindow)
#endif
#ifndef NPP_HAVE_EndDialog
NPP_STUB(EndDialog)
#endif
#ifndef NPP_HAVE_EnumDisplayMonitors
NPP_STUB(EnumDisplayMonitors)
#endif
#ifndef NPP_HAVE_EnumFontFamiliesEx
NPP_STUB(EnumFontFamiliesEx)
#endif
#ifndef NPP_HAVE_FillRect
NPP_STUB(FillRect)
#endif
#ifndef NPP_HAVE_FindWindow
NPP_PSTUB(FindWindow)
#endif
#ifndef NPP_HAVE_FormatMessageW
NPP_STUB(FormatMessageW)
#endif
#ifndef NPP_HAVE_FreeLibrary
NPP_STUB(FreeLibrary)
#endif
#ifndef NPP_HAVE_GetACP
NPP_STUB(GetACP)
#endif
#ifndef NPP_HAVE_GetClassNameA
NPP_STUB(GetClassNameA)
#endif
#ifndef NPP_HAVE_GetClassNameW
NPP_STUB(GetClassNameW)
#endif
#ifndef NPP_HAVE_GetClientRect
NPP_STUB(GetClientRect)
#endif
#ifndef NPP_HAVE_GetClipboardData
NPP_PSTUB(GetClipboardData)
#endif
#ifndef NPP_HAVE_GetCurrentProcess
NPP_PSTUB(GetCurrentProcess)
#endif
#ifndef NPP_HAVE_GetCurrentThreadId
NPP_STUB(GetCurrentThreadId)
#endif
#ifndef NPP_HAVE_GetDC
NPP_PSTUB(GetDC)
#endif
#ifndef NPP_HAVE_GetDateFormat
NPP_STUB(GetDateFormat)
#endif
#ifndef NPP_HAVE_GetDateFormatEx
NPP_STUB(GetDateFormatEx)
#endif
#ifndef NPP_HAVE_GetDeviceCaps
NPP_STUB(GetDeviceCaps)
#endif
#ifndef NPP_HAVE_GetDiskFreeSpaceExW
NPP_STUB(GetDiskFreeSpaceExW)
#endif
#ifndef NPP_HAVE_GetDlgCtrlID
NPP_STUB(GetDlgCtrlID)
#endif
#ifndef NPP_HAVE_GetDlgItem
NPP_PSTUB(GetDlgItem)
#endif
#ifndef NPP_HAVE_GetDlgItemText
NPP_STUB(GetDlgItemText)
#endif
#ifndef NPP_HAVE_GetDlgItemTextW
NPP_STUB(GetDlgItemTextW)
#endif
#ifndef NPP_HAVE_GetExitCodeProcess
NPP_STUB(GetExitCodeProcess)
#endif
#ifndef NPP_HAVE_GetFileVersionInfoSize
NPP_STUB(GetFileVersionInfoSize)
#endif
#ifndef NPP_HAVE_GetFileVersionInfoW
NPP_STUB(GetFileVersionInfoW)
#endif
#ifndef NPP_HAVE_GetFinalPathNameByHandle
NPP_STUB(GetFinalPathNameByHandle)
#endif
#ifndef NPP_HAVE_GetFocus
NPP_PSTUB(GetFocus)
#endif
#ifndef NPP_HAVE_GetKeyState
NPP_STUB(GetKeyState)
#endif
#ifndef NPP_HAVE_GetKeyboardLayout
NPP_STUB(GetKeyboardLayout)
#endif
#ifndef NPP_HAVE_GetLocalTime
NPP_STUB(GetLocalTime)
#endif
#ifndef NPP_HAVE_GetLocaleInfoEx
NPP_STUB(GetLocaleInfoEx)
#endif
#ifndef NPP_HAVE_GetLongPathName
NPP_STUB(GetLongPathName)
#endif
#ifndef NPP_HAVE_GetMenuItemCount
NPP_STUB(GetMenuItemCount)
#endif
#ifndef NPP_HAVE_GetMenuItemID
NPP_STUB(GetMenuItemID)
#endif
#ifndef NPP_HAVE_GetMenuState
NPP_STUB(GetMenuState)
#endif
#ifndef NPP_HAVE_GetMenuString
NPP_STUB(GetMenuString)
#endif
#ifndef NPP_HAVE_GetModuleHandle
NPP_PSTUB(GetModuleHandle)
#endif
#ifndef NPP_HAVE_GetModuleHandleW
NPP_PSTUB(GetModuleHandleW)
#endif
#ifndef NPP_HAVE_GetMonitorInfo
NPP_STUB(GetMonitorInfo)
#endif
#ifndef NPP_HAVE_GetObjectW
NPP_STUB(GetObjectW)
#endif
#ifndef NPP_HAVE_GetParent
NPP_PSTUB(GetParent)
#endif
#ifndef NPP_HAVE_GetPixel
NPP_STUB(GetPixel)
#endif
#ifndef NPP_HAVE_GetProductInfo
NPP_STUB(GetProductInfo)
#endif
#ifndef NPP_HAVE_GetProp
NPP_PSTUB(GetProp)
#endif
#ifndef NPP_HAVE_GetStockObject
NPP_PSTUB(GetStockObject)
#endif
#ifndef NPP_HAVE_GetSubMenu
NPP_PSTUB(GetSubMenu)
#endif
#ifndef NPP_HAVE_GetSystemInfo
NPP_STUB(GetSystemInfo)
#endif
#ifndef NPP_HAVE_GetTextExtentPoint32W
NPP_STUB(GetTextExtentPoint32W)
#endif
#ifndef NPP_HAVE_GetTextMetricsW
NPP_STUB(GetTextMetricsW)
#endif
#ifndef NPP_HAVE_GetTimeFormat
NPP_STUB(GetTimeFormat)
#endif
#ifndef NPP_HAVE_GetTimeFormatEx
NPP_STUB(GetTimeFormatEx)
#endif
#ifndef NPP_HAVE_GetVersionEx
NPP_STUB(GetVersionEx)
#endif
#ifndef NPP_HAVE_GetVersionExW
NPP_STUB(GetVersionExW)
#endif
#ifndef NPP_HAVE_GetWindowLongPtr
NPP_STUB(GetWindowLongPtr)
#endif
#ifndef NPP_HAVE_GetWindowLongPtrW
NPP_STUB(GetWindowLongPtrW)
#endif
#ifndef NPP_HAVE_GetWindowRect
NPP_STUB(GetWindowRect)
#endif
#ifndef NPP_HAVE_GetWindowSubclass
NPP_STUB(GetWindowSubclass)
#endif
#ifndef NPP_HAVE_GetWindowText
NPP_STUB(GetWindowText)
#endif
#ifndef NPP_HAVE_GetWindowTextLengthW
NPP_STUB(GetWindowTextLengthW)
#endif
#ifndef NPP_HAVE_GetWindowTextW
NPP_STUB(GetWindowTextW)
#endif
#ifndef NPP_HAVE_GlobalAlloc
NPP_PSTUB(GlobalAlloc)
#endif
#ifndef NPP_HAVE_GlobalFree
NPP_STUB(GlobalFree)
#endif
#ifndef NPP_HAVE_GlobalLock
NPP_PSTUB(GlobalLock)
#endif
#ifndef NPP_HAVE_GlobalUnlock
NPP_STUB(GlobalUnlock)
#endif
#ifndef NPP_HAVE_ImageList_Destroy
NPP_STUB(ImageList_Destroy)
#endif
#ifndef NPP_HAVE_ImageList_RemoveAll
NPP_STUB(ImageList_RemoveAll)
#endif
#ifndef NPP_HAVE_ImageList_SetIconSize
NPP_STUB(ImageList_SetIconSize)
#endif
#ifndef NPP_HAVE_InflateRect
NPP_STUB(InflateRect)
#endif
#ifndef NPP_HAVE_InsertMenu
NPP_STUB(InsertMenu)
#endif
#ifndef NPP_HAVE_IntersectRect
NPP_STUB(IntersectRect)
#endif
#ifndef NPP_HAVE_InvalidateRect
NPP_STUB(InvalidateRect)
#endif
#ifndef NPP_HAVE_IsCharLowerW
NPP_STUB(IsCharLowerW)
#endif
#ifndef NPP_HAVE_IsClipboardFormatAvailable
NPP_STUB(IsClipboardFormatAvailable)
#endif
#ifndef NPP_HAVE_IsTextUnicode
NPP_STUB(IsTextUnicode)
#endif
#ifndef NPP_HAVE_IsWindow
NPP_STUB(IsWindow)
#endif
#ifndef NPP_HAVE_IsWindowVisible
NPP_STUB(IsWindowVisible)
#endif
#ifndef NPP_HAVE_ListView_DeleteItem
NPP_STUB(ListView_DeleteItem)
#endif
#ifndef NPP_HAVE_ListView_EnsureVisible
NPP_STUB(ListView_EnsureVisible)
#endif
#ifndef NPP_HAVE_ListView_GetItemCount
NPP_STUB(ListView_GetItemCount)
#endif
#ifndef NPP_HAVE_ListView_GetSelectionMark
NPP_STUB(ListView_GetSelectionMark)
#endif
#ifndef NPP_HAVE_ListView_SetColumn
NPP_STUB(ListView_SetColumn)
#endif
#ifndef NPP_HAVE_ListView_SetItemState
NPP_STUB(ListView_SetItemState)
#endif
#ifndef NPP_HAVE_ListView_SetSelectionMark
NPP_STUB(ListView_SetSelectionMark)
#endif
#ifndef NPP_HAVE_LoadIconWithScaleDown
NPP_PSTUB(LoadIconWithScaleDown)
#endif
#ifndef NPP_HAVE_LoadImage
NPP_PSTUB(LoadImage)
#endif
#ifndef NPP_HAVE_LocalAlloc
NPP_PSTUB(LocalAlloc)
#endif
#ifndef NPP_HAVE_LocalFree
NPP_STUB(LocalFree)
#endif
#ifndef NPP_HAVE_MapViewOfFile
NPP_PSTUB(MapViewOfFile)
#endif
#ifndef NPP_HAVE_MapVirtualKeyW
NPP_STUB(MapVirtualKeyW)
#endif
#ifndef NPP_HAVE_MessageBox
NPP_STUB(MessageBox)
#endif
#ifndef NPP_HAVE_MessageBoxW
NPP_STUB(MessageBoxW)
#endif
#ifndef NPP_HAVE_ModifyMenu
NPP_STUB(ModifyMenu)
#endif
#ifndef NPP_HAVE_MonitorFromWindow
NPP_PSTUB(MonitorFromWindow)
#endif
#ifndef NPP_HAVE_MoveWindow
NPP_STUB(MoveWindow)
#endif
#ifndef NPP_HAVE_OpenClipboard
NPP_PSTUB(OpenClipboard)
#endif
#ifndef NPP_HAVE_PathCanonicalize
NPP_STUB(PathCanonicalize)
#endif
#ifndef NPP_HAVE_PathCompactPathEx
NPP_STUB(PathCompactPathEx)
#endif
#ifndef NPP_HAVE_PathCompactPathExW
NPP_STUB(PathCompactPathExW)
#endif
#ifndef NPP_HAVE_PathCreateFromUrlW
NPP_STUB(PathCreateFromUrlW)
#endif
#ifndef NPP_HAVE_PathGetDriveNumber
NPP_STUB(PathGetDriveNumber)
#endif
#ifndef NPP_HAVE_PathIsNetworkPath
NPP_STUB(PathIsNetworkPath)
#endif
#ifndef NPP_HAVE_PostMessage
NPP_STUB(PostMessage)
#endif
#ifndef NPP_HAVE_RegCloseKey
NPP_STUB(RegCloseKey)
#endif
#ifndef NPP_HAVE_RegGetValueW
NPP_STUB(RegGetValueW)
#endif
#ifndef NPP_HAVE_RegOpenKeyExA
NPP_STUB(RegOpenKeyExA)
#endif
#ifndef NPP_HAVE_RegOpenKeyExW
NPP_STUB(RegOpenKeyExW)
#endif
#ifndef NPP_HAVE_RegQueryInfoKey
NPP_STUB(RegQueryInfoKey)
#endif
#ifndef NPP_HAVE_RegQueryValueExA
NPP_STUB(RegQueryValueExA)
#endif
#ifndef NPP_HAVE_RegQueryValueExW
NPP_STUB(RegQueryValueExW)
#endif
#ifndef NPP_HAVE_RegisterClipboardFormat
NPP_STUB(RegisterClipboardFormat)
#endif
#ifndef NPP_HAVE_ReleaseDC
NPP_STUB(ReleaseDC)
#endif
#ifndef NPP_HAVE_RemoveClipboardFormatListener
NPP_STUB(RemoveClipboardFormatListener)
#endif
#ifndef NPP_HAVE_RemoveMenu
NPP_STUB(RemoveMenu)
#endif
#ifndef NPP_HAVE_RemoveProp
NPP_PSTUB(RemoveProp)
#endif
#ifndef NPP_HAVE_RemoveWindowSubclass
NPP_STUB(RemoveWindowSubclass)
#endif
#ifndef NPP_HAVE_ReplaceFile
NPP_STUB(ReplaceFile)
#endif
#ifndef NPP_HAVE_SHFileOperation
NPP_STUB(SHFileOperation)
#endif
#ifndef NPP_HAVE_SHOpenFolderAndSelectItems
NPP_STUB(SHOpenFolderAndSelectItems)
#endif
#ifndef NPP_HAVE_SHParseDisplayName
NPP_STUB(SHParseDisplayName)
#endif
#ifndef NPP_HAVE_ScreenToClient
NPP_STUB(ScreenToClient)
#endif
#ifndef NPP_HAVE_ScrollWindow
NPP_STUB(ScrollWindow)
#endif
#ifndef NPP_HAVE_SelectObject
NPP_PSTUB(SelectObject)
#endif
#ifndef NPP_HAVE_SendDlgItemMessage
NPP_STUB(SendDlgItemMessage)
#endif
#ifndef NPP_HAVE_SetClipboardData
NPP_PSTUB(SetClipboardData)
#endif
#ifndef NPP_HAVE_SetDlgItemText
NPP_STUB(SetDlgItemText)
#endif
#ifndef NPP_HAVE_SetEvent
NPP_STUB(SetEvent)
#endif
#ifndef NPP_HAVE_SetFocus
NPP_PSTUB(SetFocus)
#endif
#ifndef NPP_HAVE_SetLayeredWindowAttributes
NPP_STUB(SetLayeredWindowAttributes)
#endif
#ifndef NPP_HAVE_SetParent
NPP_PSTUB(SetParent)
#endif
#ifndef NPP_HAVE_SetProp
NPP_STUB(SetProp)
#endif
#ifndef NPP_HAVE_SetScrollInfo
NPP_STUB(SetScrollInfo)
#endif
#ifndef NPP_HAVE_SetScrollPos
NPP_STUB(SetScrollPos)
#endif
#ifndef NPP_HAVE_SetTimer
NPP_STUB(SetTimer)
#endif
#ifndef NPP_HAVE_SetWindowLongPtr
NPP_STUB(SetWindowLongPtr)
#endif
#ifndef NPP_HAVE_SetWindowPos
NPP_STUB(SetWindowPos)
#endif
#ifndef NPP_HAVE_SetWindowSubclass
NPP_STUB(SetWindowSubclass)
#endif
#ifndef NPP_HAVE_SetWindowText
NPP_STUB(SetWindowText)
#endif
#ifndef NPP_HAVE_SetWindowsHookEx
NPP_PSTUB(SetWindowsHookEx)
#endif
#ifndef NPP_HAVE_ShellExecuteExW
NPP_STUB(ShellExecuteExW)
#endif
#ifndef NPP_HAVE_ShowWindow
NPP_STUB(ShowWindow)
#endif
#ifndef NPP_HAVE_StringCchCopyW
NPP_STUB(StringCchCopyW)
#endif
#ifndef NPP_HAVE_StringFromGUID2
NPP_STUB(StringFromGUID2)
#endif
#ifndef NPP_HAVE_SystemParametersInfoW
NPP_STUB(SystemParametersInfoW)
#endif
#ifndef NPP_HAVE_SystemTimeToTzSpecificLocalTime
NPP_STUB(SystemTimeToTzSpecificLocalTime)
#endif
#ifndef NPP_HAVE_TabCtrl_SetItem
NPP_STUB(TabCtrl_SetItem)
#endif
#ifndef NPP_HAVE_TrackPopupMenu
NPP_STUB(TrackPopupMenu)
#endif
#ifndef NPP_HAVE_UnhookWindowsHookEx
NPP_STUB(UnhookWindowsHookEx)
#endif
#ifndef NPP_HAVE_UnmapViewOfFile
NPP_STUB(UnmapViewOfFile)
#endif
#ifndef NPP_HAVE_UpdateWindow
NPP_STUB(UpdateWindow)
#endif
#ifndef NPP_HAVE_VerQueryValueW
NPP_STUB(VerQueryValueW)
#endif
#ifndef NPP_HAVE_WindowFromPoint
NPP_PSTUB(WindowFromPoint)
#endif
#ifndef NPP_HAVE__create_locale
NPP_PSTUB(_create_locale)
#endif
#ifndef NPP_HAVE__free_locale
NPP_STUB(_free_locale)
#endif
#ifndef NPP_HAVE__itow
NPP_STUB(_itow)
#endif
#ifndef NPP_HAVE__wcstod_l
NPP_STUB(_wcstod_l)
#endif
#ifndef NPP_HAVE__wtoi
NPP_STUB(_wtoi)
#endif
