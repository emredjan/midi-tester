#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <MIDI.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

const uint8_t ROTARY_ENCODER_A_PIN = 13;
const uint8_t ROTARY_ENCODER_B_PIN = 14;
const uint8_t ROTARY_ENCODER_BUTTON_PIN = 27;
const uint8_t ROTARY_ENCODER_VCC_PIN = -1;

const uint8_t DUMMY_ENCODER_A_PIN = -1;
const uint8_t DUMMY_ENCODER_B_PIN = -1;
const uint8_t DUMMY_ENCODER_VCC_PIN = -1;
const uint8_t BUTTON_SEND_PIN = 36;

AiEsp32RotaryEncoder encoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN);
AiEsp32RotaryEncoder buttonSend = AiEsp32RotaryEncoder(DUMMY_ENCODER_A_PIN, DUMMY_ENCODER_B_PIN, BUTTON_SEND_PIN, DUMMY_ENCODER_VCC_PIN);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

enum encoderPage
{
    pageCmdType,
    pageChannel,
    pageCcNum,
    pageCmdValue
};

encoderPage PAGE = pageCmdType;

uint8_t midiCmdType = 0;  // 0: PC, 1: CC
uint8_t midiChannel = 1;  // 1-16
uint8_t midiCcNum = 11;   // 0-127
uint8_t midiCmdValue = 0; // 0 - 127

int16_t encoderValue;

String msg = "";
String page = "";

String statusCmdType = "";
String statusChannel = "";
String statusCcNum = "";
String statusValue = "";

void setOled(const char *msg, const char *page);
void rotary_onButtonClick();
void buttonSendClick();
void rotary_loop();

void setup()
{

    Serial.begin(115200);
    MIDI.begin(MIDI_CHANNEL_OMNI);

    encoder.begin();
    encoder.setup([] { encoder.readEncoder_ISR(); });

    buttonSend.begin();
    buttonSend.setup([] { buttonSend.readEncoder_ISR(); });

    // Initial Page
    PAGE = pageCmdType;
    encoder.setBoundaries(0, 1, false);

    display.begin();
    setOled("READY", page.c_str());
}

void loop()
{

    rotary_loop();

    delay(10);
    if (millis() > 10000)
    {
        encoder.enable();
        buttonSend.enable();
    }
}

void setOled(const char *msg, const char *page)
{
    display.clearBuffer();

    int statusIndicator = 12;

    switch (PAGE)
    {
    case pageCmdType:
        statusIndicator = 12;
        break;
    case pageChannel:
        statusIndicator = 40;
        break;
    case pageCcNum:
        statusIndicator = 73;
        break;
    case pageCmdValue:
        statusIndicator = 102;
        break;
    }

    display.setFont(u8g2_font_open_iconic_arrow_1x_t);
    display.drawStr(statusIndicator, 9, "P");

    if (midiCmdType == 0)
    {
        statusCmdType = "PC";
        statusCcNum = "";
    }
    else if (midiCmdType == 1)
    {
        statusCmdType = "CC";
        statusCcNum = (midiCcNum < 100) ? "0" : "";
        statusCcNum += (midiCcNum < 10) ? "0" : "";
        statusCcNum += String(midiCcNum);
    }

    statusChannel = (midiChannel < 10) ? "0" : "";
    statusChannel += String(midiChannel);
    statusValue = (midiCmdValue < 100) ? "0" : "";
    statusValue += (midiCmdValue < 10) ? "0" : "";
    statusValue += String(midiCmdValue);

    display.setFont(u8g2_font_helvR08_tf);
    display.drawStr(7, 18, statusCmdType.c_str());
    display.drawStr(37, 18, statusChannel.c_str());
    display.drawStr(67, 18, statusCcNum.c_str());
    display.drawStr(97, 18, statusValue.c_str());

    display.setFont(u8g2_font_inb19_mf);
    display.drawStr(5, 44, msg);

    display.setFont(u8g2_font_helvB10_tf);
    display.drawStr(7, 60, page);

    display.sendBuffer();
}

void rotary_onButtonClick()
{
    switch (PAGE)
    {
    case pageCmdType:
        PAGE = pageChannel;
        encoder.setBoundaries(1, 16, false);
        encoder.reset(midiChannel);
        msg = (midiChannel < 10) ? "0" : "";
        msg += String(midiChannel);
        page = "channel";
        break;
    case pageChannel:
        if (midiCmdType == 0)
        {
            PAGE = pageCmdValue;
            encoder.setBoundaries(0, 127, false);
            encoder.reset(midiCmdValue);
            msg = (midiCmdValue < 100) ? "0" : "";
            msg += (midiCmdValue < 10) ? "0" : "";
            msg += String(midiCmdValue);
            page = "value";
        }
        else if (midiCmdType == 1)
        {
            PAGE = pageCcNum;
            encoder.setBoundaries(0, 127, false);
            encoder.reset(midiCcNum);
            msg = (midiCcNum < 100) ? "0" : "";
            msg += (midiCcNum < 10) ? "0" : "";
            msg += String(midiCcNum);
            page = "cc num";
        }
        break;
    case pageCcNum:
        PAGE = pageCmdValue;
        encoder.setBoundaries(0, 127, false);
        encoder.reset(midiCmdValue);
        msg = (midiCmdValue < 100) ? "0" : "";
        msg += (midiCmdValue < 10) ? "0" : "";
        msg += String(midiCmdValue);
        page = "value";
        break;
    case pageCmdValue:
        PAGE = pageCmdType;
        encoder.setBoundaries(0, 1, false);
        encoder.reset(midiCmdType);
        if (midiCmdType == 0)
            msg = "PC";
        else if (midiCmdType == 1)
            msg = "CC";
        page = "type";
        break;
    }

    setOled(msg.c_str(), page.c_str());
}

void buttonSendClick()
{
    if (midiCmdType == 0)
        MIDI.sendProgramChange(midiCmdValue, midiChannel);
    else if (midiCmdType == 1)
        MIDI.sendControlChange(midiCcNum, midiCmdValue, midiChannel);

    setOled("Sent!", "");
    // setOled(msg.c_str(), page.c_str());
}

void rotary_loop()
{

    if (encoder.currentButtonState() == BUT_RELEASED)
    {
        rotary_onButtonClick();
    }

    if (buttonSend.currentButtonState() == BUT_RELEASED)
    {
        buttonSendClick();
    }

    int16_t encoderDelta = encoder.encoderChanged();

    if (encoderDelta == 0)
        return;

    if (encoderDelta != 0)
    {
        encoderValue = encoder.readEncoder();

        switch (PAGE)
        {
        case pageCmdType:
            midiCmdType = encoderValue;
            if (midiCmdType == 0)
                msg = "PC";
            else if (midiCmdType == 1)
                msg = "CC";
            page = "type";
            break;
        case pageChannel:
            midiChannel = encoderValue;
            msg = (midiChannel < 10) ? "0" : "";
            msg += String(midiChannel);
            page = "channel";
            break;
        case pageCcNum:
            midiCcNum = encoderValue;
            msg = (midiCcNum < 100) ? "0" : "";
            msg += (midiCcNum < 10) ? "0" : "";
            msg += String(midiCcNum);
            page = "cc num";
            break;
        case pageCmdValue:
            midiCmdValue = encoderValue;
            msg = (midiCmdValue < 100) ? "0" : "";
            msg += (midiCmdValue < 10) ? "0" : "";
            msg += String(midiCmdValue);
            page = "value";
            break;
        }

        setOled(msg.c_str(), page.c_str());
    }
}
