#include <stdint.h>
#include <cassert>
#include <cstdio>

uint32_t readVleUInt32_2(const char *&buffer)
{
    const unsigned char *bytes =
        reinterpret_cast<const unsigned char*>(buffer);
    unsigned char byte1 = buffer[0];

    uint32_t bias = 0;
    if ((byte1 & 0x80) == 0) {
        buffer++;
        return byte1 - 1;
    }
    bias += 127;

    if ((byte1 & 0xC0) == 0x80) {
        uint32_t result =
                (byte1 ^ 0x80) * 255 +
                (bytes[1] - 1) +
                bias;
        buffer += 2;
        return result;
    }
    bias += 255 * (1 << 6);

    if ((byte1 & 0xE0) == 0xC0) {
        uint32_t result =
                (byte1 ^ 0xC0) * 255 * 255 +
                (bytes[1] - 1) * 255 +
                (bytes[2] - 1) +
                bias;
        buffer += 3;
        return result;
    }
    bias += 255 * 255 * (1 << 5);

    if ((byte1 & 0xF0) == 0xE0) {
        uint32_t result =
                (byte1 ^ 0xE0) * 255 * 255 * 255 +
                (bytes[1] - 1) * 255 * 255 +
                (bytes[2] - 1) * 255 +
                (bytes[3] - 1) +
                bias;
        buffer += 4;
        return result;
    }
    bias += 255 * 255 * 255 * (1 << 4);

    uint32_t result =
            (byte1 ^ 0xF0) * 255 * 255 * 255 * 255 +
            (bytes[1] - 1) * 255 * 255 * 255 +
            (bytes[2] - 1) * 255 * 255 +
            (bytes[3] - 1) * 255 +
            (bytes[4] - 1) +
            bias;
    buffer += 5;
    return result;
}

void writeVleUInt32_2(char *&buffer, uint32_t value)
{
    if (value < 127) {
        *buffer++ = value + 1;
        return;
    }
    value -= 127;

#define WRITE_BYTE(i)                   \
    do {                                \
        uint32_t temp = value / 255;    \
        buffer[i] = (value % 255) + 1;  \
        value = temp;                   \
    } while(0)

    if (value < 255 * (1 << 6)) {
        WRITE_BYTE(1);
        buffer[0] = value | 0x80;
        buffer += 2;
        return;
    }
    value -= 255 * (1 << 6);
    if (value < 255 * 255 * (1 << 5)) {
        WRITE_BYTE(2);
        WRITE_BYTE(1);
        buffer[0] = value | 0xC0;
        buffer += 3;
        return;
    }
    value -= 255 * 255 * (1 << 5);
    if (value < 255 * 255 * 255 * (1 << 4)) {
        WRITE_BYTE(3);
        WRITE_BYTE(2);
        WRITE_BYTE(1);
        buffer[0] = value | 0xE0;
        buffer += 4;
        return;
    }
    value -= 255 * 255 * 255 * (1 << 4);
    WRITE_BYTE(4);
    WRITE_BYTE(3);
    WRITE_BYTE(2);
    WRITE_BYTE(1);
    buffer[0] = value | 0xF0;
    buffer += 5;
    return;

#undef WRITEBYTE
}



uint32_t readVleUInt32(const char *&buffer)
{
    uint32_t result = 0;
    unsigned char nibble;
    int bit = 0;
    do {
        nibble = static_cast<unsigned char>(*(buffer++));
        result |= (nibble & 0x7F) << bit;
        bit += 7;
    } while ((nibble & 0x80) == 0x80);
    return result;
}

// This encoding of a uint32 is guaranteed not to contain NUL bytes unless the
// value is zero.
void writeVleUInt32(char *&buffer, uint32_t value)
{
    do {
        unsigned char nibble = value & 0x7F;
        value >>= 7;
        if (value != 0)
            nibble |= 0x80;
        *buffer++ = nibble;
    } while (value != 0);
}

unsigned char tmp[64];
uint32_t i = 0;

int main()
{
    do {
        {
            char *ptmp = (char*)tmp;
            writeVleUInt32_2(ptmp, i);
            *ptmp = '\0';
        }
        {
            const char *preadtmp = (const char*)tmp;
            uint32_t result = readVleUInt32_2(preadtmp);
            assert(*preadtmp == '\0');
            if (result != i)
                printf("%02X %02X %02X %02X\n",
                       tmp[0], tmp[1], tmp[2], tmp[3]);
            assert(result == i);
        }

        if (i % 0x100000 == 0)
            printf("%08x\n", i);

    } while (++i != 0);
}
