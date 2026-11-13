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

    int16_t newSmsIndex = -1; // the index of the latest SMS that arrived, -1 = none
};

class IPhone
{
  public:
    virtual ~IPhone() = default;

    virtual PhoneStatus getStatus() = 0;
    virtual bool dial(const char *number) = 0;
    virtual bool answer() = 0;
    virtual bool hangUp() = 0;

    static void set(IPhone *phone);
    static IPhone *get();
};
