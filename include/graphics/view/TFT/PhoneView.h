#pragma once
/*
 * T-Deck phone (https://github.com/rushikesh3982/tdeck-phone): the phone's parts of the 320x240 touch interface.
 *
 * - a row at the top of the home screen: operator, 4G signal, VoLTE, GPS. Tapping it opens the phone panel;
 * - the phone panel: a number field, a keypad, a Call button, and the modem's details;
 * - the call screen, over everything: incoming (Answer / Reject), calling and in a call (Hang up), with the number
 *   and the call's length.
 *
 * Only built in with TDECK_PHONE, and it shows nothing unless a phone registered itself (IPhone::set).
 */
#include "lvgl.h"

namespace PhoneView
{
// Builds the phone's parts. showPanel switches the main area to a panel, the way the view's own buttons do.
void init(lv_obj_t *homeContainer, lv_obj_t *mainScreen, void (*showPanel)(lv_obj_t *panel));

// Refreshes everything from the phone's status. Cheap: call it from the view's task handler as often as it runs.
void update(void);
} // namespace PhoneView
