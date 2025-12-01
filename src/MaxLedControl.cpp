/*
 *    MaxLedControl.cpp - A library for controling Leds with a MAX7219/MAX7221
 *    Copyright (c) 2007 Eberhard Fahle
 * 
 *    Permission is hereby granted, free of charge, to any person
 *    obtaining a copy of this software and associated documentation
 *    files (the "Software"), to deal in the Software without
 *    restriction, including without limitation the rights to use,
 *    copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following
 *    conditions:
 * 
 *    This permission notice shall be included in all copies or 
 *    substantial portions of the Software.
 * 
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *    OTHER DEALINGS IN THE SOFTWARE.
 */


#include "MaxLedControl.h"

LedControl::LedControl(int dataPin, int clkPin, int csPin, int numDevices)
    : Adafruit_GFX(numDevices * 8, 8) {
    SPI_MOSI=dataPin;
    SPI_CLK=clkPin;
    SPI_CS=csPin;
    maxDevices = (numDevices < 1) ? 1 : numDevices;
    hardwareSPI = false;

    pinMode(SPI_MOSI, OUTPUT);
    pinMode(SPI_CLK, OUTPUT);
    pinMode(SPI_CS, OUTPUT);
    digitalWrite(SPI_CS, HIGH);

    status = new byte[maxDevices * 8];
    memset(status, 0, maxDevices * 8);

    // Initialise HW to safe state
    for (int i = 0; i < maxDevices; i++) {
        spiTransfer(i, OP_DISPLAYTEST, 0);
        setScanLimit(i, 7);
        spiTransfer(i, OP_DECODEMODE, 0);
        clearDisplay(i);
        shutdown(i, true);
    }
}

LedControl::LedControl(int csPin, int numDevices, SPIClass *spiClass)
    : Adafruit_GFX(numDevices * 8, 8) {
    SPI_CS = csPin;
    maxDevices = (numDevices < 1) ? 1 : numDevices;
    hardwareSPI = true;
    spi_bus = spiClass;

    pinMode(SPI_CS, OUTPUT);
    digitalWrite(SPI_CS, HIGH);

    spi_bus->begin();

    status = new byte[maxDevices * 8];
    memset(status, 0, maxDevices * 8);

    // Initialise HW to safe state
    for (int i = 0; i < maxDevices; i++) {
        spiTransfer(i, OP_DISPLAYTEST, 0);
        setScanLimit(i, 7);
        spiTransfer(i, OP_DECODEMODE, 0);
        clearDisplay(i);
        shutdown(i, true);
    }
}

LedControl::~LedControl() {
    if (status) {
        delete[] status;
    }
}

void LedControl::begin(int intensity) {
    for (int addr = 0; addr < getDeviceCount(); addr++) {
        shutdown(addr, false);
        setIntensity(addr, intensity);
        clearDisplay(addr);
    }
}

int LedControl::getDeviceCount() {
    return maxDevices;
}

void LedControl::shutdown(int addr, bool b) {
    if(addr<0 || addr>=maxDevices) return;
    spiTransfer(addr, OP_SHUTDOWN, b ? 0 : 1);
}

void LedControl::setScanLimit(int addr, int limit) {
    if(addr<0 || addr>=maxDevices) return;
    if(limit>=0 && limit<8)
        spiTransfer(addr, OP_SCANLIMIT, limit);
}

void LedControl::setIntensity(int addr, int intensity) {
    if(addr<0 || addr>=maxDevices) return;
    if(intensity>=0 && intensity<16)    
        spiTransfer(addr, OP_INTENSITY, intensity);
}

void LedControl::clearDisplay(int addr) {
    if(addr<0 || addr>=maxDevices) return;
    int offset = addr * 8;
    for(int i=0; i<8; i++) {
        status[offset+i] = 0;
        spiTransfer(addr, i+1, status[offset+i]);
    }
}

void LedControl::clear() {
    for(int i=0; i<maxDevices; i++)
        clearDisplay(i);
}

void LedControl::setLed(int addr, int row, int column, boolean state) {
    int offset;
    byte val=0x00;

    if(addr<0 || addr>=maxDevices)
        return;
    if(row<0 || row>7 || column<0 || column>7)
        return;
    offset=addr*8;
    val=B10000000 >> column;
    if(state)
        status[offset+row]=status[offset+row]|val;
    else {
        val=~val;
        status[offset+row]=status[offset+row]&val;
    }
    spiTransfer(addr, row+1,status[offset+row]);
}

void LedControl::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return;  // Out of bounds
    }
    // Set or clear the pixel based on the color (non-zero color sets the pixel)
    setLed(0, y, x, (color > 0));
}

void LedControl::setRow(int addr, int row, byte value) {
    int offset;
    if(addr<0 || addr>=maxDevices)
        return;
    if(row<0 || row>7)
        return;
    offset=addr*8;
    status[offset+row]=value;
    spiTransfer(addr, row+1,status[offset+row]);
}

void LedControl::setColumn(int addr, int col, byte value) {
    byte val;

    if(addr<0 || addr>=maxDevices)
        return;
    if(col<0 || col>7) 
        return;
    for(int row=0;row<8;row++) {
        val=value >> (7-row);
        val=val & 0x01;
        setLed(addr,row,col,val);
    }
}

void LedControl::setDigit(int addr, int digit, byte value, boolean dp) {
    int offset;
    byte v;

    if(addr<0 || addr>=maxDevices)
        return;
    if(digit<0 || digit>7 || value>15)
        return;
    offset=addr*8;
    v=pgm_read_byte_near(charTable + value); 
    if(dp)
        v|=B10000000;
    status[offset+digit]=v;
    spiTransfer(addr, digit+1,v);
}

void LedControl::setChar(int addr, int digit, char value, boolean dp) {
    int offset;
    byte index,v;

    if(addr<0 || addr>=maxDevices)
        return;
    if(digit<0 || digit>7)
        return;
    offset=addr*8;
    index=(byte)value;
    if(index >127) {
        //no defined beyond index 127, so we use the space char
        index=32;
    }
    v=pgm_read_byte_near(charTable + index); 
    if(dp)
        v|=B10000000;
    status[offset+digit]=v;
    spiTransfer(addr, digit+1,v);
}

void LedControl::spiTransfer(int addr, volatile byte opcode, volatile byte data) {
    if(addr < 0 || addr >= maxDevices) return;

    // MAX7219 max freq is 10MHz. 
    // We use transaction to coexist with SD cards/Sensors on same bus.
    if (hardwareSPI) {
        spi_bus->beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    }
    
    digitalWrite(SPI_CS, LOW);

    // Logic: The MAX7219 works like a shift register chain.
    // If we have 4 devices, and we want to talk to device #1 (index 1),
    // we must send data for: Dev 3 (NoOp), Dev 2 (NoOp), Dev 1 (CMD), Dev 0 (NoOp).
    
    for(int i = maxDevices - 1; i >= 0; i--) {
        if (i == addr) {
            // Send actual command
            if (hardwareSPI) {
                spi_bus->transfer(opcode);
                spi_bus->transfer(data);
            } else {
                shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, opcode);
                shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, data);
            }
        } else {
            // Send No-Op to pass through to other chips
            if (hardwareSPI) {
                spi_bus->transfer(OP_NOOP);
                spi_bus->transfer(0);
            } else {
                shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, OP_NOOP);
                shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, 0);
            }
        }
    }

    digitalWrite(SPI_CS, HIGH);

    if (hardwareSPI) {
        spi_bus->endTransaction();
    }
}    
