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
uint32_t callStartedAt = 0;
uint32_t lastRefresh = 0;
char homeText[96] = "";
char detailsText[200] = "";
char callStateText[40] = "";
char callNumberText[24] = "";

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
    lv_textarea_set_one_line(numberArea, true);
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

void describe(const PhoneStatus &s, char *home, size_t homeSize, char *details, size_t detailsSize)
{
    const char *gps = !s.gnssOn ? "GPS off" : s.gnssFix ? "GPS fix" : "GPS searching";
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
