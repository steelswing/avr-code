#include "Arduino.h"
#include "pins_arduino.h"


#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "HIDPrivate.h"
#include "Wire.h"

#define HIDSERIAL_INBUFFER_SIZE 32
#define OUTBUFFER_SIZE 8

static uchar outBuffer[OUTBUFFER_SIZE];
static uchar inBuffer[HIDSERIAL_INBUFFER_SIZE];

static uchar received = 0;
static uchar reportId = 0;
static uchar bytesRemaining;
static uchar* pos;

PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {/* USB report descriptor */
    0x06, 0x00, 0xff, // USAGE_PAGE (Generic Desktop)
    0x09, 0x01, // USAGE (Vendor Usage 1)
    0xa1, 0x01, // COLLECTION (Application)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x95, 0x08, //   REPORT_COUNT (8)
    0x09, 0x00, //   USAGE (Undefined)  
    0x82, 0x02, 0x01, //   INPUT (Data,Var,Abs,Buf)
    0x95, HIDSERIAL_INBUFFER_SIZE, //   REPORT_COUNT (32)
    0x09, 0x00, //   USAGE (Undefined)        
    0xb2, 0x02, 0x01, //   FEATURE (Data,Var,Abs,Buf)
    0xc0 // END_COLLECTION
};

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionRead(uchar *data, uchar len) {
    return 0;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar usbFunctionWrite(uchar *data, uchar len) {
    if (reportId == 0) {
        int i;
        if (len > bytesRemaining)
            len = bytesRemaining;
        bytesRemaining -= len;
        //int start = (pos==inBuffer)?1:0;
        for (i = 0; i < len; i++) {
            if (data[i] != 0) {
                *pos++ = data[i];
            }
        }
        if (bytesRemaining == 0) {
            received = 1;
            *pos++ = 0;
            return 1;
        } else {
            return 0;
        }
    } else {
        return 1;
    }
}

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (usbRequest_t *) data;
    reportId = rq->wValue.bytes[0];
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) { /* HID class request */
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* since we have only one report type, we can ignore the report-ID */
            return USB_NO_MSG; /* use usbFunctionRead() to obtain data */
        } else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
            /* since we have only one report type, we can ignore the report-ID */
            pos = inBuffer;
            bytesRemaining = rq->wLength.word;
            if (bytesRemaining > sizeof (inBuffer))
                bytesRemaining = sizeof (inBuffer);
            return USB_NO_MSG; /* use usbFunctionWrite() to receive data from host */
        }
    } else {
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}


// write one character

size_t write(uint8_t data) {
    while (!usbInterruptIsReady()) {
        usbPoll();
    }
    memset(outBuffer, 0, OUTBUFFER_SIZE);
    outBuffer[0] = data;
    usbSetInterrupt(outBuffer, OUTBUFFER_SIZE);
    return 1;
}
// write up to 8 characters

size_t write8(const uint8_t *buffer, size_t size) {
    unsigned char i;
    while (!usbInterruptIsReady()) {
        usbPoll();
    }
    memset(outBuffer, 0, OUTBUFFER_SIZE);
    for (i = 0; i < size && i < OUTBUFFER_SIZE; i++) {
        outBuffer[i] = buffer[i];
    }
    usbSetInterrupt(outBuffer, OUTBUFFER_SIZE);
    return (i);
}

uchar available() {
    return received;
}

uchar read(uchar *buffer) {
    if (received == 0) return 0;
    int i;
    for (i = 0; inBuffer[i] != 0 && i < HIDSERIAL_INBUFFER_SIZE; i++) {
        buffer[i] = inBuffer[i];
    }
    inBuffer[0] = 0;
    buffer[i] = 0;
    received = 0;
    return i;
}

// write a string

size_t write(const uint8_t *buffer, size_t size) {
    size_t count = 0;
    unsigned char i;
    for (i = 0; i < (size / OUTBUFFER_SIZE) + 1; i++) {
        count += write8(buffer + i * OUTBUFFER_SIZE, (size < (count + OUTBUFFER_SIZE)) ? (size - count) : OUTBUFFER_SIZE);
    }
    return count;
}

size_t write(const char *str) {
    return write((const uint8_t *) str, strlen(str));
}

size_t printNumber(unsigned long n, uint8_t base) {
    char buf[8 * sizeof (long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof (buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if (base < 2) base = 10;

    do {
        unsigned long m = n;
        n /= base;
        char c = m - base * n;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);

    return write(str);
}

#define DEVICE1_PAGE1 0x50
#define DEVICE1_PAGE2 0x51
#define DEVICE1_PAGE3 0x52

unsigned char buffer[32];

void EEPROM_WriteByte(byte dev, byte address, byte data) {
    Wire.beginTransmission(dev);
    Wire.write(address);
    Wire.write(data);
    Wire.endTransmission();
//    delay(5); //так нахуй надо
}

void EEPROM_WriteByte(byte dev, int address, byte data) {
    Wire.beginTransmission(dev);
    Wire.write(address);

    for (int i = 0; i < 256; i++) {
        Wire.write(i);

    }
    //    Wire.write(0x2);
    //    Wire.write(0x3);
    //    Wire.write(0x4);
    delay(5); //так нахуй надо
    Wire.endTransmission();
}

/*
 Returns number of bytes read from device

 Due to buffer size in the Wire library, don't read more than 30 bytes
 at a time!  No checking is done in this function.

 TODO: Change length to int and make it so that it does repeated
 EEPROM reads for length greater than 30.
*/
int eeprom_read_buffer(byte deviceaddr, unsigned eeaddr, byte * buffer, byte length) {
    // Three lsb of Device address byte are bits 8-10 of eeaddress
    byte devaddr = deviceaddr | ((eeaddr >> 8) & 0x07);
    byte addr = eeaddr;

    Wire.beginTransmission(devaddr);
    Wire.write(int(addr));
    Wire.endTransmission();
    Wire.requestFrom(devaddr, length);
    int i;
    for (i = 0; i < length && Wire.available(); i++) {
        buffer[i] = Wire.read();
    }
    return i;
}

uint8_t EEPROM_ReadByte(byte deviceaddr, unsigned eeaddr) {
    byte devaddr = deviceaddr | ((eeaddr >> 8) & 0x07);
    byte addr = eeaddr;

    Wire.beginTransmission(deviceaddr);
    Wire.write(addr);
    Wire.endTransmission();
    Wire.requestFrom(deviceaddr, 1);
    int rdata = 0x0;
    if (Wire.available()) {
        rdata = Wire.read();
    }

    return rdata;
}

void setup() {
    cli(); // Запрет работы прерываний	
    usbDeviceDisconnect(); // Ложный дисконнект usb
    _delay_ms(250); // Небольшая задержка
    usbDeviceConnect(); // Коннект usb 
    usbInit(); // Инициализация usb 
    sei(); // Разрешение работы прерываний
    received = 0;

    Wire.begin();

    // clear
    //    for (int i = 0; i < 256; i++) {
    //        EEPROM_WriteByte(DEVICE1_PAGE1, i, 0x0);
    //    }

    //    int devPageId = 0b01010000;
    //    for (int j = 0; j < 4; j++) {
    //        for (int i = 0; i < 256; i += 10) {
    //            EEPROM_WriteByte(devPageId, i + 0, '1');
    //            EEPROM_WriteByte(devPageId, i + 1, '2');
    //            EEPROM_WriteByte(devPageId, i + 2, '3');
    //            EEPROM_WriteByte(devPageId, i + 3, '4');
    //            EEPROM_WriteByte(devPageId, i + 4, '5');
    //            EEPROM_WriteByte(devPageId, i + 5, '6');
    //            EEPROM_WriteByte(devPageId, i + 6, '7');
    //            EEPROM_WriteByte(devPageId, i + 7, '8');
    //            EEPROM_WriteByte(devPageId, i + 8, '9');
    //            EEPROM_WriteByte(devPageId, i + 9, '_');
    //        }
    //        for (int i = 0; i < 25; i++) {
    //            EEPROM_WriteByte(devPageId, 231 + i, '*');
    //        }
    //        devPageId++;
    //    }
}

uint64_t sended = 0;
uint8_t buff[8];
int devPageId = 0b01010000;
int recivedId = 0;
bool starting = false;
bool writing = false;

void loop() {
    //    int readed = EEPROM_ReadByte(DEVICE_1, 32);
    //    String str = "hello uebishe: ";
    //    str += (char) 103;
    //    str += "\n";
    //    write(str.c_str());
    if (available() > 0) {
        int size = read(buffer);
        if (size != 0) {
            if (
                    buffer[0] == 'G' &&
                    buffer[1] == 'E' &&
                    buffer[2] == 'T' &&
                    buffer[3] == ';') {
                starting = true;
                sended = 0;
                devPageId = 0b01010000;
                recivedId = 0;
            } else if (
                    buffer[0] == 'W' &&
                    buffer[1] == 'R' &&
                    buffer[2] == 'T' &&
                    buffer[3] == ';') {
                writing = true;
                devPageId = 0b01010000;
                recivedId = 0;
                write("DONE_WRT;");
            } else if (
                    buffer[0] == 'W' &&
                    buffer[1] == 'R' &&
                    buffer[2] == 'D' &&
                    buffer[3] == ';') {
                EEPROM_WriteByte(devPageId, recivedId, buffer[4]);

                recivedId++;
                if (recivedId != 0 && recivedId % 256 == 0) {
                    devPageId++;
                }
                write("DONEW;");
                usbPoll();
            }
        }
    }
    if (starting) {
        for (int i = 0; i < 8; i++) {
            buff[i] = EEPROM_ReadByte(devPageId, sended + i);
        }

        write8(buff, 8);

        if (sended >= 1024) {
            write("END;");
            usbPoll();
            sended = 0;
            devPageId = 0b01010000;
            for (int i = 0; i < 8; i++) {
                buff[i] = 0x0;
            }
        } else {
            sended += 8;
            if (sended % 256 == 0) {
                devPageId++;
            }
        }
    }

    write('&');
    usbPoll();
}
