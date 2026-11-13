#pragma once
/*
 * T-Deck phone (https://github.com/rushikesh3982/tdeck-phone): what the touch interface needs from the phone.
 *
 * The firmware's PhoneModule (rushikesh3982/meshtastic-phone, src/modules/PhoneModule.cpp) implements this and
 * registers itself with IPhone::set(). In any other build nothing registers, IPhone::get() stays null, and the
 * interface shows nothing phone-related.
 *
 * The methods are called from the interface's own task: implementations must be safe to call from any task.
 */
#include <stdint.h>

// What the phone knows about its modem.
struct PhoneStatus {
    bool modemAlive = false; // the modem answers
    char model[24] = "";     // e.g. A7672S-FASE

    enum Sim : uint8_t { SIM_UNKNOWN, SIM_READY, SIM_NEEDS_PIN, SIM_MISSING };
    Sim sim = SIM_UNKNOWN;

    int8_t registration = -1; // +CEREG: 1 home, 5 roaming, 2 searching, 3 refused, 0 not searching, -1 unknown
    char operatorName[24] = "";
    bool volte = false;       // registered for calls over 4G
    bool lte = false;         // on 4G now
    int16_t rsrpDbm = 0;      // 4G signal, 0 = unknown
    int16_t band = -1;
    uint16_t modemMillivolts = 0;

    bool gnssOn = false;
    bool gnssFix = false;
    uint8_t satellites = 0;

    enum Call : uint8_t { CALL_IDLE, CALL_RINGING, CALL_DIALING, CALL_ACTIVE };
    Call call = CALL_IDLE;
    char callNumber[24] = ""; // the other party, while ringing or in a call

    // SMS
    uint8_t unreadSms = 0;
    uint8_t smsStored = 0, smsCapacity = 0; // on the SIM (or the modem's own storage)
    enum SmsSend : uint8_t { SMS_SEND_IDLE, SMS_SENDING, SMS_SENT, SMS_FAILED };
    SmsSend smsSend = SMS_SEND_IDLE; // the latest message sent

    // Battery (one cell feeds the T-Deck and the modem)
    bool batteryLow = false;         // below 3.4 V: a warning
    bool modemOffForBattery = false; // below 3.3 V the phone switched the modem off; charge, then switch it off and on
};

// One text message. Received ones are kept on the SIM; sent ones only until the T-Deck restarts.
struct PhoneSms {
    int16_t index = -1;    // its place on the SIM, -1 = not stored there (a sent message)
    bool incoming = true;
    bool unread = false;
    char number[24] = "";
    char time[24] = "";    // as the network gives it: yy/MM/dd,hh:mm:ss+zz
    char text[161] = "";   // plain text (characters the fonts can't show become '?')
};

class IPhone
{
  public:
    virtual ~IPhone() = default;

    virtual PhoneStatus getStatus() = 0;
    virtual bool dial(const char *number) = 0;
    virtual bool answer() = 0;
    virtual bool hangUp() = 0;

    // SMS, newest first. smsVersion() changes whenever the list does, so the interface knows to redraw it.
    virtual uint8_t smsCount() = 0;
    virtual bool smsGet(uint8_t position, PhoneSms &out) = 0;
    virtual bool sendSms(const char *number, const char *text) = 0;
    virtual bool deleteSms(uint8_t position) = 0;
    virtual void markSmsRead(uint8_t position) = 0;
    virtual uint16_t smsVersion() = 0;

    static void set(IPhone *phone);
    static IPhone *get();
};
