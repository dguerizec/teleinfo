#include "teleinfo/teleinfo.h"

#define EVERY(T, that)                              \
do {                                                \
    uint32_t interval = (uint32_t)T;                \
    static uint32_t last = millis() - (interval);   \
    uint32_t now = millis();                        \
    if(now - last > (interval)) {                   \
        do { that; } while(0);                      \
        last = now;                                 \
    }                                               \
} while(0)


Teleinfo teleinfo(Serial1, 1);

void setup() {
    Serial1.begin(1200);
}

void serialEvent1() {
    teleinfo.read_byte();
}

void send_teleinfo() {
    const char *key;
    const char *value;

    while((key = teleinfo.get_next())) {
        value = teleinfo.get_next();
        Particle.publish(String::format("edf/%s", key), value);
    }
}


void loop() {
    teleinfo.loop();
    EVERY(5000, /* milliseconds */
    /* DO */ send_teleinfo());
}
