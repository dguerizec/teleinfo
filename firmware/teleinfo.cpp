#include "teleinfo.h"
#include "Xray/Xray.h"

Teleinfo::Teleinfo(Stream &s, uint32_t timeout_us, bool verbose) {
    stream = &s;
    verbose = true;
    idx = 0;
    items[0] = NULL;

    frame = NULL;
    buffer = NULL;
    output = NULL;
    tmout_us = timeout_us;
}

Teleinfo::~Teleinfo() {
}

void Teleinfo::loop() {
    if(frame) {
        parse_frame();
    //} else {
        //read_frame();
    }
}

void Teleinfo::parse_frame() {
#define S_KEY 0
#define S_VAL 1
#define S_CKS 2
#define S_ERR 4
    int state = S_KEY;
    int x = 0;
    int sum = 0;
    int s;

    if(!frame) {
        return;
    }
    //Xray.publish("parse_frame", String(frame->length()));

    char *p = (char *)frame->c_str();
    String err = String("");

    while(*p) {
        while(*p == '\n' || *p == '\r') {
            p++;
        }
        if(!*p) {
            continue;
        }
        switch(state) {
            case S_KEY:
            case S_VAL:
                items[x++] = p;
                while(*p != ' ' && *p) {
                    sum += *p;
                    p++;
                }
                if(*p == ' ') {
                    *p = 0;
                    p++;
                    state++;
                } else {
                    if(state == S_KEY) {
                        err = String("missing space after key");
                    } else {
                        err = String("missing space after value");
                    }
                    state = S_ERR;
                }
                break;
            case S_CKS:
                sum = ((sum + 1*32) & 63) + 32;
                s = (int)*p;
                p++;
                //Xray.publish(items[x-2], String::format("<%s> <%s> %02x %02x",
                //            items[x-2], items[x-1], s, sum));
                if(s != sum || (*p != '\n' && *p != '\r' && *p)) {
                    if(s != sum) {
                        err = String("checksum error ");
                    } else if(*p != '\n' && *p != '\r' && *p) {
                        err = String("extra char after checksum:")+*p;
                    }
                    state = S_ERR;
                    break;
                }
                state = S_KEY;
                sum = 0;
                break;
            case S_ERR:
                // drop the frame altogether
                Xray.publish("parse_frame_error", String(err) + " " + x);
                delete frame;
                items[0] = NULL;
                frame = NULL;
                return;

        }
        if(x > 64 - 2) {
            err = String("Too many items");
            state = S_ERR;
        } else {
            items[x] = NULL;
            items[x+1] = NULL;
        }
    }
    idx = 0;
    if(output) {
        delete output;
    }
    output = frame;
    frame = NULL;
    //Xray.publish("parse_frame_end", String::format("%p %p", items[0], items[1]));
}

static uint32_t frame_time = 0;
static uint32_t bytes_time = 0;

void Teleinfo::read_byte() {
    uint32_t start = micros();
    while(stream->available()) {
        if(micros() - start < tmout_us) {
            if(buffer) {
                delete buffer;
                buffer = NULL;
            }
            break;
        }
        char c = stream->read() & 0x7F;
        if(!buffer) {
            if(c == FRAME_START) {
                frame_time = millis();
                bytes_time = 0;
                // create buffer at frame start
                buffer = new String();
                buffer->reserve(256);
            }
        } else if(c == FRAME_END) {
            // switch buffer -> frame
            if(frame) {
                delete frame;
            }
            frame = buffer;
            buffer = NULL;
            bytes_time += micros() - start;
            frame_time = millis() - frame_time;
            //Xray.publish("time/frame", frame_time);
            //Xray.publish("time/bytes", bytes_time/1000.);
        } else {
            // add the byte to the buffer
            *buffer += c;
        }
    }
    bytes_time += micros() - start;
    RGB.control(false);
}

void Teleinfo::read_frame() {
    //uint32_t start = millis();
    int n = 0;
    char c;
    if(!stream->available()) {
        goto end;
    }

    c = stream->read() & 0x7F;
    n++;

    if(!buffer) {
        while(c != FRAME_START) {
            if(!stream->available()) {
                continue;
                goto end;
            }
            c = stream->read() & 0x7F;
            n++;
        }

        buffer = new String();
        buffer->reserve(256);
    }

    //Xray.publish("read_frame start", String::format("%d %lu", n, (millis() - start)));
    RGB.control(true);
    RGB.color(255, 255, 255);
    while(c != FRAME_END) {
        if(stream->available()) {
            c = stream->read() & 0x7F;
            n++;
            if(c == FRAME_END) {
                if(frame) {
                    items[0] = NULL;
                    idx = 0;
                    delete frame;
                }
                frame = buffer;
                buffer = NULL;
                //Xray.publish("read_frame done", *frame); //String(frame->length()));
                //Xray.publish("read_frame size", String(frame->length()));
                //Xray.publish("read_frame time", String::format("%d %lu", n, (millis() - start)));
                goto end;
            }
            *buffer += c;
        }
    }
end:
    RGB.control(false);
    return;
}

const char *Teleinfo::get_next() {
    char *item = items[idx];
    if(item) {
        while(*item && *item == '0') {
            // Drop leading zeroes
            item++;
        }
        if(!*item && *items[idx]) {
            // go back to leave only one zero
            item--;
        }
        idx++;
    } else {
        idx = 0;
    }
    return item;
}
