/*
 * xvkbd - Virtual Keyboard for X Window System
 * (Version 4.1, 2020-05-04)
 *
 * Copyright (C) 2000-2020 by Tom Sato <VEF00200@nifty.com>
 * http://t-sato.in.coocan.jp/xvkbd/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fnmatch.h>
#include <limits.h>
#include <sys/time.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>  /* to get request code */

#include <X11/Xmu/WinUtil.h>
#include <X11/Xatom.h>

#ifdef USE_I18N
# include <X11/Xlocale.h>
#endif

#ifdef USE_XTEST
# include <X11/extensions/XTest.h>
#endif

#include "resources.h"
#define PROGRAM_NAME_WITH_VERSION "xvkbd (v4.1)"

// #define PRIVATE_DICT ".xvkbd.words"

#ifndef PATH_MAX
# define PATH_MAX 300
#endif

/*
 * Default keyboard layout is hardcoded here.
 * Layout of the main keyboard can be redefined by resources.
 */
#define NUM_KEY_ROWS    25
#define NUM_KEY_COLS    25

static char *keys_normal[NUM_KEY_ROWS][NUM_KEY_COLS] = {
  { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "BackSpace" },
  { "Escape", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\\", "`" },
  { "Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Delete" },
  { "Control_L", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "Return" },
  { "Shift_L", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Multi_key", "Shift_R" },
  { "MainMenu", "Caps_Lock", "Alt_L", "Meta_L", "space", "Meta_R", "Alt_R",
    "Left", "Right", "Up", "Down", "Focus" },
};
static char *keys_shift[NUM_KEY_ROWS][NUM_KEY_COLS] = {
  { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "BackSpace" },
  { "Escape", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "|", "~" },
  { "ISO_Left_Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "Delete" },
  { "Control_L", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "Return" },
  { "Shift_L", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Multi_key", "Shift_R" },
  { "MainMenu", "Caps_Lock", "Alt_L", "Meta_L", "space", "Meta_R", "Alt_R",
    "Left", "Right", "Up", "Down", "Focus" },
};
static char *keys_altgr[NUM_KEY_ROWS][NUM_KEY_COLS] = { { NULL } };
static char *keys_shift_altgr[NUM_KEY_ROWS][NUM_KEY_COLS] = { { NULL } };

static char *key_labels[NUM_KEY_ROWS][NUM_KEY_COLS] = {
  { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Backspace" },
  { "Esc", "!\n1", "@\n2", "#\n3", "$\n4", "%\n5", "^\n6",
    "&\n7", "*\n8", "(\n9", ")\n0", "_\n-", "+\n=", "|\n\\", "~\n`" },
  { "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{\n[", "}\n]", "Del" },
  { "Control", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":\n;", "\"\n'", "Return" },
  { "Shift", "Z", "X", "C", "V", "B", "N", "M", "<\n,", ">\n.", "?\n/", "Com\npose", "Shift" },
  { "MainMenu", "Caps\nLock", "Alt", "Meta", "", "Meta", "Alt",
    "left", "right", "up", "down", "Focus" },
};
static char *normal_key_labels[NUM_KEY_ROWS][NUM_KEY_COLS] = {
  { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Backspace" },
  { "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\\", "`" },
  { "Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Del" },
  { "Ctrl", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "Return" },
  { "Shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Comp", "Shift" },
  { "MainMenu", "Caps", "Alt", "Meta", "", "Meta", "Alt",
    "left", "right", "up", "down", "Focus" },
};
static char *shift_key_labels[NUM_KEY_ROWS][NUM_KEY_COLS] = {
  { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Backspace" },
  { "Esc", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "|", "~" },
  { "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "Del" },
  { "Ctrl", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "Return" },
  { "Shift", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Comp", "Shift" },
  { "MainMenu", "Caps", "Alt", "Meta", "", "Meta", "Alt",
    "left", "right", "up", "down", "Focus" },
};
static char *altgr_key_labels[NUM_KEY_ROWS][NUM_KEY_COLS] = { { NULL } };
static char *shift_altgr_key_labels[NUM_KEY_ROWS][NUM_KEY_COLS] = { { NULL } };


#define NUM_KEYPAD_ROWS  NUM_KEY_ROWS
#define NUM_KEYPAD_COLS  NUM_KEY_COLS

static char *keypad[NUM_KEYPAD_ROWS][NUM_KEYPAD_COLS] = {
  { "Num_Lock",  "KP_Divide",   "KP_Multiply", "Focus"       },
  { "Home",      "Up",          "Page_Up",     "KP_Add"      },
  { "Left",      "5",           "Right",       "KP_Subtract" },
  { "End",       "Down",        "Page_Down",   "KP_Enter"    },
  { "Insert",                   "Delete"                     },
};
static char *keypad_shift[NUM_KEYPAD_ROWS][NUM_KEYPAD_COLS] = {
  { "Num_Lock",  "KP_Divide",   "KP_Multiply", "Focus"       },
  { "KP_7",      "KP_8",        "KP_9",        "KP_Add"      },
  { "KP_4",      "KP_5",        "KP_6",        "KP_Subtract" },
  { "KP_1",      "KP_2",        "KP_3",        "KP_Enter"    },
  { "KP_0",                     "."                          },
};
static char *keypad_label[NUM_KEYPAD_ROWS][NUM_KEYPAD_COLS] = {
  { "Num\nLock", "/",           "*",           "Focus"       },
  { "7\nHome",   "8\nUp  ",     "9\nPgUp",     "+"           },
  { "4\nLeft",   "5\n    ",     "6\nRight",    "-"           },
  { "1\nEnd ",   "2\nDown",     "3\nPgDn",     "Enter"       },
  { "0\nIns ",                  ".\nDel "                    },
};

#define NUM_SUN_FKEY_ROWS 6
#define NUM_SUN_FKEY_COLS 3

static char *sun_fkey[NUM_SUN_FKEY_ROWS][NUM_SUN_FKEY_COLS] = {
  { "L1", "L2"  },
  { "L3", "L4"  },
  { "L5", "L6"  },
  { "L7", "L8"  },
  { "L9", "L10" },
  { "Help"      },
};
static char *sun_fkey_label[NUM_SUN_FKEY_ROWS][NUM_SUN_FKEY_COLS] = {
  { "Stop \nL1", "Again\nL2"  },
  { "Props\nL3", "Undo \nL4"  },
  { "Front\nL5", "Copy \nL6"  },
  { "Open \nL7", "Paste\nL8"  },
  { "Find \nL9", "Cut  \nL10" },
  { "Help"                    },
};

/*
 * Image for arrow keys
 */
#define up_width 7
#define up_height 13
static unsigned char up_bits[] = {
   0x08, 0x1c, 0x1c, 0x3e, 0x2a, 0x49, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
   0x08};

#define down_width 7
#define down_height 13
static unsigned char down_bits[] = {
   0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x49, 0x2a, 0x3e, 0x1c, 0x1c,
   0x08};

#define left_width 13
#define left_height 13
static unsigned char left_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x18, 0x00, 0x0e, 0x00,
   0xff, 0x1f, 0x0e, 0x00, 0x18, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00};

#define right_width 13
#define right_height 13
static unsigned char right_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x03, 0x00, 0x0e,
   0xff, 0x1f, 0x00, 0x0e, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00};

#define check_width 16
#define check_height 16
static unsigned char check_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x38, 0x00, 0x1e, 0x08, 0x0f,
  0x8c, 0x07, 0xde, 0x03, 0xfe, 0x03, 0xfc, 0x01, 0xf8, 0x00, 0xf0, 0x00,
  0x70, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00};

#define back_width 18
#define back_height 13
static unsigned char back_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00,
   0x78, 0x00, 0x00, 0xfe, 0xff, 0x03, 0xff, 0xff, 0x03, 0xfe, 0xff, 0x03,
   0x78, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00};

/*
 * Resources and options
 */
#define Offset(entry) XtOffset(struct appres_struct *, entry)
static XtResource application_resources[] = {
/*
  { "description", "Description", XtRString, sizeof(char *),
    Offset(description), XtRImmediate,
    PROGRAM_NAME_WITH_VERSION " - virtual keyboard for X window system\n\n"
    "Copyright (C) 2000-2019 by Tom Sato <VEF00200@nifty.com>\n"
    "http://homepage3.nifty.com/tsato/xvkbd/\n\n"
    "This program is free software with ABSOLUTELY NO WARRANTY,\n"
    "distributed under the terms of the GNU General Public License.\n" },
  { "showManualCommand", "ShowManualCommand", XtRString, sizeof(char *),
    Offset(show_manual_command), XtRImmediate, "xterm -e man xvkbd &" },

  { "windowGeometry", "Geometry", XtRString, sizeof(char *),
    Offset(geometry), XtRImmediate, "" },
  { "inheritGeoemetry", "Inherit", XtRBoolean, sizeof(Boolean),
     Offset(inherit_geometry), XtRImmediate, (XtPointer)TRUE },
*/
  { "debug", "Debug", XtRBoolean, sizeof(Boolean),
     Offset(debug), XtRImmediate, (XtPointer)FALSE },
  { "version", "Version", XtRBoolean, sizeof(Boolean),
     Offset(version), XtRImmediate, (XtPointer)FALSE },

#ifdef USE_XTEST
  { "xtest", "XTest", XtRBoolean, sizeof(Boolean),
     Offset(xtest), XtRImmediate, (XtPointer)TRUE },
#else
  { "xtest", "XTest", XtRBoolean, sizeof(Boolean),
     Offset(xtest), XtRImmediate, (XtPointer)FALSE },
#endif
  { "noSync", "NoSync", XtRBoolean, sizeof(Boolean),
     Offset(no_sync), XtRImmediate, (XtPointer)FALSE },
/*
  { "alwaysOnTop", "AlwaysOnTop", XtRBoolean, sizeof(Boolean),
     Offset(always_on_top), XtRImmediate, (XtPointer)FALSE },
  { "wmToolbar", "WmToolbar", XtRBoolean, sizeof(Boolean),
     Offset(wm_toolbar), XtRImmediate, (XtPointer)FALSE },
*/
/*
  { "jumpPointer", "JumpPointer", XtRBoolean, sizeof(Boolean),
     Offset(jump_pointer), XtRImmediate, (XtPointer)TRUE },
  { "jumpPointerAlways", "JumpPointer", XtRBoolean, sizeof(Boolean),
     Offset(jump_pointer_always), XtRImmediate, (XtPointer)TRUE },
  { "jumpPointerBack", "JumpPointer", XtRBoolean, sizeof(Boolean),
     Offset(jump_pointer_back), XtRImmediate, (XtPointer)TRUE },
*/
  { "quickModifiers", "QuickModifiers", XtRBoolean, sizeof(Boolean),
     Offset(quick_modifiers), XtRImmediate, (XtPointer)TRUE },
  { "altgrLock", "ModifiersLock", XtRBoolean, sizeof(Boolean),
     Offset(altgr_lock), XtRImmediate, (XtPointer)FALSE },
  { "shiftLock", "ModifiersLock", XtRBoolean, sizeof(Boolean),
     Offset(shift_lock), XtRImmediate, (XtPointer)FALSE },
  { "modifiersLock", "ModifiersLock", XtRBoolean, sizeof(Boolean),
     Offset(modifiers_lock), XtRImmediate, (XtPointer)FALSE },

  { "numLockState", "NumLockState", XtRBoolean, sizeof(Boolean),
     Offset(num_lock_state), XtRImmediate, (XtPointer)TRUE },
  { "autoRepeat", "AutoRepeat", XtRBoolean, sizeof(Boolean),
     Offset(auto_repeat), XtRImmediate, (XtPointer)TRUE },
  { "modalKeytop", "ModalKeytop", XtRBoolean, sizeof(Boolean),
     Offset(modal_keytop), XtRImmediate, (XtPointer)FALSE },
  { "minimizable", "Minimizable", XtRBoolean, sizeof(Boolean),
     Offset(minimizable), XtRImmediate, (XtPointer)FALSE },
  { "secure", "Secure", XtRBoolean, sizeof(Boolean),
     Offset(secure), XtRImmediate, (XtPointer)FALSE },
  { "nonexitable", "Secure", XtRBoolean, sizeof(Boolean),
     Offset(nonexitable), XtRImmediate, (XtPointer)FALSE },
  { "modalKeytop", "ModalKeytop", XtRBoolean, sizeof(Boolean),
     Offset(modal_keytop), XtRImmediate, (XtPointer)FALSE },
  { "modalThreshold", "ModalThreshold", XtRInt, sizeof(int),
     Offset(modal_threshold), XtRImmediate, (XtPointer)150 },
  { "keypad", "Keypad", XtRBoolean, sizeof(Boolean),
     Offset(keypad), XtRImmediate, (XtPointer)TRUE },
  { "functionkey", "FunctionKey", XtRBoolean, sizeof(Boolean),
     Offset(function_key), XtRImmediate, (XtPointer)TRUE },

  { "compact", "Compact", XtRBoolean, sizeof(Boolean),
     Offset(compact), XtRImmediate, (XtPointer)FALSE },
  { "keypadOnly", "KeypadOnly", XtRBoolean, sizeof(Boolean),
     Offset(keypad_only), XtRImmediate, (XtPointer)FALSE },
  { "keypadKeysym", "KeypadKeysym", XtRBoolean, sizeof(Boolean),
     Offset(keypad_keysym), XtRImmediate, (XtPointer)FALSE },
  { "autoAddKeysym", "AutoAddKeysym", XtRBoolean, sizeof(Boolean),
     Offset(auto_add_keysym), XtRImmediate, (XtPointer)TRUE },
  { "listWidgets", "Debug", XtRBoolean, sizeof(Boolean),
     Offset(list_widgets), XtRImmediate, (XtPointer)FALSE },
/*
  { "resizable", "Resizable", XtRBoolean, sizeof(Boolean),
    Offset(resizable), XtRImmediate, (XtPointer)TRUE },
*/
  { "positiveModifiers", "PositiveModifiers", XtRString, sizeof(char *),
    Offset(positive_modifiers), XtRImmediate, "" },
  { "utf16", "Utf16", XtRBoolean, sizeof(Boolean),
     Offset(utf16), XtRImmediate, (XtPointer)FALSE },
  { "text", "Text", XtRString, sizeof(char *),
    Offset(text), XtRImmediate, "" },
  { "file", "File", XtRString, sizeof(char *),
    Offset(file), XtRImmediate, "" },
/*
  { "window", "Window", XtRString, sizeof(char *),
    Offset(window), XtRImmediate, "" },
  { "widget", "Widget", XtRString, sizeof(char *),
    Offset(widget), XtRImmediate, "" },
  { "remoteDisplay", "RemoteDisplay", XtRString, sizeof(char *),
    Offset(remote_display), XtRImmediate, "" },
*/
/*
  { "generalFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      Offset(general_font), XtRString, XtDefaultFont},
  { "letterFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      Offset(letter_font), XtRString, XtDefaultFont},
  { "specialFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      Offset(special_font), XtRString, XtDefaultFont},
  { "keypadFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      Offset(keypad_font), XtRString, XtDefaultFont},
*/
/*
  { "generalBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(general_background), XtRString, "gray" },
  { "specialBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(special_background), XtRString, "gray" },
  { "specialForeground", XtCForeground, XtRPixel, sizeof(Pixel),
     Offset(special_foreground), XtRString, "black" },
*/
/*
#ifdef USE_I18N
  { "specialFontSet", XtCFontSet, XtRFontSet, sizeof(XFontSet),
      Offset(special_fontset), XtRString, XtDefaultFontSet},
#endif
*/
/*
  { "highlightBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(highlight_background), XtRString, "gray" },
  { "highlightForeground", XtCForeground, XtRPixel, sizeof(Pixel),
     Offset(highlight_foreground), XtRString, "forestgreen" },
  { "focusBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(focus_background), XtRString, "gray" },
  { "remoteFocusBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(remote_focus_background), XtRString, "cyan" },
  { "balloonBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(balloon_background), XtRString, "LightYellow1" },
  { "launchBalloonBackground", XtCBackground, XtRPixel, sizeof(Pixel),
     Offset(launch_balloon_background), XtRString, "SkyBlue1" },
*/
  { "normalkeys", "NormalKeys", XtRString, sizeof(char *),
    Offset(keys_normal), XtRImmediate, "" },
  { "shiftkeys", "ShiftKeys", XtRString, sizeof(char *),
    Offset(keys_shift), XtRImmediate, "" },
  { "altgrkeys", "AltgrKeys", XtRString, sizeof(char *),
    Offset(keys_altgr), XtRImmediate, "" },
  { "shiftaltgrkeys", "ShiftAltgrKeys", XtRString, sizeof(char *),
    Offset(keys_shift_altgr), XtRImmediate, "" },
  { "keylabels", "KeyLabels", XtRString, sizeof(char *),
    Offset(key_labels), XtRImmediate, "" },
  { "normalkeylabels", "NormalKeyLabels", XtRString, sizeof(char *),
    Offset(normal_key_labels), XtRImmediate, "" },
  { "shiftkeylabels", "ShiftKeyLabels", XtRString, sizeof(char *),
    Offset(shift_key_labels), XtRImmediate, "" },
  { "altgrkeylabels", "AltgrKeyLabels", XtRString, sizeof(char *),
    Offset(altgr_key_labels), XtRImmediate, "" },
  { "shiftaltgrkeylabels", "ShiftAltgrKeyLabels", XtRString, sizeof(char *),
    Offset(shift_altgr_key_labels), XtRImmediate, "" },

  { "normalkeypad", "NormalKeypad", XtRString, sizeof(char *),
    Offset(keypad_normal), XtRImmediate, "" },
  { "shiftkeypad", "ShiftKeypad", XtRString, sizeof(char *),
    Offset(keypad_shift), XtRImmediate, "" },
  { "keypad_labels", "KeypadLabels", XtRString, sizeof(char *),
    Offset(keypad_labels), XtRImmediate, "" },

  { "deadkeys", "DeadKeys", XtRString, sizeof(char *),
    Offset(deadkeys), XtRImmediate, "" },
  { "altgrKeycode", "AltgrKeycode", XtRInt, sizeof(int),
    Offset(altgr_keycode), XtRImmediate, (XtPointer)0 },
/*
  { "keyFile", "KeyFile", XtRString, sizeof(char *),
    Offset(key_file), XtRImmediate, ".xvkbd" },
  { "dictFile", "DictFile", XtRString, sizeof(char *),
    Offset(dict_file), XtRImmediate, SHAREDIR "/words.english" },
*/
  { "customizations", "Customizations", XtRString, sizeof(char *),
    Offset(customizations), XtRImmediate, "default" },
  { "editableFunctionKeys", "EditableFunctionKeys", XtRInt, sizeof(int),
     Offset(editable_function_keys), XtRImmediate, (XtPointer)12 },

  { "maxWidthRatio", "MaxRatio", XtRFloat, sizeof(float),
     Offset(max_width_ratio), XtRString, "0.9" },
  { "maxHeightRatio", "MaxRatio", XtRFloat, sizeof(float),
     Offset(max_height_ratio), XtRString, "0.5" },
  { "textDelay", "TextDelay", XtRInt, sizeof(int),
     Offset(text_delay), XtRImmediate, (XtPointer)10 },

  { "keyClickPitch", "KeyClickPitch", XtRInt, sizeof(int),
     Offset(key_click_pitch), XtRImmediate, (XtPointer)1000 },
  { "keyClickDuration", "KeyClickDuration", XtRInt, sizeof(int),
     Offset(key_click_duration), XtRImmediate, (XtPointer)1 },
  { "autoClickDelay", "AutoClickDelay", XtRInt, sizeof(int),
     Offset(autoclick_delay), XtRImmediate, (XtPointer)0 },
/*
  { "savePrivateDictInterval", "SavePrivateDictInterval", XtRInt, sizeof(int),
     Offset(save_private_dict_interval), XtRImmediate, (XtPointer)5000 },
  { "privateDictDecay", "PrivateDictDecay", XtRFloat, sizeof(float),
    Offset(private_dict_decay), XtRString, "0.99" },
  { "insertBlankAfterCompletion", "InsertBlankAfterCompletion", XtRBoolean, sizeof(Boolean),
     Offset(insert_blank_after_completion), XtRImmediate, (XtPointer)TRUE },
  { "integrateCompletionPanel", "IntegrateCompletionPanel", XtRBoolean, sizeof(Boolean),
     Offset(integrate_completion_panel), XtRImmediate, (XtPointer)TRUE },
*/
};
#undef Offset

static XrmOptionDescRec options[] = {
  { "-geometry", ".windowGeometry", XrmoptionSepArg, NULL },
  { "-windowgeometry", ".windowGeometry", XrmoptionSepArg, NULL },
  { "-debug", ".debug", XrmoptionNoArg, "True" },
#ifdef USE_XTEST
  { "-xtest", ".xtest", XrmoptionNoArg, "True" },
  { "-xsendevent", ".xtest", XrmoptionNoArg, "False" },
  { "-no-jump-pointer", ".jumpPointer", XrmoptionNoArg, "False" },
  { "-no-back-pointer", ".jumpPointerBack", XrmoptionNoArg, "False" },
#endif
  { "-no-sync", ".noSync", XrmoptionNoArg, "True" },
  { "-always-on-top", ".alwaysOnTop", XrmoptionNoArg, "True" },  // EXPERIMENTAL
  { "-no-resize", ".resizable", XrmoptionNoArg, "False" },
  { "-quick", ".quickModifiers", XrmoptionNoArg, "True" },
  { "-modifiers", ".positiveModifiers", XrmoptionSepArg, NULL },
  { "-utf16", ".utf16", XrmoptionNoArg, "True" },
  { "-text", ".text", XrmoptionSepArg, NULL },
  { "-file", ".file", XrmoptionSepArg, NULL },
  { "-delay", ".textDelay", XrmoptionSepArg, NULL },
  { "-window", ".window", XrmoptionSepArg, NULL },
  { "-widget", ".widget", XrmoptionSepArg, NULL },
  { "-remote-display", ".remoteDisplay", XrmoptionSepArg, NULL },
  { "-altgr-lock", ".altgrLock", XrmoptionNoArg, "True" },
  { "-no-altgr-lock", ".altgrLock", XrmoptionNoArg, "False" },
  { "-no-repeat", ".autoRepeat", XrmoptionNoArg, "False" },
  { "-norepeat", ".autoRepeat", XrmoptionNoArg, "False" },
  { "-no-keypad", ".keypad", XrmoptionNoArg, "False" },
  { "-nokeypad", ".keypad", XrmoptionNoArg, "False" },
  { "-no-functionkey", ".functionkey", XrmoptionNoArg, "False" },
  { "-nofunctionkey", ".functionkey", XrmoptionNoArg, "False" },
  { "-highlight", ".highlightForeground", XrmoptionSepArg, NULL },
  { "-compact", ".compact", XrmoptionNoArg, "True" },
  { "-keypad", ".keypadOnly", XrmoptionNoArg, "True" },
  { "-true-keypad", ".keypadKeysym", XrmoptionNoArg, "True" },
  { "-truekeypad", ".keypadKeysym", XrmoptionNoArg, "True" },
  { "-no-add-keysym", ".autoAddKeysym", XrmoptionNoArg, "False" },
  { "-altgr-keycode", ".altgrKeycode", XrmoptionSepArg, NULL },
  { "-list", ".listWidgets", XrmoptionNoArg, "True" },
  { "-modal", ".modalKeytop", XrmoptionNoArg, "True" },
  { "-minimizable", ".minimizable", XrmoptionNoArg, "True" },
  { "-secure", ".secure", XrmoptionNoArg, "True" },
  { "-nonexitable", ".nonexitable", XrmoptionNoArg, "True" },
  { "-xdm", ".Secure", XrmoptionNoArg, "True" },
  { "-completion", ".integrateCompletionPanel", XrmoptionNoArg, "True" },
  { "-dict", ".dictFile", XrmoptionSepArg, NULL },
  { "-keyfile", ".keyFile", XrmoptionSepArg, NULL },
  { "-customizations", ".customizations", XrmoptionSepArg, NULL },
  { "-version", ".version", XrmoptionNoArg, "True" },
  { "-help", ".version", XrmoptionNoArg, "True" },
};

/*
 * Global variables
 */
struct appres_struct appres;

static int argc1;
static char **argv1;

static XtAppContext app_con;
static Widget toplevel = None;
static Widget key_widgets[NUM_KEY_ROWS][NUM_KEY_COLS];
static Widget main_menu = None;

static Dimension toplevel_height = 1000;

static Display *dpy;
static Atom wm_delete_window = None;

static KeySym *keysym_table = NULL;
static int min_keycode, max_keycode;
static int keysym_per_keycode;
static Boolean error_detected;

static int alt_mask = 0;
static int meta_mask = 0;
static int super_mask = 0;
static int altgr_mask = 0;
static int level3_shift_mask = 0;
static KeySym altgr_keysym = NoSymbol;

static int shift_state = 0;
static int mouse_shift = 0;

static Display *target_dpy = NULL;

static Window toplevel_parent = None;
static Window focused_window = None;
static Window focused_subwindow = None;

// static Pixmap xvkbd_pixmap = None;

static int AddKeysym(KeySym keysym, Boolean top);  /* forward */

/*
 * Search for window which has specified instance name (WM_NAME)
 * or class name (WM_CLASS).
 */
static Window FindWindow(Window top, char *name)
{
  Window w;
  Window *children, dummy;
  unsigned int nchildren;
  int i;
  XClassHint hint;
  char *win_name;

  w = None;

  if (appres.debug) fprintf(stderr, "xvkbd: FindWindow: id=0x%lX", (long)top);

  if (XGetClassHint(target_dpy, top, &hint)) {
    if (hint.res_name) {
      if (appres.debug) fprintf(stderr, " instance=\"%s\"", hint.res_name);
      if (fnmatch(name, hint.res_name, 0) == 0) w = top;
      XFree(hint.res_name);
    }
    if (hint.res_class) {
      if (appres.debug) fprintf(stderr, " class=\"%s\"", hint.res_class);
      if (fnmatch(name, hint.res_class, 0) == 0) w = top;
      XFree(hint.res_class);
    }
  }
  if (XFetchName(target_dpy, top, &win_name)) { /* window title */
    if (appres.debug) fprintf(stderr, " title=\"%s\"", win_name);
    if (fnmatch(name, win_name, 0) == 0) w = top;
    XFree(win_name);
  }

  if (appres.debug) fprintf(stderr, "%s\n", (w == None) ? "" : " [matched]");

  if (w == None &&
      XQueryTree(target_dpy, top, &dummy, &dummy, &children, &nchildren)) {
    for (i = 0; i < nchildren; i++) {
      w = FindWindow(children[i], name);
      if (w != None) break;
    }
    if (children) XFree((char *)children);
  }

  return(w);
}

/*
 * This will be called to get window to set input focus,
 * when user pressed the "Focus" button.
 */
static void GetFocusedWindow(void)
{
  Cursor cursor;
  XEvent event;
  Window target_root, child;
  int junk_i;
  unsigned junk_u;
  Window junk_w;
  int scrn;
  int cur_x, cur_y, last_x, last_y;
  double x_ratio, y_ratio;

  XFlush(target_dpy);
  target_root = RootWindow(target_dpy, DefaultScreen(target_dpy));

  cursor = XCreateFontCursor(dpy, (target_dpy == dpy) ? XC_crosshair : XC_dot);
  if (XGrabPointer(dpy, RootWindow(dpy, DefaultScreen(dpy)), False, ButtonPressMask,
                   GrabModeSync, GrabModeAsync, None,
                   cursor, CurrentTime) == 0) {
    if (appres.debug) fprintf(stderr, "xvkbd: GetFocusedWindow: Grab pointer - waiting for button press\n");
    last_x = -1;
    last_y = -1;
    x_ratio = ((double)WidthOfScreen(DefaultScreenOfDisplay(target_dpy))
	       / WidthOfScreen(XtScreen(toplevel)));
    y_ratio = ((double)HeightOfScreen(DefaultScreenOfDisplay(target_dpy))
	       / HeightOfScreen(XtScreen(toplevel)));
    do {
      XAllowEvents(dpy, SyncPointer, CurrentTime);
      if (target_dpy == dpy) {
	XNextEvent(dpy, &event);
      } else {
	XCheckTypedEvent(dpy, ButtonPress, &event);
	if (XQueryPointer(dpy, RootWindow(dpy, DefaultScreen(dpy)), &junk_w, &junk_w,
			  &cur_x, &cur_y, &junk_i, &junk_i, &junk_u)) {
	  cur_x = cur_x * x_ratio;
	  cur_y = cur_y * y_ratio;
	}
	if (cur_x != last_x || cur_y != last_y) {
	  if (appres.debug) fprintf(stderr, "xvkbd: Moving pointer to (%d, %d) on %s\n",
				    cur_x, cur_y, XDisplayString(target_dpy));
	  XWarpPointer(target_dpy, None, target_root, 0, 0, 0, 0, cur_x, cur_y);
	  XFlush(target_dpy);
	  last_x = cur_x;
	  last_y = cur_y;
	  XQueryPointer(target_dpy, target_root, &junk_w, &child,
			&cur_x, &cur_y, &junk_i, &junk_i, &junk_u);
	  usleep(10000);
	} else {
	  usleep(100000);
	}
      }
    } while (event.type != ButtonPress);
    XUngrabPointer(dpy, CurrentTime);

    focused_window = None;
    if (target_dpy == dpy) focused_window = event.xbutton.subwindow;
    if (focused_window == None) {
      XFlush(target_dpy);
      for (scrn = 0; scrn < ScreenCount(target_dpy); scrn++) {
	if (XQueryPointer(target_dpy, RootWindow(target_dpy, scrn), &junk_w, &child,
			  &junk_i, &junk_i, &junk_i, &junk_i, &junk_u)) {
	  if (appres.debug) fprintf(stderr, "xvkbd: Window on the other display/screen (screen #%d of %s) focused\n",
				    scrn, XDisplayString(target_dpy));
	  target_root = RootWindow(target_dpy, scrn);
	  focused_window = child;
	  break;
	}
      }
    }
    if (focused_window == None) focused_window = target_root;
    else focused_window = XmuClientWindow(target_dpy, focused_window);
    if (appres.debug) fprintf(stderr, "xvkbd: Selected window is: 0x%lX on %s\n",
			      focused_window, XDisplayString(target_dpy));

    if (target_dpy == dpy && XtWindow(toplevel) == focused_window) {
      focused_window = None;
      focused_subwindow = focused_window;
      return;
    }

    focused_subwindow = focused_window;
    do {  /* search the child window */
      XQueryPointer(target_dpy, focused_subwindow, &junk_w, &child,
                    &junk_i, &junk_i, &junk_i, &junk_i, &junk_u);
      if (child != None) {
        focused_subwindow = child;
        if (appres.debug) fprintf(stderr, "  going down: 0x%lX\n", focused_subwindow);
      }
    } while (child != None);
    if (appres.list_widgets || strlen(appres.widget) != 0) {
      child = FindWidget(toplevel, focused_window, appres.widget);
      if (child != None) focused_subwindow = child;
    }
  } else {
    fprintf(stderr, "%s: cannot grab pointer\n", PROGRAM_NAME);
  }
}

/*
 * Read keyboard mapping and modifier mapping.
 * Keyboard mapping is used to know what keys are in shifted position.
 * Modifier mapping is required because we should know Alt and Meta
 * key are used as which modifier.
 */
static void Highlight(char *name, int state);
static void AddModifier(KeySym keysym);
static void SendKeyPressedEvent(KeySym keysym, unsigned int shift, int press_release);

static Boolean need_read_keymap = TRUE;

static void MappingModified(Widget w, XMappingEvent *event,
			      String *pars, Cardinal *n_pars)
{
  if (appres.debug) fprintf(stderr, "xvkbd: MappingModified()\n");
  need_read_keymap = TRUE;

  if (event != NULL) XRefreshKeyboardMapping(event);
}

static void ReadKeymap(void)
{
  int i;
  int keycode, inx, pos;
  KeySym keysym;
  XModifierKeymap *modifiers;
  Widget w;
  int last_altgr_mask;
  int mode_switch_mask;

  if (appres.debug) fprintf(stderr, "xvkbd: ReadKeymap()\n");

  /* workaround for wrong keys caused after Shift key is pressed in German locale - xvkbd-3.8 */
  SendKeyPressedEvent(NoSymbol, !shift_state, 0);
  SendKeyPressedEvent(NoSymbol, shift_state, 0);

  XDisplayKeycodes(target_dpy, &min_keycode, &max_keycode);
  if (keysym_table != NULL) XFree(keysym_table);
  keysym_table = XGetKeyboardMapping(target_dpy,
                             min_keycode, max_keycode - min_keycode + 1,
                             &keysym_per_keycode);
  for (keycode = min_keycode; keycode <= max_keycode; keycode++) {
    /* if the first keysym is alphabet and the second keysym is NoSymbol,
       it is equivalent to pair of lowercase and uppercase alphabet */
    inx = (keycode - min_keycode) * keysym_per_keycode;
    if (keysym_table[inx + 1] == NoSymbol
	&& ((XK_A <= keysym_table[inx] && keysym_table[inx] <= XK_Z)
	    || (XK_a <= keysym_table[inx] && keysym_table[inx] <= XK_z))) {
      if (XK_A <= keysym_table[inx] && keysym_table[inx] <= XK_Z)
	keysym_table[inx] = keysym_table[inx] - XK_A + XK_a;
      keysym_table[inx + 1] = keysym_table[inx] - XK_a + XK_A;
    }
  }

  last_altgr_mask = altgr_mask;
  alt_mask = 0;
  meta_mask = 0;
  altgr_mask = 0;
  super_mask = 0;
  mode_switch_mask = 0;
  level3_shift_mask = 0;
  altgr_keysym = NoSymbol;
  modifiers = XGetModifierMapping(target_dpy);

  if (appres.debug) fprintf(stderr, "xvkbd: ReadKeymap: max_keypermod=%d\n", modifiers->max_keypermod);

  for (i = 0; i < 8; i++) {
    for (pos = 0; pos < modifiers->max_keypermod; pos++) {
      keycode = modifiers->modifiermap[i * modifiers->max_keypermod + pos];
      if (keycode < min_keycode || max_keycode < keycode) continue;

      keysym = keysym_table[(keycode - min_keycode) * keysym_per_keycode];
      if (alt_mask == 0 && (keysym == XK_Alt_L || keysym == XK_Alt_R)) {
	alt_mask = 1 << i;
	if (i != 3) fprintf(stderr, "%s: warning: Alt is assigned to modifier %d instead of %d\n",
			    PROGRAM_NAME, i - 2, 1);
      } else if (meta_mask == 0 && (keysym == XK_Meta_L || keysym == XK_Meta_R)) {
	meta_mask = 1 << i;
	if (i != 5) fprintf(stderr, "%s: warning: Meta is assigned to modifier %d instead of %d\n",
			    PROGRAM_NAME, i - 4, 3);
      } else if (super_mask == 0 && (keysym == XK_Super_L || keysym == XK_Super_R)) {
	super_mask = 1 << i;
	if (i != 6) fprintf(stderr, "%s: warning: Super is assigned to modifier %d instead of %d\n",
			    PROGRAM_NAME, i - 5, 4);
      } else if (mode_switch_mask == 0 && keysym == XK_Mode_switch) {
	mode_switch_mask = 1 << i;
      } else if (level3_shift_mask == 0 && keysym == XK_ISO_Level3_Shift) {
	level3_shift_mask = 1 << i;
      }
    }
  }

  if (appres.debug)
    fprintf(stderr, "xvkbd: alt_mask = 0x%x, meta_mask = 0x%x, super_mask = 0x%x, "
	    "altgr_mask = 0x%x, mode_switch_mask = 0x%x, level3_shift_mask = 0x%x\n",
	    alt_mask, meta_mask, super_mask, altgr_mask, mode_switch_mask, level3_shift_mask);

  if (level3_shift_mask == mode_switch_mask) {
    mode_switch_mask = 0x2000;
    if (appres.debug) {
      fprintf(stderr, "xvkbd: both ISO_Level3_Shift and Mode_switch found\n");
      fprintf(stderr, "xvkbd: assuming ISO_Level3_Shift=0x%x, Mode_switch=0x%x\n",
	      level3_shift_mask, mode_switch_mask);
    }
  }

  XFreeModifiermap(modifiers);

  if (mode_switch_mask != 0) {
    altgr_keysym = XK_Mode_switch;
    altgr_mask = mode_switch_mask;
  } else {
    fprintf(stderr, "%s: Mode_switch not available as a modifier\n", PROGRAM_NAME);
    if (level3_shift_mask == 0) {
      fprintf(stderr, "%s: AltGr can't be used\n", PROGRAM_NAME);
    } else {
      fprintf(stderr, "%s: although ISO_Level3_Shift is used instead, AltGr may not work correctly\n", PROGRAM_NAME);
      altgr_keysym = XK_ISO_Level3_Shift;
      altgr_mask = level3_shift_mask;
    }
  }

  w = XtNameToWidget(toplevel, "*Multi_key");
  if (w != None) {
    if (XKeysymToKeycode(target_dpy, XK_Multi_key) == NoSymbol) {
      if (!appres.auto_add_keysym || AddKeysym(XK_Multi_key, FALSE) == NoSymbol)
	XtSetSensitive(w, FALSE);
    }
  }
  w = XtNameToWidget(toplevel, "*Mode_switch");
  if (w != None) {
    if (appres.xtest && 0 < appres.altgr_keycode) {
      XtSetSensitive(w, TRUE);
      if (appres.debug)
	fprintf(stderr, "xvkbd: keycode %d will be used for AltGr - it was specified with altgrKeycode\n",
		appres.altgr_keycode);
    } else if (altgr_mask) {
      XtSetSensitive(w, TRUE);
    } else {
      XtSetSensitive(w, FALSE);
      if (shift_state & last_altgr_mask) {
	shift_state &= ~last_altgr_mask;
	Highlight("Mode_switch", FALSE);
      }
    }
  }

  if (appres.auto_add_keysym) {
    if (!altgr_mask) AddModifier(XK_Mode_switch);
  }
}

/*
 * This will called when X error is detected when attempting to
 * send a event to a client window;  this will normally caused
 * when the client window is destroyed.
 */
static int MyErrorHandler(Display *my_dpy, XErrorEvent *event)
{
  char msg[200];

  error_detected = TRUE;
  if (event->error_code == BadWindow) {
    if (appres.debug)
      fprintf(stderr, "xvkbd: BadWindow - couldn't find target window 0x%lX (destroyed?)\n",
	      (long)focused_window);
    return 0;
  }
  XGetErrorText(my_dpy, event->error_code, msg, sizeof(msg) - 1);
  fprintf(stderr, "xvkbd: X error trapped: %s, request-code=%d\n", msg, event->request_code);
  if (appres.debug) abort();
  return 0;
}

/*
 * Send event to the focused window.
 * If input focus is specified explicitly, select the window
 * before send event to the window.
 */
static void SendEvent(XKeyEvent *event)
{
  static Boolean first = TRUE;

  if (!appres.no_sync) {
    XSync(event->display, FALSE);
    XSetErrorHandler(MyErrorHandler);
  }

  error_detected = FALSE;
  if (focused_window != None) {
    /* set input focus if input focus is set explicitly */
    if (appres.debug)
      fprintf(stderr, "xvkbd: set input focus to window 0x%lX (0x%lX)\n",
              (long)focused_window, (long)event->window);
    XSetInputFocus(event->display, focused_window, RevertToParent, CurrentTime);
    if (!appres.no_sync) XSync(event->display, FALSE);
  }
  if (!error_detected) {
    if (appres.xtest) {
#ifdef USE_XTEST
      if (appres.debug)
	fprintf(stderr, "xvkbd: XTestFakeKeyEvent(0x%lx, %ld, %d)\n",
		(long)event->display, (long)event->keycode, event->type == KeyPress);
      if (appres.jump_pointer) {
	Window root, child, w;
	int root_x, root_y, x, y;
	unsigned int mask;
	int revert_to;

	w = None;
	if (first || strlen(appres.text) == 0 || appres.jump_pointer_back) {
	  first = FALSE;

	  w = focused_subwindow;
	  if (w == None && appres.jump_pointer_always)
	    XGetInputFocus(event->display, &w, &revert_to);

	  if (w != None) {
	    if (appres.debug)
	      fprintf(stderr, "xvkbd: SendEvent: jump pointer to window 0x%lx\n", (long)w);

	    XQueryPointer(event->display, w,
			  &root, &child, &root_x, &root_y, &x, &y, &mask);
	    XWarpPointer(event->display, None, w, 0, 0, 0, 0, 1, 1);
	    XFlush(event->display);
	  }
	}

	XTestFakeKeyEvent(event->display, event->keycode, event->type == KeyPress, 0);
	XFlush(event->display);

	if (w != None && appres.jump_pointer_back) {
	  XWarpPointer(event->display, None, root, 0, 0, 0, 0, root_x, root_y);
	  XFlush(event->display);
	}
      } else {
	XTestFakeKeyEvent(event->display, event->keycode, event->type == KeyPress, 0);
 	XFlush(event->display);
      }
#else
      fprintf(stderr, "%s: this binary is compiled without XTEST support\n",
	      PROGRAM_NAME);
#endif
    } else {
      XSendEvent(event->display, event->window, TRUE, KeyPressMask, (XEvent *)event);
      if (!appres.no_sync) XSync(event->display, FALSE);

      if (error_detected
	  && (focused_subwindow != None) && (focused_subwindow != event->window)) {
	error_detected = FALSE;
	event->window = focused_subwindow;
	if (appres.debug)
	  fprintf(stderr, "   retry: send event to window 0x%lX (0x%lX)\n",
		  (long)focused_window, (long)event->window);
	XSendEvent(event->display, event->window, TRUE, KeyPressMask, (XEvent *)event);
	if (!appres.no_sync) XSync(event->display, FALSE);
      }
    }
  }

  if (error_detected) {
    /* reset focus because focused window is (probably) no longer exist */
    XBell(dpy, 0);
    focused_window = None;
    focused_subwindow = None;
  }

  XSetErrorHandler(NULL);
}

/*
 * Insert a specified keysym to unused position in the keymap table.
 * This will be called to add required keysyms on-the-fly.
 * if the second parameter is TRUE, the keysym will be added to the
 * non-shifted position - this may be required for modifier keys
 * (e.g. Mode_switch) and some special keys (e.g. F20).
 */
static int AddKeysym(KeySym keysym, Boolean top)
{
  int keycode, pos, max_pos, inx, phase;

  if (appres.debug) fprintf(stderr, "xvkbd: AddKeySym(%lx)\n", (long)keysym);

  if (top) {
    max_pos = 0;
  } else {
    max_pos = keysym_per_keycode - 1;
    if (4 <= max_pos) max_pos = 3;
  }

  for (phase = 0; phase < 2; phase++) {
    for (keycode = max_keycode; min_keycode <= keycode; keycode--) {
      for (pos = max_pos; 0 <= pos; pos--) {
	inx = (keycode - min_keycode) * keysym_per_keycode;
	if ((phase != 0 || keysym_table[inx] == NoSymbol)
	    && (keysym_table[inx] < 0xFF00
		|| (0x10000 <= keysym_table[inx] && keysym_table[inx] < 0x1008f000))) {
	  /* In the first phase, to avoid modifing existing keys, */
	  /* add the keysym only to the keys which has no keysym in the first position. */
	  /* If no place found in the first phase, add the keysym for any keys except */
	  /* for modifier keys and other special keys */
	  if (keysym_table[inx + pos] == NoSymbol) {
	    if (appres.debug)
	      fprintf(stderr, "xvkbd: Adding keysym \"%s\" at keycode %d position %d/%d\n",
		      XKeysymToString(keysym), keycode, pos, keysym_per_keycode);
	    keysym_table[inx + pos] = keysym;
	    XChangeKeyboardMapping(target_dpy, keycode, keysym_per_keycode, &keysym_table[inx], 1);
	    XFlush(target_dpy);
	    return keycode;
	  }
	}
      }
    }
  }
  fprintf(stderr, "%s: couldn't add \"%s\" to keymap\n",
	  PROGRAM_NAME, XKeysymToString(keysym));
  XBell(dpy, 0);
  return NoSymbol;
}

/*
 * Add the specified key as a new modifier.
 * This is used to use Mode_switch (AltGr) as a modifier.
 */
static void AddModifier(KeySym keysym)
{
  XModifierKeymap *modifiers;
  int keycode, i, pos;

  keycode = XKeysymToKeycode(target_dpy, keysym);
  if (keycode == NoSymbol) keycode = AddKeysym(keysym, TRUE);

  modifiers = XGetModifierMapping(target_dpy);
  for (i = 7; 3 < i; i--) {
    if (modifiers->modifiermap[i * modifiers->max_keypermod] == NoSymbol
	|| ((keysym_table[(modifiers->modifiermap[i * modifiers->max_keypermod]
			   - min_keycode) * keysym_per_keycode]) == XK_ISO_Level3_Shift
	    && keysym == XK_Mode_switch)) {
      for (pos = 0; pos < modifiers->max_keypermod; pos++) {
	if (modifiers->modifiermap[i * modifiers->max_keypermod + pos] == NoSymbol) {
	  if (appres.debug)
	    fprintf(stderr, "xvkbd: Adding modifier \"%s\" as %dth modifier\n",
		    XKeysymToString(keysym), i);
	  modifiers->modifiermap[i * modifiers->max_keypermod + pos] = keycode;
	  XSetModifierMapping(target_dpy, modifiers);
	  return;
	}
      }
    }
  }
  fprintf(stderr, "%s: couldn't add \"%s\" as modifier\n",
	  PROGRAM_NAME, XKeysymToString(keysym));
  XBell(dpy, 0);
}

/*
 * Send sequence of KeyPressed/KeyReleased events to the focused
 * window to simulate keyboard.  If modifiers (shift, control, etc)
 * are set ON, many events will be sent.
 */
#define SENDKEY_KEY_PRESS    1
#define SENDKEY_KEY_RELEASE  2

static void SendKeyPressedEvent(KeySym keysym, unsigned int shift, int press_release)
{
  Window cur_focus;
  int revert_to;
  XKeyEvent event;
  int keycode;
  Window root, *children;
  unsigned int n_children;
  int phase, inx;
  Boolean found;
  Boolean last_caps_lock = FALSE;

  if (need_read_keymap) {
    need_read_keymap = FALSE;
    ReadKeymap();
  }

  if (focused_subwindow != None)
    cur_focus = focused_subwindow;
  else
    XGetInputFocus(target_dpy, &cur_focus, &revert_to);

  if (appres.debug) {
    char ch = '?';
    if ((keysym & ~0x7f) == 0 && isprint(keysym)) ch = keysym;
    fprintf(stderr, "xvkbd: SendKeyPressedEvent: focus=0x%lX, key=0x%lX (%c), shift=0x%lX\n",
            (long)cur_focus, (long)keysym, ch, (long)shift);
  }

  if (XtWindow(toplevel) != None) {
    if (toplevel_parent == None) {
      XQueryTree(target_dpy, RootWindow(target_dpy, DefaultScreen(target_dpy)),
                 &root, &toplevel_parent, &children, &n_children);
      XFree(children);
    }
    if (cur_focus == None || cur_focus == PointerRoot
	|| cur_focus == XtWindow(toplevel) || cur_focus == toplevel_parent) {
      /* notice user when no window focused or the xvkbd window is focused */
      XBell(dpy, 0);
      return;
    }
  }

  found = FALSE;
  keycode = 0;
  if (keysym != NoSymbol) {
    for (phase = 0; phase < 2; phase++) {
      for (keycode = min_keycode; !found && (keycode <= max_keycode); keycode++) {
	/* Determine keycode for the keysym:  we use this instead
	   of XKeysymToKeycode() because we must know shift_state, too */
	/* 1: Shift, 2: AltGr, 3: Shift+AltGr, 4: Level3, 5: Shift+Level3 */
	inx = (keycode - min_keycode) * keysym_per_keycode;
	if (keysym_table[inx] == keysym) {
	  shift &= ~altgr_mask;
	  if (keysym_table[inx + 1] != NoSymbol) shift &= ~ShiftMask;
	  found = TRUE;
	  break;
	} else if (keysym_table[inx + 1] == keysym) {
	  shift &= ~altgr_mask;
	  shift |= ShiftMask;
	  found = TRUE;
	  break;
	}
      }
      if (!found && altgr_mask && 3 <= keysym_per_keycode) {
	for (keycode = min_keycode; !found && (keycode <= max_keycode); keycode++) {
	  inx = (keycode - min_keycode) * keysym_per_keycode;
	  if (keysym_table[inx + 2] == keysym) {
	    shift &= ~ShiftMask;
	    shift |= altgr_mask;
	    found = TRUE;
	    break;
	  } else if (4 <= keysym_per_keycode && keysym_table[inx + 3] == keysym) {
	    shift |= ShiftMask | altgr_mask;
	    found = TRUE;
	    break;
	  } else if (5 <= keysym_per_keycode && keysym_table[inx + 4] == keysym) {
	    shift &= ~(ShiftMask | altgr_mask);
	    shift |= level3_shift_mask;
	    found = TRUE;
	    break;
	  } else if (6 <= keysym_per_keycode && keysym_table[inx + 5] == keysym) {
	    shift &= ~altgr_mask;
	    shift |= ShiftMask | level3_shift_mask;
	    found = TRUE;
	    break;
	  }
	}
      }
      if (found || !appres.auto_add_keysym) break;

      if (0xF000 <= keysym) {
	/* for special keys such as function keys,
	   first try to add it in the non-shifted position of the keymap */
	if (AddKeysym(keysym, TRUE) == NoSymbol) AddKeysym(keysym, FALSE);
      } else {
	AddKeysym(keysym, FALSE);
      }
    }
    if (appres.debug) {
      if (found) {
	fprintf(stderr, "xvkbd: SendKeyPressedEvent: keysym=0x%lx, keycode=%ld, shift=0x%lX\n",
		(long)keysym, (long)keycode, (long)shift);
	fprintf(stderr, "keysym table: keycode %d = ", keycode);
      	for (inx = (keycode - min_keycode) * keysym_per_keycode;
	     inx < (keycode - min_keycode + 1) * keysym_per_keycode;
	     inx++)
	  fprintf(stderr, " 0x%lx (%s)",
		  (long)keysym_table[inx], keysym_table[inx] ? XKeysymToString(keysym_table[inx]) : "null");
	fprintf(stderr, "\n");
      } else
	fprintf(stderr, "SendKeyPressedEvent: keysym=0x%lx - keycode not found\n",
		(long)keysym);
    }
  }

  event.display = target_dpy;
  event.window = cur_focus;
  event.root = RootWindow(event.display, DefaultScreen(event.display));
  event.subwindow = None;
  event.time = CurrentTime;
  event.x = 1;
  event.y = 1;
  event.x_root = 1;
  event.y_root = 1;
  event.same_screen = TRUE;

#ifdef USE_XTEST
  if (appres.xtest && press_release == 0) {
    Window root, child;
    int root_x, root_y, x, y;
    unsigned int mask;

    XQueryPointer(target_dpy, event.root, &root, &child, &root_x, &root_y, &x, &y, &mask);

    event.type = KeyRelease;
    event.state = 0;
    if (mask & ControlMask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Control_L);
      SendEvent(&event);
    }
    if (mask & alt_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Alt_L);
      SendEvent(&event);
    }
    if (mask & meta_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Meta_L);
      SendEvent(&event);
    }
    if (mask & super_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Super_L);
      SendEvent(&event);
    }
    if (mask & altgr_mask) {
      if (0 < appres.altgr_keycode)
	event.keycode = appres.altgr_keycode;
      else
	event.keycode = XKeysymToKeycode(target_dpy, altgr_keysym);
      SendEvent(&event);
    }
    if (mask & level3_shift_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_ISO_Level3_Shift);
      SendEvent(&event);
    }
    if (mask & ShiftMask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Shift_L);
      SendEvent(&event);
    }
    if (mask & LockMask) {
      last_caps_lock = TRUE;
      event.type = KeyPress;
      event.keycode = XKeysymToKeycode(target_dpy, XK_Caps_Lock);
      SendEvent(&event);
      event.type = KeyRelease;
      event.keycode = XKeysymToKeycode(target_dpy, XK_Caps_Lock);
      SendEvent(&event);
    }
  }
#endif

  event.type = KeyPress;
  event.state = 0;
  if (shift & ControlMask) {
    if (appres.debug) fprintf(stderr, "[Control] ");
    event.keycode = XKeysymToKeycode(target_dpy, XK_Control_L);
    SendEvent(&event);
    event.state |= ControlMask;
  }
  if (shift & alt_mask) {
    if (appres.debug) fprintf(stderr, "[Alt] ");
    event.keycode = XKeysymToKeycode(target_dpy, XK_Alt_L);
    SendEvent(&event);
    event.state |= alt_mask;
  }
  if (shift & meta_mask) {
    if (appres.debug) fprintf(stderr, "[Meta] ");
    event.keycode = XKeysymToKeycode(target_dpy, XK_Meta_L);
    SendEvent(&event);
    event.state |= meta_mask;
  }
  if (shift & super_mask) {
    if (appres.debug) fprintf(stderr, "[Super] ");
    event.keycode = XKeysymToKeycode(target_dpy, XK_Super_L);
    SendEvent(&event);
    event.state |= super_mask;
  }
  if (shift & altgr_mask) {
    if (appres.debug) fprintf(stderr, "[AltGr] ");
    if (0 < appres.altgr_keycode)
      event.keycode = appres.altgr_keycode;
    else
      event.keycode = XKeysymToKeycode(target_dpy, altgr_keysym);
    SendEvent(&event);
    event.state |= altgr_mask;
  }
  if (shift & level3_shift_mask) {
    if (appres.debug) fprintf(stderr, "[Level3] ");
    event.keycode = XKeysymToKeycode(target_dpy, XK_ISO_Level3_Shift);
    SendEvent(&event);
    event.state |= level3_shift_mask;
  }
  if (shift & ShiftMask) {
    if (appres.debug) fprintf(stderr, "[Shift] ");
    event.keycode = XKeysymToKeycode(target_dpy, XK_Shift_L);
    SendEvent(&event);
    event.state |= ShiftMask;
  }

  if (keysym != NoSymbol) {  /* send event for the key itself */
    event.keycode = found ? keycode : XKeysymToKeycode(target_dpy, keysym);


    if (event.keycode == NoSymbol) {
      if ((keysym & ~0x7f) == 0 && isprint(keysym))
        fprintf(stderr, "%s: no such key: %c\n",
                PROGRAM_NAME, (char)keysym);
      else if (XKeysymToString(keysym) != NULL)
        fprintf(stderr, "%s: no such key: keysym=%s (0x%lX)\n",
                PROGRAM_NAME, XKeysymToString(keysym), (long)keysym);
      else
        fprintf(stderr, "%s: no such key: keysym=0x%lX\n",
                PROGRAM_NAME, (long)keysym);
      XBell(dpy, 0);
    } else if (press_release == 0) {
      SendEvent(&event);
      event.type = KeyRelease;
      SendEvent(&event);
    } else {
      if (press_release & SENDKEY_KEY_PRESS) SendEvent(&event);
      event.type = KeyRelease;
      if (press_release & SENDKEY_KEY_RELEASE) SendEvent(&event);
    }
  }

  if (last_caps_lock) {
    /* restore last Caps_Lock state */
    event.type = KeyPress;
    event.keycode = XKeysymToKeycode(target_dpy, XK_Caps_Lock);
    SendEvent(&event);
    event.type = KeyRelease;
    event.keycode = XKeysymToKeycode(target_dpy, XK_Caps_Lock);
    SendEvent(&event);
  }
  event.type = KeyRelease;
  if (shift & ShiftMask) {
    event.keycode = XKeysymToKeycode(target_dpy, XK_Shift_L);
    SendEvent(&event);
    event.state &= ~ShiftMask;
  }
  if (press_release == 0) {
    if (shift & altgr_mask) {
      if (0 < appres.altgr_keycode)
	event.keycode = appres.altgr_keycode;
      else
	event.keycode = XKeysymToKeycode(target_dpy, altgr_keysym);
      SendEvent(&event);
      event.state &= ~altgr_mask;
    }
    if (shift & level3_shift_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_ISO_Level3_Shift);
      SendEvent(&event);
      event.state &= ~level3_shift_mask;
    }
    if (shift & meta_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Meta_L);
      SendEvent(&event);
      event.state &= ~meta_mask;
    }
    if (shift & super_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Super_L);
      SendEvent(&event);
      event.state &= ~super_mask;
    }
    if (shift & alt_mask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Alt_L);
      SendEvent(&event);
      event.state &= ~alt_mask;
    }
    if (shift & ControlMask) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Control_L);
      SendEvent(&event);
      event.state &= ~ControlMask;
    }
  }

  if (appres.no_sync) XFlush(dpy);
}

static Boolean need_insert_blank = FALSE;




/*
 * Send given string to the focused window as if the string
 * is typed from a keyboard.
 */
static void KeyPressed(Widget w, char *key, char *data);

static void SendString(const char *str)
{
  const char *cp, *cp2;
  char key[50];
  int len;
  int val;
  Window target_root, child, junk_w;
  int junk_i;
  unsigned junk_u;
  int cur_x, cur_y;

  if (appres.debug) fprintf(stderr, "xvkbd: SendString(%s)\n", str);

  if (need_read_keymap) {
    need_read_keymap = FALSE;
    ReadKeymap();
  }

  shift_state = 0;
  for (cp = str; *cp != '\0'; cp++) {
    if (0 < appres.text_delay) usleep(appres.text_delay * 1000);
    if (*cp == '\\') {
      cp++;
      switch (*cp) {
      case '\0':
        fprintf(stderr, "%s: missing character after \"\\\"\n",
                PROGRAM_NAME);
        return;
      case '[':  /* we can write any keysym as "\[keysym]" here */
        cp2 = strchr(cp, ']');
        
        if (cp2 == NULL) {
          fprintf(stderr, "%s: no closing \"]\" after \"\\[\"\n",
                  PROGRAM_NAME);
        } else {
          len = cp2 - cp - 1;
          if (sizeof(key) <= len) len = sizeof(key) - 1;
          strncpy(key, cp + 1, len);
          key[len] = '\0';
          KeyPressed(None, key, NULL);
          cp = cp2;
        }
        
        //cp=cp2;
        break;
      case '{':		/*  "\{keysym}" will send the keysym more directly, and
						"\{+keysym}" and "\{-keysym}" will press the release the key */
        cp2 = strchr(cp, '}');
        if (cp2 == NULL) {
          fprintf(stderr, "%s: no closing \"}\" after \"\\{\"\n",
                  PROGRAM_NAME);
        } else {
	  int press_release = SENDKEY_KEY_PRESS | SENDKEY_KEY_RELEASE;
	  KeySym keysym;
	  if (*(cp + 1) == '+') {
	    press_release = SENDKEY_KEY_PRESS;
	    cp++;
	  } else if (*(cp + 1) == '-') {
	    press_release = SENDKEY_KEY_RELEASE;
	    cp++;
	  }
          len = cp2 - cp - 1;
          if (sizeof(key) <= len) len = sizeof(key) - 1;
          strncpy(key, cp + 1, len);
          key[len] = '\0';
	  keysym = XStringToKeysym(key);
	  if (keysym == NoSymbol) fprintf(stderr, "%s: no such keysym: %s\n",
					  PROGRAM_NAME, key);
	  SendKeyPressedEvent(keysym, 0, press_release);
          cp = cp2;
        }
	break;
      case 'S': shift_state |= ShiftMask; break;
      case 'C': shift_state |= ControlMask; break;
      case 'A': shift_state |= alt_mask; break;
      case 'M': shift_state |= meta_mask; break;
      case 'W': shift_state |= super_mask; break;
      case 'b': SendKeyPressedEvent(XK_BackSpace, shift_state, 0); shift_state = 0; break;
      case 't': SendKeyPressedEvent(XK_Tab, shift_state, 0); shift_state = 0; break;
      case 'n': SendKeyPressedEvent(XK_Linefeed, shift_state, 0); shift_state = 0; break;
      case 'r': SendKeyPressedEvent(XK_Return, shift_state, 0); shift_state = 0; break;
      case 'e': SendKeyPressedEvent(XK_Escape, shift_state, 0); shift_state = 0; break;
      case 'd': SendKeyPressedEvent(XK_Delete, shift_state, 0); shift_state = 0; break;
      case 'D':  /* delay */
	cp++;
	if ('1' <= *cp && *cp <= '9') {
	  usleep((*cp - '0') * 100000);
	} else {
          fprintf(stderr, "%s: no digit after \"\\m\"\n",
                  PROGRAM_NAME);
	}
	break;
      case 'm':  /* simulate click mouse button */
	cp++;
	if ('1' <= *cp && *cp <= '9') {
	  if (appres.debug) fprintf(stderr, "xvkbd: XTestFakeButtonEvent(%d)\n", *cp - '0');
	  XTestFakeButtonEvent(target_dpy, *cp - '0', True, CurrentTime);
	  XTestFakeButtonEvent(target_dpy, *cp - '0', False, CurrentTime);
	  XFlush(dpy);
	} else {
          fprintf(stderr, "%s: no digit after \"\\m\"\n",
                  PROGRAM_NAME);
	}
	break;
      case 'x':
      case 'y':  /* move mouse pointer */
	sscanf(cp + 1, "%d", &val);
	target_root = RootWindow(target_dpy, DefaultScreen(target_dpy));
	XQueryPointer(target_dpy, target_root, &junk_w, &child,
		      &cur_x, &cur_y, &junk_i, &junk_i, &junk_u);
	if (*cp == 'x') {
	  if (isdigit(*(cp + 1))) cur_x = val;
	  else cur_x += val;
	} else {
	  if (isdigit(*(cp + 1))) cur_y = val;
	  else cur_y += val;
	}
	XWarpPointer(target_dpy, None, target_root, 0, 0, 0, 0, cur_x, cur_y);
	XFlush(dpy);
	cp++;
	while (isdigit(*(cp + 1)) || *(cp + 1) == '+' || *(cp + 1) == '-') cp++;
        break;
      default:
	SendKeyPressedEvent(*cp, shift_state, 0);
	shift_state = 0;
	break;
      }
    } else {
      SendKeyPressedEvent(*cp, shift_state, 0);
      shift_state = 0;
    }
  }
}

/*
 * Highlight/unhighligh spcified modifier key on the screen.
 */
static void Highlight(char *name, int state)
{
  char name1[50];
  Widget w;

  snprintf(name1, sizeof(name1), "*%s", name);
  w = XtNameToWidget(toplevel, name1);
  if (w != None) {
    if (strstr(name, "Focus") != NULL) {
      if (target_dpy == dpy)
        XtVaSetValues(w, XtNbackground, appres.focus_background, NULL);
      else
        XtVaSetValues(w, XtNbackground, appres.remote_focus_background, NULL);
      if (state)
        XtVaSetValues(w, XtNforeground, appres.highlight_foreground, NULL);
      else
        XtVaSetValues(w, XtNforeground, appres.special_foreground, NULL);
    } else {
      if (state)
        XtVaSetValues(w, XtNbackground, appres.highlight_background,
                      XtNforeground, appres.highlight_foreground, NULL);
      else
        XtVaSetValues(w, XtNbackground, appres.special_background,
                      XtNforeground, appres.special_foreground, NULL);
    }
  }
}

/*
 * Highlight/unhighligh keys on the screen to reflect the state.
 */
static Boolean CheckShiftState(int row, int col, int shift)
{
  Boolean shifted;

  shifted = (shift & ShiftMask);
  if (shift & LockMask) {
    if (shift & altgr_mask) {
      if (keys_shift_altgr[row][col] == NULL) return FALSE;
      if (strcasecmp(keys_altgr[row][col], keys_shift_altgr[row][col]) == 0) return !shifted;
    } else {
      if (keys_shift[row][col] == NULL) return FALSE;
      if (strcasecmp(keys_normal[row][col], keys_shift[row][col]) == 0) return !shifted;
    }
  }
  return shifted;
}

static void RefreshShiftState(Boolean force)
{
  static Boolean first = TRUE;
  static int last_shift_state = 0;
  static int last_mouse_shift = 0;
  static int last_num_lock_state = FALSE;
  static Display *last_target_dpy = NULL;
  static long last_focus = 0;
  int cur_shift;
  int changed;
  int first_row, row, col;
  Boolean shifted;
  char *label;
  int mask;

  cur_shift = shift_state | mouse_shift;
  changed = cur_shift ^ (last_shift_state | last_mouse_shift);
  if (first || force) changed = 0xffff;

  if (changed & ShiftMask) {
    Highlight("Shift_L", cur_shift & ShiftMask);
    Highlight("Shift_R", cur_shift & ShiftMask);
  }
  if (changed & ControlMask) {
    Highlight("Control_L", cur_shift & ControlMask);
    Highlight("Control_R", cur_shift & ControlMask);
  }
  if (changed & alt_mask) {
    Highlight("Alt_L", cur_shift & alt_mask);
    Highlight("Alt_R", cur_shift & alt_mask);
  }
  if (changed & meta_mask) {
    Highlight("Meta_L", cur_shift & meta_mask);
    Highlight("Meta_R", cur_shift & meta_mask);
  }
  if (changed & super_mask) {
    Highlight("Super_L", cur_shift & super_mask);
    Highlight("Super_R", cur_shift & super_mask);
  }
  if (changed & LockMask) {
    Highlight("Caps_Lock", cur_shift & LockMask);
  }
  if (changed & altgr_mask) {
    Highlight("Mode_switch", cur_shift & altgr_mask);
  }
  if (last_num_lock_state != appres.num_lock_state) {
    Highlight("Num_Lock", appres.num_lock_state);
    Highlight("keypad_panel*Num_Lock", appres.num_lock_state);
  }
  if (last_target_dpy != target_dpy || last_focus != focused_window) {
    Highlight("Focus", focused_window != 0);
    Highlight("keypad*Focus", focused_window != 0);
    Highlight("keypad_panel*Focus", focused_window != 0);
    last_target_dpy = target_dpy;
    last_focus = focused_window;
  }

  mask = ShiftMask | LockMask | altgr_mask;
  changed = (shift_state & mask) ^ (last_shift_state & mask);
  if (first || force) changed = TRUE;
  if (changed && !appres.keypad_only
      && (appres.modal_keytop || toplevel_height < appres.modal_threshold)) {
    first_row = appres.function_key ? 0 : 1;
    for (row = first_row; row < NUM_KEY_ROWS; row++) {
      for (col = 0; col < NUM_KEY_COLS; col++) {
	shifted = CheckShiftState(row, col, cur_shift);
	if (key_widgets[row][col] != None) {
	  if ((shift_state & altgr_mask) && altgr_key_labels[row][col] != NULL) {
	    if (shifted && shift_altgr_key_labels[row][col] != NULL)
	      label = shift_altgr_key_labels[row][col];
	    else
	      label = altgr_key_labels[row][col];
	  } else {
	    if (shifted && shift_key_labels[row][col] != NULL)
	      label = shift_key_labels[row][col];
	    else
	      label = normal_key_labels[row][col];
	  }
	  if (label == NULL) {
	    fprintf(stderr, "%s: no label for key %d,%d\n", PROGRAM_NAME, row, col);
	    label = "";
	  }
	  if (strcmp(label, "space") == 0) label = "";
	  XtVaSetValues(key_widgets[row][col], XtNlabel, label, NULL);
	}
      }
    }
  }

  last_shift_state = shift_state;
  last_mouse_shift = mouse_shift;
  last_num_lock_state = appres.num_lock_state;
  first = FALSE;

#ifdef USE_XTEST
  if (appres.xtest && strlen(appres.positive_modifiers) != 0) {
    /* modifiers specified in positiveModifiers resouce will be hold down
       so that it can be used with, for example, mouse operations */

    Window root, child;
    int root_x, root_y, x, y;
    unsigned int mask;

    XKeyEvent event;

    event.display = target_dpy;
    event.window = RootWindow(event.display, DefaultScreen(event.display));
    event.root = event.window;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = TRUE;
    event.state = 0;

    XQueryPointer(target_dpy, event.root, &root, &child, &root_x, &root_y, &x, &y, &mask);

    if (strstr(appres.positive_modifiers, "shift") != NULL
	&& (shift_state & ShiftMask) != (mask & ShiftMask)) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Shift_L);
      event.type = (shift_state & ShiftMask) ? KeyPress : KeyRelease;
      SendEvent(&event);
    }
    if (strstr(appres.positive_modifiers, "control") != NULL
	&& (shift_state & ControlMask) != (mask & ControlMask)) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Control_L);
      event.type = (shift_state & ControlMask) ? KeyPress : KeyRelease;
      SendEvent(&event);
    }
    if (strstr(appres.positive_modifiers, "alt") != NULL
	&& (shift_state & alt_mask) != (mask & alt_mask)) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Alt_L);
      event.type = (shift_state & alt_mask) ? KeyPress : KeyRelease;
      SendEvent(&event);
    }
    if (strstr(appres.positive_modifiers, "meta") != NULL
	&& (shift_state & meta_mask) != (mask & meta_mask)) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Meta_L);
      event.type = (shift_state & meta_mask) ? KeyPress : KeyRelease;
      SendEvent(&event);
    }
    if (strstr(appres.positive_modifiers, "super") != NULL
	&& (shift_state & super_mask) != (mask & super_mask)) {
      event.keycode = XKeysymToKeycode(target_dpy, XK_Super_L);
      event.type = (shift_state & super_mask) ? KeyPress : KeyRelease;
      SendEvent(&event);
    }
  }
#endif
}


static unsigned int n_key_repeat;

static char *keyboard_layout = NULL;


static Boolean props_panel_active = FALSE;

/*
 * Callback for main menu (activated from "xvkbd" logo).
 */

#define DISPLAY_NAME_LENGTH 50

/*
 * This will be called when user pressed a key on the screen.
 */
static const char *FindFunctionKeyValue(Widget w, const char *key, Boolean shiftable);
//static void ShowBalloon(Widget w, XEvent *event, String *pars, Cardinal *n_pars);
static void KeyClick(void);
static void StopAutoclick(void);


static void KeyPressed(Widget w, char *key, char *data)
{
  int row, col;
  int cur_shift;
  char *key1 = NULL;
  KeySym keysym;
  Boolean shifted;
  const char *value;
  Boolean found;

  if (appres.debug) fprintf(stderr, "xvkbd: KeyPressed: key=%s, widget=%lx\n", key, (long)w);

  if (need_read_keymap) {
    need_read_keymap = FALSE;
    ReadKeymap();
  }

  value = FindFunctionKeyValue(w, key, TRUE);
  if (value != NULL) {
    if (appres.debug) fprintf(stderr, "  Assigned string: %s\n", value);
    if (value[0] == '!') {
      if (appres.debug) fprintf(stderr, "  Launching: %s\n", value + 1);
      if (!appres.secure) system(value + 1);
    } else {
      if (value[0] == '\\') value = value + 1;
      if (appres.debug) fprintf(stderr, "  Sending: %s\n", value);
      SendString(value);
    }
    //ShowBalloon(w, NULL, NULL, NULL);
    return;
  }

  if (strncmp(key, "Shift", strlen("Shift")) == 0) {
    if (shift_state & ShiftMask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= ShiftMask;
  } else if (strncmp(key, "Control", strlen("Control")) == 0) {
    if (shift_state & ControlMask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= ControlMask;
  } else if (alt_mask != 0 && strncmp(key, "Alt", strlen("Alt")) == 0) {
    if (shift_state & alt_mask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= alt_mask;
  } else if (meta_mask != 0 && strncmp(key, "Meta", strlen("Meta")) == 0) {
    if (shift_state & meta_mask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= meta_mask;
  } else if (super_mask != 0 && strncmp(key, "Super", strlen("Super")) == 0) {
    if (shift_state & super_mask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= super_mask;
  } else if (strcmp(key, "Mode_switch") == 0) {
    if (shift_state & altgr_mask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= altgr_mask;
  } else if (strcmp(key, "Caps_Lock") == 0) {
    if (shift_state & LockMask) SendKeyPressedEvent(NoSymbol, shift_state, 0);
    shift_state ^= LockMask;
  } else if (strcmp(key, "Num_Lock") == 0) {
    appres.num_lock_state = !appres.num_lock_state;
  } else if (strcmp(key, "Focus") == 0) {
    cur_shift = shift_state | mouse_shift;
    if (cur_shift & ShiftMask) {
      focused_window = None;
      focused_subwindow = None;
    } else {
      GetFocusedWindow();
    }
  } else {
    if (appres.quick_modifiers && mouse_shift == 0 && w != None) {
      Window junk_w;
      int junk_i;
      unsigned junk_u;
      int cur_x, cur_y;
      Dimension btn_wd, btn_ht;

      n_key_repeat = n_key_repeat + 1;
      if (n_key_repeat == 1) return;

      XtVaGetValues(w, XtNwidth, &btn_wd, XtNheight, &btn_ht, NULL);
      XQueryPointer(dpy, XtWindow(w), &junk_w, &junk_w,
		    &junk_i, &junk_i, &cur_x, &cur_y, &junk_u);

      mouse_shift = 0;
      if (cur_x < 0 && btn_ht < cur_y) {
	mouse_shift |= alt_mask;  // left-down
      } else {
	if (cur_y < 0) mouse_shift |= ShiftMask;  // up
	else if (btn_ht < cur_y) mouse_shift |= meta_mask;  // down
	if (cur_x < 0) mouse_shift |= ControlMask;  // left
	else if (btn_wd < cur_x) mouse_shift |= altgr_mask;  // right
      }
    }
    cur_shift = shift_state | mouse_shift;
    shifted = (shift_state & ShiftMask);
    key1 = key;
    if (w != None) {
      if (sscanf(key, "pad%d,%d", &row, &col) == 2) {
	key1 = appres.num_lock_state ? keypad_shift[row][col]: keypad[row][col];
      } else {
	found = FALSE;
	if (sscanf(key, "%d,%d", &row, &col) == 2) {
	  found = TRUE;
	} else if (w != None) {
	  int first_row = appres.function_key ? 0 : 1;
	  for (row = first_row; row < NUM_KEY_ROWS; row++) {
	    for (col = 0; col < NUM_KEY_COLS; col++) {
	      if (key_widgets[row][col] == w) {
		found = TRUE;
		break;
	      }
	    }
	    if (col < NUM_KEY_COLS) break;
	  }
	}
	if (found) {
	  shifted = CheckShiftState(row, col, cur_shift);
	  if ((cur_shift & altgr_mask) && keys_altgr[row][col] != NULL) {
	    if (shifted && keys_shift_altgr[row][col] != NULL) {
	      key1 = keys_shift_altgr[row][col];
	      if (strcmp(keys_altgr[row][col], keys_shift_altgr[row][col]) != 0)
		cur_shift &= ~ShiftMask;
	    } else {
	      key1 = keys_altgr[row][col];
	    }
	  } else {
	    if (shifted && keys_shift[row][col] != NULL) {
	      key1 = keys_shift[row][col];
	      if (strcmp(keys_normal[row][col], keys_shift[row][col]) != 0)
		cur_shift &= ~ShiftMask;
	    } else {
	      key1 = keys_normal[row][col];
	    }
	  }
	}  // if (found) ...
      }  // if (sscanf(key, "pad%d,%d", ...
    }  // if (w != None) ...

    if (appres.debug) fprintf(stderr, "xvkbd: KeyPressed: key=%s, key1=%s\n", key, key1);

    if (strlen(key1) == 1) {
      if (need_insert_blank && !ispunct(*key1) && !isspace(*key1)) SendKeyPressedEvent(' ', 0, 0);
      SendKeyPressedEvent((KeySym)*key1 & 0xff, cur_shift, 0);
      // AddToCompletionText((KeySym)*key1); HEEJ
    } else {
      while (islower(key1[0]) && key1[1] == ':') {
	switch (key1[0]) {
	case 's': cur_shift |= ShiftMask; break;
	case 'c': cur_shift |= ControlMask; break;
	case 'a': cur_shift |= alt_mask; break;
	case 'm': cur_shift |= meta_mask; break;
	case 'w': cur_shift |= super_mask; break;
	default: fprintf(stderr, "%s: unknown modidier: %s\n",
			 PROGRAM_NAME, key1); break;
	}
	key1 = key1 + 2;
      }
      if (key1[0] == '0' && key1[1] == 'x') {
	long val;
	sscanf(key1, "%lx", &val);
	keysym = val;
      } else {
	keysym = XStringToKeysym(key1);
      }
      if (keysym == NoSymbol) fprintf(stderr, "%s: no such keysym: %s\n",
				      PROGRAM_NAME, key);
      if ((!appres.keypad_keysym && strncmp(key1, "KP_", 3) == 0)
	  || XKeysymToKeycode(target_dpy, keysym) == NoSymbol) {
	switch ((unsigned)keysym) {
	case XK_KP_Equal: keysym = XK_equal; break;
	case XK_KP_Divide: keysym = XK_slash; break;
	case XK_KP_Multiply: keysym = XK_asterisk; break;
	case XK_KP_Add: keysym = XK_plus; break;
	case XK_KP_Subtract: keysym = XK_minus; break;
	case XK_KP_Enter: keysym = XK_Return; break;
	case XK_KP_0: keysym = XK_0; break;
	case XK_KP_1: keysym = XK_1; break;
	case XK_KP_2: keysym = XK_2; break;
	case XK_KP_3: keysym = XK_3; break;
	case XK_KP_4: keysym = XK_4; break;
	case XK_KP_5: keysym = XK_5; break;
	case XK_KP_6: keysym = XK_6; break;
	case XK_KP_7: keysym = XK_7; break;
	case XK_KP_8: keysym = XK_8; break;
	case XK_KP_9: keysym = XK_9; break;
	case XK_Shift_L: keysym = XK_Shift_R; break;
	case XK_Shift_R: keysym = XK_Shift_L; break;
	case XK_Control_L: keysym = XK_Control_R; break;
	case XK_Control_R: keysym = XK_Control_L; break;
	case XK_Alt_L: keysym = XK_Alt_R; break;
	case XK_Alt_R: keysym = XK_Alt_L; break;
	case XK_Meta_L: keysym = XK_Meta_R; break;
	case XK_Meta_R: keysym = XK_Meta_L; break;
	case XK_Super_L: keysym = XK_Super_R; break;
	case XK_Super_R: keysym = XK_Super_L; break;
	default:
	  if (keysym == NoSymbol || !appres.auto_add_keysym)
	    fprintf(stderr, "%s: no such key: %s\n",
		    PROGRAM_NAME, key1); break;
	}
      }
      SendKeyPressedEvent(keysym, cur_shift, 0);
      //AddToCompletionText(keysym);

      if ((cur_shift & ControlMask) && (cur_shift & alt_mask)) {
        if (strstr(XServerVendor(dpy), "XFree86") != NULL) {
          if (strcmp(key1, "KP_Add") == 0) {
            if (!appres.secure) system("xvidtune -next");
          } else if (strcmp(key1, "KP_Subtract") == 0) {
            if (!appres.secure) system("xvidtune -prev");
          }
        }
      }
    }
    if (!appres.shift_lock)
      shift_state &= ~ShiftMask;
    if (!appres.modifiers_lock)
      shift_state &= ~(ControlMask | alt_mask | meta_mask | super_mask);
    if (!appres.altgr_lock)
      shift_state &= ~altgr_mask;
  }
  RefreshShiftState(FALSE);
  need_insert_blank = FALSE;

  if (w != None) {
    KeyClick();
    // StopAutoclick();
  }
}


/*
 * Redefine keyboard layout.
 * "spec" is a sequence of words separated with spaces, and it is
 * usally specified in app-defaults file, as:
 *
 *   xvkbd.AltGrKeys: \
 *      F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 F11 F12 BackSpace \n\
 *      Escape \271 \262 \263 \243 \254 \251 { [ ] } \\ ' ^ ' \n\
 *      ...
 *
 * White spaces separate the keys, and " \n" (note that white space
 * before the \n) separate the rows of keys.
 */
static void RedefineKeys(char *array[NUM_KEY_ROWS][NUM_KEY_COLS], const char *spec)
{
  char *s = XtNewString(spec);
  char *cp;
  int row, col;
  int key_rows = NUM_KEY_ROWS;
  int key_cols = NUM_KEY_COLS;

  for (row = 0; row < key_rows; row++) {
    for (col = 0; col < key_cols; col++) array[row][col] = NULL;
  }
  row = 0;
  col = 0;
  cp = strtok(s, " ");
  while (cp != NULL) {
    if (*cp == '\n') {
      row = row + 1;
      col = 0;
      cp = cp + 1;
    }
    if (*cp != '\0') {
      if (key_rows <= row) {
        fprintf(stderr, "%s: too many key rows: \"%s\" ignored\n",
                PROGRAM_NAME, cp);
      } else if (key_cols <= col) {
        fprintf(stderr, "%s: too many keys in a row: \"%s\" ignored\n",
                PROGRAM_NAME, cp);
      } else {
        array[row][col] = XtNewString(cp);
        col = col + 1;
      }
    }
    cp = strtok(NULL, " ");
  }
  XtFree(s);
}


/*
 * Load list of text to be assigned to function keys.
 * Each line contains name of the key (with optional modifier)
 * and the text to be assigned to the key, as:
 *
 *   F1 text for F1
 *   s:F2 text for Shift-F2
 */
static char fkey_filename[PATH_MAX] = "";

static struct fkey_struct {
  struct fkey_struct *next;
  char *value;
} *fkey_list = NULL;


/*
 * Edit string assigned for function keys.
 * Modifiers (Shift, Ctrl, etc.) can't be handled here.
 */
static Widget edit_fkey_panel = None;
static Widget fkey_menu_button = None;
static Widget fkey_value_menu_button = None;
static Widget fkey_value_entry = None;
static char fkey_value[100] = "";
static char cur_fkey[20] = "";
static char *cur_fkey_value_mode = "";

static void FKeyValueMenuSelected(Widget w, char *key)
{
  char *key1, *cp;

  if (key[0] == 'c') {
    cur_fkey_value_mode = "command";
    key1 = "*command";
  } else {
    cur_fkey_value_mode = "string";
    key1 = "*string";
  }
  XtVaGetValues(XtNameToWidget(fkey_value_menu_button, key1), XtNlabel, &cp, NULL);
  XtVaSetValues(fkey_value_menu_button, XtNlabel, cp, NULL);
}

static void FKeyMenuSelected(Widget w, char *key)
{
  struct fkey_struct *sp, *sp2;
  int len;
  const char *value, *prefix;
  char key2[20];

  if (appres.debug) fprintf(stderr, "xvkbd: FKeyMenuSelected(%s)\n", key);

  if (key == NULL)
    strcpy(key2, "");
  else if (strncmp(key, "Shift-", strlen("Shift-")) == 0)
    snprintf(key2, sizeof(key2), "s:%s", &key[strlen("Shift-")]);
  else
    strcpy(key2, key);

  if (strcmp(cur_fkey, key2) != 0) {
    if (strlen(cur_fkey) != 0) {
      len = strlen(cur_fkey);
      sp2 = NULL;
      for (sp = fkey_list; sp != NULL; sp = sp->next) {
	if (strncmp(sp->value, cur_fkey, len) == 0 && isspace(sp->value[len]))
	  break;
	sp2 = sp;
      }
      if (strlen(fkey_value) != 0) {  // assign new string for the function key
	if (sp == NULL) {  // it was not defined before now
	  sp = malloc(sizeof(struct fkey_struct));
	  if (fkey_list == NULL) fkey_list = sp;
	  else sp2->next = sp;
	  sp->next = NULL;
	  sp->value = NULL;
	}
	sp->value = realloc(sp->value, len + strlen(fkey_value) + 5);
	prefix = "";
	if (cur_fkey_value_mode[0] == 'c') prefix = "!";
	else if (fkey_value[0] == '!' || fkey_value[0] == '\\') prefix = "\\";
	sprintf(sp->value, "%s %s%s", cur_fkey, prefix, fkey_value);
      } else {  // empty string - remove the entry for the function key
	if (sp != NULL) {
	  if (sp2 != NULL) sp2->next = sp->next;
	  else fkey_list = sp->next;
	  free(sp->value);
	  free(sp);
	}
      }
    }

    if (key != NULL) {
      XtVaSetValues(fkey_menu_button, XtNlabel, key, NULL);

      value = FindFunctionKeyValue(None, key2, FALSE);
      if (value == NULL) value = "";

      FKeyValueMenuSelected(None, (value[0] == '!') ? "command" : "string");

      if (value[0] == '!' || value[0] == '\\') value = value + 1;
      strncpy(fkey_value, value, sizeof(fkey_value) - 1);
      XtVaSetValues(fkey_value_entry, XtNstring, fkey_value, NULL);

      strcpy(cur_fkey, key2);
    }
  }
}

/*
 * If text is assigned to the specified function key,
 * return the text.  Otherwise, return NULL.
 */
static const char *FindFunctionKeyValue(Widget w, const char *key, Boolean shiftable)
{
  char label[50];
  char prefix;
  struct fkey_struct *sp;
  int len;

  if (w != None) {
    int row, col;
    const char *s1;
    if (sscanf(key, "pad%d,%d", &row, &col) == 2) {
      s1 = appres.num_lock_state ? keypad_shift[row][col]: keypad[row][col];
      key = s1;
    }
  }

  prefix = '\0';
  if (shiftable) {
    if (shift_state & super_mask) prefix = 'w';
    else if (shift_state & meta_mask) prefix = 'm';
    else if (shift_state & alt_mask) prefix = 'a';
    else if (shift_state & ControlMask) prefix = 'c';
    else if (shift_state & ShiftMask) prefix = 's';
  }
  if (prefix == '\0') snprintf(label, sizeof(label), "%s", key);
  else snprintf(label, sizeof(label), "%c:%s", prefix, key);
  len = strlen(label);

  for (sp = fkey_list; sp != NULL; sp = sp->next) {
    if (strncmp(sp->value, label, len) == 0 && isspace(sp->value[len]))
      return &(sp->value[len + 1]);
  }
  return NULL;
}

/*
 * Key click
 */
void KeyClick(void)
{
  XKeyboardState ks;
  XKeyboardControl kc;

  if (0 < appres.key_click_duration) {
    XGetKeyboardControl(dpy, &ks);

    kc.bell_duration = ks.bell_duration;
    kc.bell_pitch = appres.key_click_pitch;
    kc.bell_duration = appres.key_click_duration;
    XChangeKeyboardControl(dpy, KBBellPitch | KBBellDuration, &kc);
    XBell(dpy, 0);
    XSync(dpy, FALSE);

    kc.bell_pitch = ks.bell_pitch;
    kc.bell_duration = ks.bell_duration;
    XChangeKeyboardControl(dpy, KBBellPitch | KBBellDuration, &kc);
    XSync(dpy, FALSE);
  }
}

/*
 * Display balloon message for the function keys,
 * if text is assigned to the key.
 */
static Boolean balloon_panel_open = FALSE;
static Widget balloon_panel = None;

static	XtIntervalId autoclick_id = (XtIntervalId)0;

static void StopAutoclick(void)
{
  if (autoclick_id != (XtIntervalId)0) {
    if (appres.debug) fprintf(stderr, "xvkbd: StopAutoclick: %lx\n", (long)autoclick_id);

    XtRemoveTimeOut(autoclick_id);
    autoclick_id = (XtIntervalId)0;
  }
}

static void Autoclick(void)
{
  StopAutoclick();

  XTestFakeButtonEvent(target_dpy, 1, True, CurrentTime);
  XTestFakeButtonEvent(target_dpy, 1, False, CurrentTime);
}




















/*
 * The main program.
 */
int main(int argc, char *argv[]){
	
	static String fallback_resources[] = {
		#include "XVkbd-common.h"
		NULL,
	};
	
	Boolean open_keypad_panel = FALSE;
	char ch;
	Window child;
	int op, ev, err;
	
	argc1 = argc;
	argv1 = malloc(sizeof(char *) * (argc1 + 5));
	memcpy(argv1, argv, sizeof(char *) * argc1);
	argv1[argc1] = NULL;
	
#ifdef USE_I18N
	XtSetLanguageProc(NULL, NULL, NULL);
#endif
	
	toplevel = XtVaAppInitialize(
		NULL, "XVkbd",
		options, XtNumber(options),
		&argc, argv, fallback_resources, NULL
	);
	
	dpy = XtDisplay(toplevel);
	app_con = XtWidgetToApplicationContext(toplevel);
	
	target_dpy = dpy;
	
	if(1 < argc){
		fprintf(stderr, "%s: illegal option: %s\n\n", PROGRAM_NAME, argv[1]);
	}
	
	XtGetApplicationResources(
		toplevel, &appres,
		application_resources, XtNumber(application_resources),
		NULL, 0
	);
	
	if(appres.version){
		fprintf(stdout, "%s\n", appres.description);
		exit(1);
	}
	if(appres.debug){
		fprintf(stdout, "%s, compiled %s\n", PROGRAM_NAME_WITH_VERSION, __DATE__);
	}
	if(appres.compact){
		appres.keypad = FALSE;
		appres.function_key = FALSE;
	}
	if(appres.keypad_only && !appres.keypad){
		appres.keypad_only = FALSE;
		open_keypad_panel = TRUE;
	}
	if(1 || appres.no_sync){
		XSync(dpy, FALSE);
		XSetErrorHandler(MyErrorHandler);
	}
	
	focused_subwindow = focused_window;
	MappingModified(None, NULL, NULL, NULL);
	
	if(strlen(appres.text) != 0){
		appres.keypad_keysym = TRUE;
		SendString(appres.text);
		// printf("success\n");
		exit(0);
	}
	
	// printf("failure\n");
	
	exit(1);
}
