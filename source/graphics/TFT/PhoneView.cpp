// T-Deck phone (https://github.com/rushikesh3982/tdeck-phone): the phone's parts of the 320x240 touch interface.
// See include/graphics/view/TFT/PhoneView.h.
#if defined(TDECK_PHONE) && defined(VIEW_320x240)

#include "graphics/view/TFT/PhoneView.h"
#include "fonts.h"
#include "phone/IPhone.h"
#include "styles.h"
#include <stdio.h>
#include <string.h>

namespace
{
constexpr uint32_t REFRESH_MS = 250;
const lv_color_t GREEN = lv_color_hex(0x2e7d32);
const lv_color_t RED = lv_color_hex(0xc62828);
const lv_color_t BLUE = lv_color_hex(0x1565c0);
const lv_color_t GREY = lv_color_hex(0x546e7a);

void (*showPanel)(lv_obj_t *panel) = nullptr;

// home screen row
lv_obj_t *homeButton = nullptr;
lv_obj_t *homeLabel = nullptr;

// phone panel
lv_obj_t *panel = nullptr;
lv_obj_t *numberArea = nullptr;
lv_obj_t *detailsLabel = nullptr;

// call screen
lv_obj_t *callScreen = nullptr;
lv_obj_t *callStateLabel = nullptr;
lv_obj_t *callNumberLabel = nullptr;
lv_obj_t *answerButton = nullptr;
lv_obj_t *rejectButton = nullptr;
lv_obj_t *hangUpButton = nullptr;
lv_obj_t *focusBeforeCall = nullptr;

PhoneStatus::Call shownCall = PhoneStatus::CALL_IDLE;
uint8_t shownUnread = 0;
uint32_t callStartedAt = 0;
uint32_t lastRefresh = 0;
char homeText[96] = "";
char detailsText[200] = "";
char callStateText[40] = "";
char callNumberText[24] = "";

// SMS panel: a list, a message, and a new message, one shown at a time
lv_obj_t *smsPanel = nullptr;
lv_obj_t *smsListView = nullptr;
lv_obj_t *smsReadView = nullptr;
lv_obj_t *smsComposeView = nullptr;
lv_obj_t *smsTitle = nullptr;
lv_obj_t *smsList = nullptr;
lv_obj_t *smsReadHeader = nullptr;
lv_obj_t *smsReadText = nullptr;
lv_obj_t *smsDeleteLabel = nullptr;
lv_obj_t *smsTo = nullptr;
lv_obj_t *smsBody = nullptr;
lv_obj_t *smsCounter = nullptr;
lv_obj_t *smsSendState = nullptr;
PhoneSms smsOpen;            // the message being read (positions shift as messages come and go)
bool smsDeleteArmed = false; // Delete asks once ("Sure?") before deleting
uint16_t smsShownVersion = 0xffff;
PhoneStatus::SmsSend shownSend = PhoneStatus::SMS_SEND_IDLE;
char smsTitleText[48] = "";
char smsSendText[40] = "";

const char *const KEYPAD[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n",
                              "*", "0", "#", "\n", "+", LV_SYMBOL_BACKSPACE, ""};

void setText(lv_obj_t *label, char *shown, size_t size, const char *text)
{
    if (strcmp(shown, text) != 0) {
        strncpy(shown, text, size - 1);
        shown[size - 1] = 0;
        lv_label_set_text(label, text);
    }
}

// A one-line text field 34 px high: less padding than the theme's, so the text sits in the middle and no scrollbar shows.
void oneLineField(lv_obj_t *field)
{
    lv_textarea_set_one_line(field, true);
    lv_obj_set_style_pad_ver(field, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(field, LV_SCROLLBAR_MODE_OFF);
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_style_bg_color(button, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);
    return button;
}

void dialNumber()
{
    IPhone *phone = IPhone::get();
    const char *number = lv_textarea_get_text(numberArea);
    if (phone && number && number[0])
        phone->dial(number);
}

void onOpenPanel(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || !showPanel)
        return;
    showPanel(panel);
    lv_group_t *group = lv_group_get_default();
    if (group)
        lv_group_focus_obj(numberArea); // so the T-Deck's keyboard types the number
}

void onKeypad(lv_event_t *e)
{
    lv_obj_t *keypad = (lv_obj_t *)lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(keypad);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE)
        return;
    const char *key = lv_buttonmatrix_get_button_text(keypad, id);
    if (!key)
        return;
    if (strcmp(key, LV_SYMBOL_BACKSPACE) == 0)
        lv_textarea_delete_char(numberArea);
    else
        lv_textarea_add_text(numberArea, key);
}

void onCall(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        dialNumber();
}

void onNumberReady(lv_event_t *e)
{
    dialNumber(); // Enter on the keyboard
}

void onAnswer(lv_event_t *e)
{
    if (IPhone::get())
        IPhone::get()->answer();
}

void onHangUp(lv_event_t *e)
{
    if (IPhone::get())
        IPhone::get()->hangUp();
}

void buildHomeRow(lv_obj_t *homeContainer)
{
    // The same shape as the home screen's other rows: a 36x36 button and a label beside it, at the top.
    homeButton = lv_button_create(homeContainer);
    lv_obj_set_size(homeButton, 36, 36);
    add_style_home_button_style(homeButton);
    lv_obj_set_style_shadow_width(homeButton, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *icon = lv_label_create(homeButton);
    lv_label_set_text(icon, LV_SYMBOL_CALL);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(icon);
    lv_obj_add_event_cb(homeButton, onOpenPanel, LV_EVENT_CLICKED, nullptr);

    homeLabel = lv_label_create(homeContainer);
    lv_obj_set_size(homeLabel, LV_PCT(80), LV_SIZE_CONTENT);
    lv_obj_set_style_text_line_space(homeLabel, -4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(homeLabel, "Phone");
    lv_obj_add_flag(homeLabel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(homeLabel, onOpenPanel, LV_EVENT_CLICKED, nullptr);

    lv_obj_move_to_index(homeButton, 0);
    lv_obj_move_to_index(homeLabel, 1);
}

void onOpenSms(lv_event_t *e); // in the SMS part below

void buildPanel(lv_obj_t *mainScreen)
{
    // Where the view's own panels sit: right of the button bar, below the top bar.
    panel = lv_obj_create(mainScreen);
    lv_obj_set_pos(panel, LV_PCT(12), LV_PCT(10));
    lv_obj_set_size(panel, LV_PCT(88), LV_PCT(90));
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    add_style_panel_style(panel);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    numberArea = lv_textarea_create(panel);
    oneLineField(numberArea);
    lv_textarea_set_placeholder_text(numberArea, "Number");
    lv_textarea_set_accepted_chars(numberArea, "0123456789+*#");
    lv_textarea_set_max_length(numberArea, 20);
    lv_obj_set_size(numberArea, LV_PCT(62), 34);
    lv_obj_align(numberArea, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(numberArea, onNumberReady, LV_EVENT_READY, nullptr);

    lv_obj_t *callButton = makeButton(panel, LV_SYMBOL_CALL " Call", GREEN);
    lv_obj_set_size(callButton, LV_PCT(35), 34);
    lv_obj_align(callButton, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(callButton, onCall, LV_EVENT_CLICKED, nullptr);

    detailsLabel = lv_label_create(panel);
    lv_obj_set_width(detailsLabel, LV_PCT(48));
    lv_obj_align(detailsLabel, LV_ALIGN_TOP_LEFT, 0, 42);
    lv_obj_set_style_text_font(detailsLabel, &ui_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(detailsLabel, "");

    lv_obj_t *smsButton = makeButton(panel, LV_SYMBOL_ENVELOPE " SMS", BLUE);
    lv_obj_set_size(smsButton, LV_PCT(46), 32);
    lv_obj_align(smsButton, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_event_cb(smsButton, onOpenSms, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *keypad = lv_buttonmatrix_create(panel);
    lv_buttonmatrix_set_map(keypad, KEYPAD);
    lv_obj_set_size(keypad, LV_PCT(50), 146);
    lv_obj_align(keypad, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_pad_all(keypad, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(keypad, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(keypad, &lv_font_montserrat_16, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(keypad, onKeypad, LV_EVENT_VALUE_CHANGED, nullptr);
}

void buildCallScreen()
{
    // Over everything, on LVGL's top layer.
    callScreen = lv_obj_create(lv_layer_top());
    lv_obj_set_size(callScreen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(callScreen, lv_color_hex(0x101820), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(callScreen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(callScreen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(callScreen, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(callScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(callScreen, LV_OBJ_FLAG_HIDDEN);

    callStateLabel = lv_label_create(callScreen);
    lv_obj_set_style_text_font(callStateLabel, &ui_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(callStateLabel, lv_color_hex(0xb0bec5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(callStateLabel, LV_ALIGN_TOP_MID, 0, 40);

    callNumberLabel = lv_label_create(callScreen);
    lv_obj_set_style_text_font(callNumberLabel, &ui_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(callNumberLabel, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(callNumberLabel, LV_ALIGN_TOP_MID, 0, 80);

    answerButton = makeButton(callScreen, LV_SYMBOL_CALL " Answer", GREEN);
    lv_obj_set_size(answerButton, 130, 48);
    lv_obj_align(answerButton, LV_ALIGN_BOTTOM_LEFT, 10, -20);
    lv_obj_add_event_cb(answerButton, onAnswer, LV_EVENT_CLICKED, nullptr);

    rejectButton = makeButton(callScreen, LV_SYMBOL_CLOSE " Reject", RED);
    lv_obj_set_size(rejectButton, 130, 48);
    lv_obj_align(rejectButton, LV_ALIGN_BOTTOM_RIGHT, -10, -20);
    lv_obj_add_event_cb(rejectButton, onHangUp, LV_EVENT_CLICKED, nullptr);

    hangUpButton = makeButton(callScreen, LV_SYMBOL_CLOSE " Hang up", RED);
    lv_obj_set_size(hangUpButton, 160, 48);
    lv_obj_align(hangUpButton, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(hangUpButton, onHangUp, LV_EVENT_CLICKED, nullptr);
}

void showCall(const PhoneStatus &s)
{
    lv_group_t *group = lv_group_get_default();
    if (s.call == PhoneStatus::CALL_IDLE) {
        lv_obj_add_flag(callScreen, LV_OBJ_FLAG_HIDDEN);
        if (group && focusBeforeCall && lv_obj_is_valid(focusBeforeCall))
            lv_group_focus_obj(focusBeforeCall);
        focusBeforeCall = nullptr;
        return;
    }
    if (lv_obj_has_flag(callScreen, LV_OBJ_FLAG_HIDDEN)) {
        focusBeforeCall = group ? lv_group_get_focused(group) : nullptr;
        lv_obj_remove_flag(callScreen, LV_OBJ_FLAG_HIDDEN);
    }
    bool ringing = s.call == PhoneStatus::CALL_RINGING;
    if (ringing) {
        lv_obj_remove_flag(answerButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(rejectButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(hangUpButton, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(answerButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(rejectButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(hangUpButton, LV_OBJ_FLAG_HIDDEN);
    }
    if (group)
        lv_group_focus_obj(ringing ? answerButton : hangUpButton); // so the trackball's press works too
    if (s.call == PhoneStatus::CALL_ACTIVE)
        callStartedAt = lv_tick_get();
}


// ---- SMS

// "26/11/13,10:30:00+22" (yy/MM/dd,hh:mm:ss+zz) -> "13/11 10:30"
void shortTime(const char *t, char *out, size_t len)
{
    if (strlen(t) >= 14 && t[8] == ',')
        snprintf(out, len, "%.2s/%.2s %.5s", t + 6, t + 3, t + 9);
    else
        snprintf(out, len, "%s", t);
}

void showSmsView(lv_obj_t *view)
{
    lv_obj_add_flag(smsListView, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(smsReadView, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(smsComposeView, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_HIDDEN);
}

// Where the message being read now is in the list, -1 if it's gone.
int findOpen()
{
    IPhone *phone = IPhone::get();
    if (!phone)
        return -1;
    PhoneSms m;
    for (uint8_t i = 0; i < phone->smsCount(); i++) {
        if (phone->smsGet(i, m) && m.index == smsOpen.index && m.incoming == smsOpen.incoming &&
            !strcmp(m.time, smsOpen.time) && !strcmp(m.number, smsOpen.number))
            return i;
    }
    return -1;
}

void rebuildSmsList();

void openCompose(const char *to)
{
    lv_textarea_set_text(smsTo, to ? to : "");
    lv_textarea_set_text(smsBody, "");
    lv_label_set_text(smsCounter, "0/160");
    lv_label_set_text(smsSendState, "");
    smsSendText[0] = 0;
    showSmsView(smsComposeView);
    lv_group_t *group = lv_group_get_default();
    if (group)
        lv_group_focus_obj(to && to[0] ? smsBody : smsTo);
}

void onSmsItem(lv_event_t *e)
{
    IPhone *phone = IPhone::get();
    uint8_t position = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (!phone || !phone->smsGet(position, smsOpen))
        return;
    phone->markSmsRead(position);
    char when[16];
    shortTime(smsOpen.time, when, sizeof when);
    char header[64];
    snprintf(header, sizeof header, "%s %s%s%s", smsOpen.incoming ? "From" : "To", smsOpen.number, when[0] ? ", " : "", when);
    lv_label_set_text(smsReadHeader, header);
    lv_label_set_text(smsReadText, smsOpen.text);
    smsDeleteArmed = false;
    lv_label_set_text(smsDeleteLabel, "Delete");
    showSmsView(smsReadView);
}

void onSmsBack(lv_event_t *e)
{
    showSmsView(smsListView);
    smsShownVersion = 0xffff; // redraw the list
}

void onSmsNew(lv_event_t *e)
{
    // a number typed on the phone panel becomes the recipient
    openCompose(lv_textarea_get_text(numberArea));
}

void onSmsReply(lv_event_t *e)
{
    openCompose(smsOpen.number);
}

void onSmsCall(lv_event_t *e)
{
    if (IPhone::get())
        IPhone::get()->dial(smsOpen.number);
}

void onSmsDelete(lv_event_t *e)
{
    if (!smsDeleteArmed) {
        smsDeleteArmed = true;
        lv_label_set_text(smsDeleteLabel, "Sure?");
        return;
    }
    int position = findOpen();
    if (position >= 0 && IPhone::get())
        IPhone::get()->deleteSms((uint8_t)position);
    onSmsBack(e);
}

void sendSms()
{
    IPhone *phone = IPhone::get();
    const char *to = lv_textarea_get_text(smsTo);
    const char *text = lv_textarea_get_text(smsBody);
    if (!phone || !to[0] || !text[0])
        return;
    if (!phone->sendSms(to, text))
        lv_label_set_text(smsSendState, "Not sent: busy, or a wrong number");
}

void onSmsSend(lv_event_t *e)
{
    sendSms();
}

// Enter in the message sends it; Enter in "To" moves on to the message.
void onSmsBodyInsert(lv_event_t *e)
{
    const char *text = (const char *)lv_event_get_param(e);
    if (text && strcmp(text, "\n") == 0) {
        lv_textarea_set_insert_replace(smsBody, "");
        sendSms();
    }
}

void onSmsToReady(lv_event_t *e)
{
    lv_group_t *group = lv_group_get_default();
    if (group)
        lv_group_focus_obj(smsBody);
}

void onSmsBodyChanged(lv_event_t *e)
{
    char count[12];
    snprintf(count, sizeof count, "%u/160", (unsigned)strlen(lv_textarea_get_text(smsBody)));
    lv_label_set_text(smsCounter, count);
}

void onOpenSms(lv_event_t *e)
{
    if (!showPanel)
        return;
    showPanel(smsPanel);
    showSmsView(smsListView);
    smsShownVersion = 0xffff;
    rebuildSmsList();
}

lv_obj_t *makeView(lv_obj_t *parent)
{
    // No styles of its own: its labels take the panel's text colour, so they read well in every theme.
    lv_obj_t *view = lv_obj_create(parent);
    lv_obj_remove_style_all(view);
    lv_obj_set_size(view, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);
    return view;
}

lv_obj_t *smallButton(lv_obj_t *parent, const char *text, lv_color_t color, lv_event_cb_t cb, lv_obj_t **labelOut = nullptr)
{
    lv_obj_t *button = makeButton(parent, text, color);
    lv_obj_set_size(button, 62, 30);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, nullptr);
    if (labelOut)
        *labelOut = lv_obj_get_child(button, 0);
    return button;
}

void buildSmsPanel(lv_obj_t *mainScreen)
{
    smsPanel = lv_obj_create(mainScreen);
    lv_obj_set_pos(smsPanel, LV_PCT(12), LV_PCT(10));
    lv_obj_set_size(smsPanel, LV_PCT(88), LV_PCT(90));
    lv_obj_remove_flag(smsPanel, LV_OBJ_FLAG_SCROLLABLE);
    add_style_panel_style(smsPanel);
    lv_obj_add_flag(smsPanel, LV_OBJ_FLAG_HIDDEN);

    // the list: a title with the count on the SIM, a New button, and the messages, newest first
    smsListView = makeView(smsPanel);
    smsTitle = lv_label_create(smsListView);
    lv_obj_set_style_text_font(smsTitle, &ui_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(smsTitle, LV_ALIGN_TOP_LEFT, 0, 8);
    lv_label_set_text(smsTitle, "SMS");
    lv_obj_t *newButton = makeButton(smsListView, LV_SYMBOL_EDIT " New", BLUE);
    lv_obj_set_size(newButton, 84, 30);
    lv_obj_align(newButton, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(newButton, onSmsNew, LV_EVENT_CLICKED, nullptr);
    smsList = lv_list_create(smsListView);
    lv_obj_set_size(smsList, LV_PCT(100), LV_PCT(80));
    lv_obj_align(smsList, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(smsList, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    // a message: who and when, the text (it scrolls), and what to do with it
    smsReadView = makeView(smsPanel);
    smsReadHeader = lv_label_create(smsReadView);
    lv_obj_set_width(smsReadHeader, LV_PCT(100));
    lv_obj_set_style_text_font(smsReadHeader, &ui_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(smsReadHeader, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *textBox = lv_obj_create(smsReadView);
    lv_obj_set_size(textBox, LV_PCT(100), LV_PCT(66));
    lv_obj_align(textBox, LV_ALIGN_TOP_LEFT, 0, 18);
    lv_obj_set_style_pad_all(textBox, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    smsReadText = lv_label_create(textBox);
    lv_obj_set_width(smsReadText, LV_PCT(100));
    lv_label_set_long_mode(smsReadText, LV_LABEL_LONG_WRAP);
    // four buttons sharing the row's width equally
    lv_obj_t *row = lv_obj_create(smsReadView);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 30);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *rowButtons[] = {smallButton(row, "Back", GREY, onSmsBack), smallButton(row, "Reply", BLUE, onSmsReply),
                              smallButton(row, "Call", GREEN, onSmsCall),
                              smallButton(row, "Delete", RED, onSmsDelete, &smsDeleteLabel)};
    for (lv_obj_t *button : rowButtons)
        lv_obj_set_flex_grow(button, 1);

    // a new message: to, text (160 characters, one SMS), Back and Send
    smsComposeView = makeView(smsPanel);
    smsTo = lv_textarea_create(smsComposeView);
    oneLineField(smsTo);
    lv_textarea_set_placeholder_text(smsTo, "To");
    lv_textarea_set_accepted_chars(smsTo, "0123456789+");
    lv_textarea_set_max_length(smsTo, 20);
    lv_obj_set_size(smsTo, LV_PCT(100), 34);
    lv_obj_align(smsTo, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(smsTo, onSmsToReady, LV_EVENT_READY, nullptr);
    smsBody = lv_textarea_create(smsComposeView);
    lv_textarea_set_placeholder_text(smsBody, "Message (Enter sends it)");
    lv_textarea_set_max_length(smsBody, 160);
    lv_obj_set_size(smsBody, LV_PCT(100), 96);
    lv_obj_align(smsBody, LV_ALIGN_TOP_LEFT, 0, 38);
    lv_obj_add_event_cb(smsBody, onSmsBodyInsert, LV_EVENT_INSERT, nullptr);
    lv_obj_add_event_cb(smsBody, onSmsBodyChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    smsCounter = lv_label_create(smsComposeView);
    lv_obj_set_style_text_font(smsCounter, &ui_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(smsCounter, LV_ALIGN_TOP_RIGHT, 0, 138);
    smsSendState = lv_label_create(smsComposeView);
    lv_obj_set_style_text_font(smsSendState, &ui_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(smsSendState, LV_ALIGN_TOP_LEFT, 0, 138);
    lv_obj_t *back = smallButton(smsComposeView, "Back", GREY, onSmsBack);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t *send = makeButton(smsComposeView, LV_SYMBOL_UPLOAD " Send", GREEN);
    lv_obj_set_size(send, 100, 30);
    lv_obj_align(send, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(send, onSmsSend, LV_EVENT_CLICKED, nullptr);

    showSmsView(smsListView);
}

void rebuildSmsList()
{
    IPhone *phone = IPhone::get();
    if (!phone)
        return;
    uint16_t version = phone->smsVersion();
    if (version == smsShownVersion)
        return;
    smsShownVersion = version;
    lv_obj_clean(smsList);
    uint8_t count = phone->smsCount();
    if (count == 0) {
        lv_obj_t *none = lv_label_create(smsList);
        lv_label_set_text(none, "No messages");
        return;
    }
    PhoneSms m;
    for (uint8_t i = 0; i < count; i++) {
        if (!phone->smsGet(i, m))
            break;
        char when[16], firstLine[44], item[96];
        shortTime(m.time, when, sizeof when);
        size_t n = 0;
        for (const char *p = m.text; *p && n < sizeof firstLine - 1; p++)
            firstLine[n++] = *p == '\n' ? ' ' : *p;
        firstLine[n] = 0;
        snprintf(item, sizeof item, "%s%s%s  %s\n%s", m.unread ? "* " : "", m.incoming ? "" : "To ", m.number, when,
                 firstLine);
        lv_obj_t *button = lv_list_add_button(smsList, nullptr, item);
        lv_obj_set_style_text_font(button, &ui_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_t *label = lv_obj_get_child(button, 0); // the list scrolls long text sideways; two still lines instead
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_height(label, 2 * lv_font_get_line_height(&ui_font_montserrat_12));
        lv_obj_add_event_cb(button, onSmsItem, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
}

void updateSms(const PhoneStatus &s)
{
    if (lv_obj_has_flag(smsPanel, LV_OBJ_FLAG_HIDDEN))
        return;
    char title[sizeof smsTitleText];
    if (s.smsCapacity)
        snprintf(title, sizeof title, "SMS: %u of %u on SIM", s.smsStored, s.smsCapacity);
    else
        snprintf(title, sizeof title, "SMS");
    setText(smsTitle, smsTitleText, sizeof smsTitleText, title);
    if (!lv_obj_has_flag(smsListView, LV_OBJ_FLAG_HIDDEN))
        rebuildSmsList();

    if (s.smsSend != shownSend) {
        shownSend = s.smsSend;
        if (!lv_obj_has_flag(smsComposeView, LV_OBJ_FLAG_HIDDEN)) {
            const char *state = s.smsSend == PhoneStatus::SMS_SENDING ? "Sending..."
                                : s.smsSend == PhoneStatus::SMS_SENT  ? "Sent"
                                : s.smsSend == PhoneStatus::SMS_FAILED ? "Not sent: try again"
                                                                       : "";
            setText(smsSendState, smsSendText, sizeof smsSendText, state);
            if (s.smsSend == PhoneStatus::SMS_SENT)
                lv_textarea_set_text(smsBody, "");
        }
    }
}

void describe(const PhoneStatus &s, char *home, size_t homeSize, char *details, size_t detailsSize)
{
    const char *gps = !s.gnssOn ? "GPS off" : s.gnssFix ? "GPS fix" : "GPS searching";
    if (s.modemOffForBattery && !s.modemAlive) {
        snprintf(home, homeSize, "Phone off: battery low\ncharge, then switch the modem off and on");
        snprintf(details, detailsSize, "Modem: switched off,\nbattery low.\nCharge, then switch\nthe modem off and on");
        return;
    }
    if (!s.modemAlive) {
        snprintf(home, homeSize, "Phone off\nthe modem's switch is off");
        snprintf(details, detailsSize, "Modem: off\n(its switch, on the\nback's right side)");
        return;
    }
    char signal[24] = "";
    if (s.lte && s.rsrpDbm)
        snprintf(signal, sizeof signal, "%d dBm", s.rsrpDbm);

    const char *sim = s.sim == PhoneStatus::SIM_READY       ? "ready"
                      : s.sim == PhoneStatus::SIM_NEEDS_PIN ? "needs its PIN"
                      : s.sim == PhoneStatus::SIM_MISSING   ? "not found"
                                                            : "checking";
    const char *net = s.registration == 1   ? "home"
                      : s.registration == 5 ? "roaming"
                      : s.registration == 2 ? "searching"
                      : s.registration == 3 ? "refused"
                                            : "not registered";
    bool registered = s.registration == 1 || s.registration == 5;

    if (s.sim != PhoneStatus::SIM_READY)
        snprintf(home, homeSize, "Phone: SIM %s\n%s", sim, gps);
    else if (!registered)
        snprintf(home, homeSize, "Phone: %s\n%s", net, gps);
    else
        snprintf(home, homeSize, "%s %s  %s\n%s%s", s.operatorName[0] ? s.operatorName : "Network", s.lte ? "4G" : "2G",
                 signal, s.volte ? "VoLTE, " : "", gps);

    char band[12] = "";
    if (s.lte && s.band > 0)
        snprintf(band, sizeof band, ", B%d", s.band);
    char gpsDetail[32];
    if (s.gnssFix)
        snprintf(gpsDetail, sizeof gpsDetail, "fix, %u satellites", s.satellites);
    else
        snprintf(gpsDetail, sizeof gpsDetail, "%s", s.gnssOn ? "searching" : "off");
    snprintf(details, detailsSize, "SIM: %s\nNetwork: %s\n%s\nVoLTE: %s\n4G: %s%s\nBattery: %.2f V\nGPS: %s",
             sim, net, s.operatorName, s.volte ? "yes" : "no", signal[0] ? signal : "-", band, s.modemMillivolts / 1000.0,
             gpsDetail);
}
} // namespace

namespace PhoneView
{
void init(lv_obj_t *homeContainer, lv_obj_t *mainScreen, void (*show)(lv_obj_t *panel))
{
    showPanel = show; // the phone registers itself later; until it does, update() shows nothing
    buildHomeRow(homeContainer);
    buildPanel(mainScreen);
    buildSmsPanel(mainScreen);
    buildCallScreen();
}

void update(void)
{
    IPhone *phone = IPhone::get();
    if (!phone || !homeLabel || lv_tick_elaps(lastRefresh) < REFRESH_MS)
        return;
    lastRefresh = lv_tick_get();

    PhoneStatus s = phone->getStatus();
    char home[sizeof homeText];
    char details[sizeof detailsText];
    describe(s, home, sizeof home, details, sizeof details);
    if (s.unreadSms) {
        char sms[24];
        snprintf(sms, sizeof sms, ", %u new SMS", s.unreadSms);
        strncat(home, sms, sizeof home - strlen(home) - 1);
    }
    if (s.batteryLow && s.modemAlive)
        strncat(home, ", battery low", sizeof home - strlen(home) - 1);

    // The screen comes on for a call (and stays on until it is answered) and for a new SMS
    if (s.call == PhoneStatus::CALL_RINGING || s.call == PhoneStatus::CALL_DIALING ||
        (s.unreadSms > shownUnread))
        lv_display_trigger_activity(NULL);
    shownUnread = s.unreadSms;
    updateSms(s);
    setText(homeLabel, homeText, sizeof homeText, home);
    if (!lv_obj_has_flag(panel, LV_OBJ_FLAG_HIDDEN))
        setText(detailsLabel, detailsText, sizeof detailsText, details);

    if (s.call != shownCall) {
        showCall(s);
        shownCall = s.call;
    }
    if (s.call != PhoneStatus::CALL_IDLE) {
        char state[sizeof callStateText];
        if (s.call == PhoneStatus::CALL_RINGING) {
            snprintf(state, sizeof state, "Incoming call");
        } else if (s.call == PhoneStatus::CALL_DIALING) {
            snprintf(state, sizeof state, "Calling...");
        } else {
            uint32_t secs = lv_tick_elaps(callStartedAt) / 1000;
            snprintf(state, sizeof state, "%s  %02u:%02u", s.volte ? "In call, 4G" : "In call", (unsigned)(secs / 60),
                     (unsigned)(secs % 60));
        }
        setText(callStateLabel, callStateText, sizeof callStateText, state);
        setText(callNumberLabel, callNumberText, sizeof callNumberText, s.callNumber[0] ? s.callNumber : "Unknown number");
    }
}
} // namespace PhoneView

#endif
