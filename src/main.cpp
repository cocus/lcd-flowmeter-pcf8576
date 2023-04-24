#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>

// the following code was lifted from https://github.com/OBrown92/LX_Lcd_Vario
// but it's a WIP

// Continue and Last Bit
#define CONTINUE 128 // Command continuation Bit
#define LAST 0       // Last Command

// MODE SET
#define MODESET 64 // Mode Set Command

#define MODE_NORMAL 0 // Display Mode
#define MODE_POWERSAVING 16

#define DISPLAY_DISABLED 0 // Display On/Off
#define DISPLAY_ENABLED 8

#define BIAS_THIRD 0 // BIAS Mode
#define BIAS_HALF 4

#define DRIVE_STATIC 1 // Drive Mode
#define DRIVE_2 2
#define DRIVE_3 3
#define DRIVE_4 0

// BLINK
#define BLINK 112 // Blink Command

#define BLINKING_NORMAL 0 // Blink Mode
#define BLINKING_ALTERNATION 4

#define BLINK_FREQUENCY_OFF 0 // Blink Frequency
#define BLINK_FREQUENCY2 1
#define BLINK_FREQUENCY1 2
#define BLINK_FREQUENCY05 3

// LOADDATAPOINTER
#define LOADDATAPOINTER 0x00 // Data pointer start address

// BANK SELECT
#define BANKSELECT 120 // Bank Select Command

#define BANKSELECT_O1_RAM0 0
#define BANKSELECT_O1_RAM2 2

#define BANKSELECT_O2_RAM0 0
#define BANKSELECT_O2_RAM2 1

// Device Select, not really sure how to implement best,
// because you can combine these commands to address 8 PCF
#define DEVICE_SELECT 96 // Device Select Command
#define DEVICE_SELECT_A2 4
#define DEVICE_SELECT_A1 2
#define DEVICE_SELECT_A0 1 // A2,A1,A0 = B111

void pcf_sendbuff(const uint8_t* buff)
{
    Wire.beginTransmission(0x38);
	Wire.write(CONTINUE | DEVICE_SELECT); //select the first PCF, assume that the first is the device with 000, maybe implement another logic
	Wire.write(LOADDATAPOINTER);
	for (int i=0; i < 1 * 20; i++)
    {
		Wire.write(buff[i]);
	}
	Wire.endTransmission();
}

void pcf_clear()
{
    //works only for one Hardware Adress for now, have to implement the logic for two
    Wire.beginTransmission(0x38);
	Wire.write(CONTINUE | DEVICE_SELECT); //select the first PCF, assume that the first is the device with 000, maybe implement another logic
	Wire.write(LOADDATAPOINTER);
	for (int i=0; i < 1 * 20; i++)
    {
		Wire.write(0x00);
	}
	Wire.endTransmission();
}

void pcf_fire()
{
    //works only for one Hardware Adress for now, have to implement the logic for two
    Wire.beginTransmission(0x38);
	Wire.write(CONTINUE | DEVICE_SELECT); //select the first PCF, assume that the first is the device with 000, maybe implement another logic
	Wire.write(LOADDATAPOINTER);
	for (int i=0; i < 1 * 20; i++)
    {
		Wire.write(0xff);
	}
	Wire.endTransmission();
}

// (NOTE: this is not the right mode for the PCF of this device, but it "works", WIP)
byte set_modeset = MODESET | MODE_NORMAL | DISPLAY_ENABLED | BIAS_THIRD | DRIVE_3;
byte set_blink = BLINK | BLINKING_ALTERNATION | BLINK_FREQUENCY_OFF;
byte set_datapointer = LOADDATAPOINTER | 0;
byte set_bankselect = BANKSELECT | BANKSELECT_O1_RAM0 | BANKSELECT_O2_RAM0; //doesn't really matter, because Drive mode is 4
byte set_deviceselect_1 = DEVICE_SELECT; //A0, A1, A2 set to ground
byte set_deviceselect_2 = DEVICE_SELECT | DEVICE_SELECT_A0; //A0 is high


void pcf_init()
{
    //init all added PCF's, don't know exactly if you have to init all
    for (uint8_t pcf = 0; pcf < 1; pcf++){
        Wire.beginTransmission(0x38);
        Wire.write(CONTINUE | set_modeset); //modeset
        Wire.write(CONTINUE | set_blink); //blink
        Wire.write(LAST | set_deviceselect_2); //devSel
        //Wire.write(LAST | settings[pcf][4]); //banksel
        Wire.endTransmission();
    }
}

void setup()
{
    Wire.begin(); // join i2c bus (address optional for master)
    Serial.begin(9600);
    while (!Serial)
        ; // Leonardo: wait for serial monitor
    pcf_init();
    Serial.println("PCF init!");
    pcf_clear();
}

struct
{
    uint8_t ch;
    uint8_t display;
    uint8_t dp;
} display[8] = { 0 };

const uint8_t seven_gbdfdpeac[] =
{
    0x88, 
    0xBE, 
    0x19, 
    0x1C, 
    0x2E, 
    0x4C, 
    0x48, 
    0xBC, 
    0x08, 
    0x0C, 
    0x28, 
    0x4A, 
    0xC9, 
    0x1A, 
    0x49, 
    0x69
};

void pcf_update()
{
    uint8_t buffer[20] = { 0 };

    // starts at 5
    for (size_t i = 0; i < 8; i++)
    {
        uint8_t sevenseg = ~seven_gbdfdpeac[display[i].ch - '0'];
        // g    b    d    f    dp   e    a    c
        // 0x80 0x40 0x20 0x10 0x08 0x04 0x02 0x01
        uint8_t b = (sevenseg & 0x40) ? 0x80 : 0;
        b |= (sevenseg & 0x01) ? 0x40 : 0;
        b |= (display[i].dp) ? 0x20 : 0;
        b |= (sevenseg & 0x02) ? 0x10 : 0;
        b |= (sevenseg & 0x80) ? 0x08 : 0;
        b |= (sevenseg & 0x20) ? 0x04 : 0;
        b |= (sevenseg & 0x10) ? 0x02 : 0;
        b |= (sevenseg & 0x04) ? 0x01 : 0;
        // b    c    dp   a    g    d    f    e
        // 0x80 0x40 0x20 0x10 0x08 0x04 0x02 0x01
        //Serial.print("Sevenseg is "); Serial.print(sevenseg, HEX); Serial.print(", b is "); Serial.println(b, HEX);
        buffer[i + 5] = display[i].display ? b : 0;
    }

    pcf_sendbuff(buffer);
}


void loop()
{
    char ascii[30] = { '\0' };
    utoa(millis(), ascii, 10);
    Serial.print("Sending ");
    Serial.println(ascii);

    size_t offset = 0;
    for (size_t i = 0; i < 8; i++)
    {
        if (ascii[i] == '\0')
        {
            if (i < 8)
            {
                offset = i;
            }
            break;
        }
    }
    for (size_t i = 0; i < 8; i++)
    {
        if (8 - offset + i > 7)
        {
            break;
        }
        if (ascii[i] == '\0')
        {
            display[8 - offset + i].display = 0;
            break;
        }
        display[8 - offset + i].ch = ascii[i];
        display[8 - offset + i].display = 1;
    }
    pcf_update();
    /*
    if (Serial.available())
    {
        Serial.read();
        buffer[wrptr] = counter;
        Serial.print("Adding ");
        Serial.print(counter, HEX);
        Serial.print(" at pos ");
        Serial.println(wrptr);
        counter++;
        if (counter == 0)
        {
            if (++wrptr == 20)
            {
                wrptr = 0;
            }
        }

        pcf_sendbuff(buffer);
    }*/

    /*Serial.println("PCF loop!");
    pcf_clear();
    delay(1000);
    pcf_fire();
    delay(5000); // wait 5 seconds for next scan*/


    delay(100);
}
