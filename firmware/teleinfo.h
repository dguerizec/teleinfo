#ifndef __TELEINFO_H__
#define __TELEINFO_H__
#include "application.h"

#define FRAME_START 0x02
#define FRAME_END   0x03
#define FRAME_MAX_LEN       1024
#define FRAME_READ_TIMEOUT  2000

class Teleinfo {
    private:
        Stream *stream;
        bool verbose;
        String *frame;
        String *buffer;
        String *output;
        char *items[64];
        int idx;
        uint32_t tmout_us;
        void parse_frame();
    public:
        Teleinfo(Stream &s, uint32_t timeout_us=2000000L, bool verbose=0);
        ~Teleinfo();
        void read_frame();
        void read_byte();
        void loop();
        const char *get_next();
};


#endif // __TELEINFO_H__
