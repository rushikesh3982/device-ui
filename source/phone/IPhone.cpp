#include "phone/IPhone.h"

// T-Deck phone: the phone the firmware registered, if any.
static IPhone *registeredPhone = nullptr;

void IPhone::set(IPhone *phone)
{
    registeredPhone = phone;
}

IPhone *IPhone::get()
{
    return registeredPhone;
}
