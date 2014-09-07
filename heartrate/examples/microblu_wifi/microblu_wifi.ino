/*
 * This sketch uses the MQTT library to bridge firmata to Octoblu. You can then turn pins
 * off and on remotely, and much much more. To drive from Node.js and Octoblu see:
 * https://github.com/octoblu/serial/tree/master/examples/firmata/SkynetSerial
 *
 * Requires our fork of the MQTT PubSubClient https://github.com/jacobrosenthal/pubsubclient
 *
 * Works with ethernet shields compatible with EthernetClient library from
 * Arduino. If you don't know, grab the original
 * http://arduino.cc/en/Main/ArduinoWiFiShield
 *
 * Remember not to mess with wifi's unavailable pins (7, 10, 11, 12, 13 and 4 if using SD card)
 *
 * You will notice we're using F() in Serial.print. It is covered briefly on
 * the arduino print page but it means we can store our strings in program
 * memory instead of in ram.
 *
 */

#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <StreamBuffer.h>
#include <ringbuffer.h>
#include <b64.h>
#include <Servo.h>
#include <Wire.h>
#include <Firmata.h>
#include <stdio.h>

#define LED 4//indicator, Grove - LED is connected with D4 of Arduino
#define BAC_SENSOR A0// Blood Alcohol Sensor
boolean led_state = LOW;//state of LED, each time an external interrupt
                //will change the state of LED
unsigned char counter;
unsigned long temp[21];
unsigned long sub;
bool data_effect=true;
unsigned int heart_rate;//the measurement result of heart rate
unsigned int previous_heart_rate=0;//the measurement result of heart rate
unsigned int bac=0;//the measurement result of heart rate
unsigned int previous_bac=0;//the measurement result of heart rate

const int max_heartpluse_duty = 2000;//you can change it follow your system's request.

ringbuffer write(150); //firmata out - Capabilities Response requires ~150 on Uno
ringbuffer read(50); //firmata in - min 67% of biggest incoming firmata b64 string

StreamBuffer stream(write, read);
StreamBuffer externalaccess(read, write);

// char ssid[] = "Gangplank"; //  your network SSID (name)
// char pass[] = "walktheplank";    // your WPA network password
// char ssid[] = "OB-HQ"; //  your network SSID (name)
// char pass[] = "0ct0b1u2014";    // your WPA network password
char ssid[] = "Cocktopod"; //  your network SSID (name)
char pass[] = "mofos2010";    // your WPA network password
int keyIndex = 0;              // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

char server[] = "meshblu.octoblu.com";
// char server[] = "192.168.2.5";
// char server[] = "172.17.1.171";

//Your 'firmware' type UUID and token for Octoblu //TODO where to get one
char UUID[]  = "d870d511-1c42-11e4-861d-89322229e557";
char TOKEN[] = "036lco5cu9haxajorciqfdv7sx3fecdi";

WiFiClient client;
PubSubClient microblu(server, 1883, onMessage, client);

// move the following defines to Firmata.h?
#define I2C_WRITE B00000000
#define I2C_READ B00001000
#define I2C_READ_CONTINUOUSLY B00010000
#define I2C_STOP_READING B00011000
#define I2C_READ_WRITE_MODE_MASK B00011000
#define I2C_10BIT_ADDRESS_MODE_MASK B00100000

#define MAX_QUERIES 8
#define MINIMUM_SAMPLING_INTERVAL 10

#define REGISTER_NOT_SPECIFIED -1

/*==============================================================================
 * GLOBAL VARIABLES
 *============================================================================*/

/* analog inputs */
int analogInputsToReport = 0; // bitwise array to store pin reporting

/* digital input ports */
byte reportPINs[TOTAL_PORTS];       // 1 = report this port, 0 = silence
byte previousPINs[TOTAL_PORTS];     // previous 8 bits sent

/* pins configuration */
byte pinConfig[TOTAL_PINS];         // configuration of every pin
byte portConfigInputs[TOTAL_PORTS]; // each bit: 1 = pin in INPUT, 0 = anything else
int pinState[TOTAL_PINS];           // any value that has been written

/* timer variables */
unsigned long currentMillis;        // store the current value from millis()
unsigned long previousMillis;       // for comparison with currentMillis
int samplingInterval = 500;          // how often to run the main loop (in ms)

/* i2c data */
struct i2c_device_info {
  byte addr;
  byte reg;
  byte bytes;
};

/* for i2c read continuous more */
i2c_device_info query[MAX_QUERIES];

byte i2cRxData[32];
boolean isI2CEnabled = false;
signed char queryIndex = -1;
unsigned int i2cReadDelayTime = 0;  // default delay time between i2c read request and Wire.requestFrom()

Servo servos[MAX_SERVOS];

/*==============================================================================
 * FUNCTIONS
 *============================================================================*/

void readAndReportData(byte address, int theRegister, byte numBytes) {
  // allow I2C requests that don't require a register read
  // for example, some devices using an interrupt pin to signify new data available
  // do not always require the register read so upon interrupt you call Wire.requestFrom()
  if (theRegister != REGISTER_NOT_SPECIFIED) {
    Wire.beginTransmission(address);
    #if ARDUINO >= 100
    Wire.write((byte)theRegister);
    #else
    Wire.send((byte)theRegister);
    #endif
    Wire.endTransmission();
    // do not set a value of 0
    if (i2cReadDelayTime > 0) {
      // delay is necessary for some devices such as WiiNunchuck
      delayMicroseconds(i2cReadDelayTime);
    }
  } else {
    theRegister = 0;  // fill the register with a dummy value
  }

  Wire.requestFrom(address, numBytes);  // all bytes are returned in requestFrom

  // check to be sure correct number of bytes were returned by slave
  if(numBytes == Wire.available()) {
    i2cRxData[0] = address;
    i2cRxData[1] = theRegister;
    for (int i = 0; i < numBytes; i++) {
      #if ARDUINO >= 100
      i2cRxData[2 + i] = Wire.read();
      #else
      i2cRxData[2 + i] = Wire.receive();
      #endif
    }
  }
  else {
    if(numBytes > Wire.available()) {
      Firmata.sendString("I2C Read Error: Too many bytes received");
    } else {
      Firmata.sendString("I2C Read Error: Too few bytes received");
    }
  }

  // send slave address, register and received bytes
  Firmata.sendSysex(SYSEX_I2C_REPLY, numBytes + 2, i2cRxData);
}

void outputPort(byte portNumber, byte portValue, byte forceSend)
{
  // pins not configured as INPUT are cleared to zeros
  portValue = portValue & portConfigInputs[portNumber];
  // only send if the value is different than previously sent
  if(forceSend || previousPINs[portNumber] != portValue) {
    Firmata.sendDigitalPort(portNumber, portValue);
    previousPINs[portNumber] = portValue;
  }
}

/* -----------------------------------------------------------------------------
 * check all the active digital inputs for change of state, then add any events
 * to the Serial output queue using Serial.print() */
void checkDigitalInputs(void)
{
  /* Using non-looping code allows constants to be given to readPort().
   * The compiler will apply substantial optimizations if the inputs
   * to readPort() are compile-time constants. */
  if (TOTAL_PORTS > 0 && reportPINs[0]) outputPort(0, readPort(0, portConfigInputs[0]), false);
  if (TOTAL_PORTS > 1 && reportPINs[1]) outputPort(1, readPort(1, portConfigInputs[1]), false);
  if (TOTAL_PORTS > 2 && reportPINs[2]) outputPort(2, readPort(2, portConfigInputs[2]), false);
  if (TOTAL_PORTS > 3 && reportPINs[3]) outputPort(3, readPort(3, portConfigInputs[3]), false);
  if (TOTAL_PORTS > 4 && reportPINs[4]) outputPort(4, readPort(4, portConfigInputs[4]), false);
  if (TOTAL_PORTS > 5 && reportPINs[5]) outputPort(5, readPort(5, portConfigInputs[5]), false);
  if (TOTAL_PORTS > 6 && reportPINs[6]) outputPort(6, readPort(6, portConfigInputs[6]), false);
  if (TOTAL_PORTS > 7 && reportPINs[7]) outputPort(7, readPort(7, portConfigInputs[7]), false);
  if (TOTAL_PORTS > 8 && reportPINs[8]) outputPort(8, readPort(8, portConfigInputs[8]), false);
  if (TOTAL_PORTS > 9 && reportPINs[9]) outputPort(9, readPort(9, portConfigInputs[9]), false);
  if (TOTAL_PORTS > 10 && reportPINs[10]) outputPort(10, readPort(10, portConfigInputs[10]), false);
  if (TOTAL_PORTS > 11 && reportPINs[11]) outputPort(11, readPort(11, portConfigInputs[11]), false);
  if (TOTAL_PORTS > 12 && reportPINs[12]) outputPort(12, readPort(12, portConfigInputs[12]), false);
  if (TOTAL_PORTS > 13 && reportPINs[13]) outputPort(13, readPort(13, portConfigInputs[13]), false);
  if (TOTAL_PORTS > 14 && reportPINs[14]) outputPort(14, readPort(14, portConfigInputs[14]), false);
  if (TOTAL_PORTS > 15 && reportPINs[15]) outputPort(15, readPort(15, portConfigInputs[15]), false);
}

// -----------------------------------------------------------------------------
/* sets the pin mode to the correct state and sets the relevant bits in the
 * two bit-arrays that track Digital I/O and PWM status
 */
void setPinModeCallback(byte pin, int mode)
{
  if (pinConfig[pin] == I2C && isI2CEnabled && mode != I2C) {
    // disable i2c so pins can be used for other functions
    // the following if statements should reconfigure the pins properly
    disableI2CPins();
  }
  if (IS_PIN_SERVO(pin) && mode != SERVO && servos[PIN_TO_SERVO(pin)].attached()) {
    servos[PIN_TO_SERVO(pin)].detach();
  }
  if (IS_PIN_ANALOG(pin)) {
    reportAnalogCallback(PIN_TO_ANALOG(pin), mode == ANALOG ? 1 : 0); // turn on/off reporting
  }
  if (IS_PIN_DIGITAL(pin)) {
    if (mode == INPUT) {
      portConfigInputs[pin/8] |= (1 << (pin & 7));
    } else {
      portConfigInputs[pin/8] &= ~(1 << (pin & 7));
    }
  }
  pinState[pin] = 0;
  switch(mode) {
  case ANALOG:
    if (IS_PIN_ANALOG(pin)) {
      if (IS_PIN_DIGITAL(pin)) {
        pinMode(PIN_TO_DIGITAL(pin), INPUT); // disable output driver
        digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable internal pull-ups
      }
      pinConfig[pin] = ANALOG;
    }
    break;
  case INPUT:
    if (IS_PIN_DIGITAL(pin)) {
      pinMode(PIN_TO_DIGITAL(pin), INPUT); // disable output driver
      digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable internal pull-ups
      pinConfig[pin] = INPUT;
    }
    break;
  case OUTPUT:
    if (IS_PIN_DIGITAL(pin)) {
      digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable PWM
      pinMode(PIN_TO_DIGITAL(pin), OUTPUT);
      pinConfig[pin] = OUTPUT;
    }
    break;
  case PWM:
    if (IS_PIN_PWM(pin)) {
      pinMode(PIN_TO_PWM(pin), OUTPUT);
      analogWrite(PIN_TO_PWM(pin), 0);
      pinConfig[pin] = PWM;
    }
    break;
  case SERVO:
    if (IS_PIN_SERVO(pin)) {
      pinConfig[pin] = SERVO;
      if (!servos[PIN_TO_SERVO(pin)].attached()) {
          servos[PIN_TO_SERVO(pin)].attach(PIN_TO_DIGITAL(pin));
      }
    }
    break;
  case I2C:
    if (IS_PIN_I2C(pin)) {
      // mark the pin as i2c
      // the user must call I2C_CONFIG to enable I2C for a device
      pinConfig[pin] = I2C;
    }
    break;
  default:
    Firmata.sendString("Unknown pin mode"); // TODO: put error msgs in EEPROM
  }
  // TODO: save status to EEPROM here, if changed
}

void analogWriteCallback(byte pin, int value)
{
  if (pin < TOTAL_PINS) {
    switch(pinConfig[pin]) {
    case SERVO:
      if (IS_PIN_SERVO(pin))
        servos[PIN_TO_SERVO(pin)].write(value);
        pinState[pin] = value;
      break;
    case PWM:
      if (IS_PIN_PWM(pin))
        analogWrite(PIN_TO_PWM(pin), value);
        pinState[pin] = value;
      break;
    }
  }
}

void digitalWriteCallback(byte port, int value)
{
  byte pin, lastPin, mask=1, pinWriteMask=0;

  if (port < TOTAL_PORTS) {
    // create a mask of the pins on this port that are writable.
    lastPin = port*8+8;
    if (lastPin > TOTAL_PINS) lastPin = TOTAL_PINS;
    for (pin=port*8; pin < lastPin; pin++) {
      // do not disturb non-digital pins (eg, Rx & Tx)
      if (IS_PIN_DIGITAL(pin)) {
        // only write to OUTPUT and INPUT (enables pullup)
        // do not touch pins in PWM, ANALOG, SERVO or other modes
        if (pinConfig[pin] == OUTPUT || pinConfig[pin] == INPUT) {
          pinWriteMask |= mask;
          pinState[pin] = ((byte)value & mask) ? 1 : 0;
        }
      }
      mask = mask << 1;
    }
    writePort(port, (byte)value, pinWriteMask);
  }
}


// -----------------------------------------------------------------------------
/* sets bits in a bit array (int) to toggle the reporting of the analogIns
 */
//void FirmataClass::setAnalogPinReporting(byte pin, byte state) {
//}
void reportAnalogCallback(byte analogPin, int value)
{
  if (analogPin < TOTAL_ANALOG_PINS) {
    if(value == 0) {
      analogInputsToReport = analogInputsToReport &~ (1 << analogPin);
    } else {
      analogInputsToReport = analogInputsToReport | (1 << analogPin);
    }
  }
  // TODO: save status to EEPROM here, if changed
}

void reportDigitalCallback(byte port, int value)
{
  if (port < TOTAL_PORTS) {
    reportPINs[port] = (byte)value;
  }
  // do not disable analog reporting on these 8 pins, to allow some
  // pins used for digital, others analog.  Instead, allow both types
  // of reporting to be enabled, but check if the pin is configured
  // as analog when sampling the analog inputs.  Likewise, while
  // scanning digital pins, portConfigInputs will mask off values from any
  // pins configured as analog
}

/*==============================================================================
 * SYSEX-BASED commands
 *============================================================================*/

void sysexCallback(byte command, byte argc, byte *argv)
{
  byte mode;
  byte slaveAddress;
  byte slaveRegister;
  byte data;
  unsigned int delayTime;

  switch(command) {
  case I2C_REQUEST:
    mode = argv[1] & I2C_READ_WRITE_MODE_MASK;
    if (argv[1] & I2C_10BIT_ADDRESS_MODE_MASK) {
      Firmata.sendString("10-bit addressing mode is not yet supported");
      return;
    }
    else {
      slaveAddress = argv[0];
    }

    switch(mode) {
    case I2C_WRITE:
      Wire.beginTransmission(slaveAddress);
      for (byte i = 2; i < argc; i += 2) {
        data = argv[i] + (argv[i + 1] << 7);
        #if ARDUINO >= 100
        Wire.write(data);
        #else
        Wire.send(data);
        #endif
      }
      Wire.endTransmission();
      delayMicroseconds(70);
      break;
    case I2C_READ:
      if (argc == 6) {
        // a slave register is specified
        slaveRegister = argv[2] + (argv[3] << 7);
        data = argv[4] + (argv[5] << 7);  // bytes to read
        readAndReportData(slaveAddress, (int)slaveRegister, data);
      }
      else {
        // a slave register is NOT specified
        data = argv[2] + (argv[3] << 7);  // bytes to read
        readAndReportData(slaveAddress, (int)REGISTER_NOT_SPECIFIED, data);
      }
      break;
    case I2C_READ_CONTINUOUSLY:
      if ((queryIndex + 1) >= MAX_QUERIES) {
        // too many queries, just ignore
        Firmata.sendString("too many queries");
        break;
      }
      queryIndex++;
      query[queryIndex].addr = slaveAddress;
      query[queryIndex].reg = argv[2] + (argv[3] << 7);
      query[queryIndex].bytes = argv[4] + (argv[5] << 7);
      break;
    case I2C_STOP_READING:
    byte queryIndexToSkip;
      // if read continuous mode is enabled for only 1 i2c device, disable
      // read continuous reporting for that device
      if (queryIndex <= 0) {
        queryIndex = -1;
      } else {
        // if read continuous mode is enabled for multiple devices,
        // determine which device to stop reading and remove it's data from
        // the array, shifiting other array data to fill the space
        for (byte i = 0; i < queryIndex + 1; i++) {
          if (query[i].addr = slaveAddress) {
            queryIndexToSkip = i;
            break;
          }
        }

        for (byte i = queryIndexToSkip; i<queryIndex + 1; i++) {
          if (i < MAX_QUERIES) {
            query[i].addr = query[i+1].addr;
            query[i].reg = query[i+1].addr;
            query[i].bytes = query[i+1].bytes;
          }
        }
        queryIndex--;
      }
      break;
    default:
      break;
    }
    break;
  case I2C_CONFIG:
    delayTime = (argv[0] + (argv[1] << 7));

    if(delayTime > 0) {
      i2cReadDelayTime = delayTime;
    }

    if (!isI2CEnabled) {
      enableI2CPins();
    }

    break;
  case SERVO_CONFIG:
    if(argc > 4) {
      // these vars are here for clarity, they'll optimized away by the compiler
      byte pin = argv[0];
      int minPulse = argv[1] + (argv[2] << 7);
      int maxPulse = argv[3] + (argv[4] << 7);

      if (IS_PIN_SERVO(pin)) {
        if (servos[PIN_TO_SERVO(pin)].attached())
          servos[PIN_TO_SERVO(pin)].detach();
        servos[PIN_TO_SERVO(pin)].attach(PIN_TO_DIGITAL(pin), minPulse, maxPulse);
        setPinModeCallback(pin, SERVO);
      }
    }
    break;
  case SAMPLING_INTERVAL:
    if (argc > 1) {
      samplingInterval = argv[0] + (argv[1] << 7);
      if (samplingInterval < MINIMUM_SAMPLING_INTERVAL) {
        samplingInterval = MINIMUM_SAMPLING_INTERVAL;
      }
    } else {
      //Firmata.sendString("Not enough data");
    }
    break;
  case EXTENDED_ANALOG:
    if (argc > 1) {
      int val = argv[1];
      if (argc > 2) val |= (argv[2] << 7);
      if (argc > 3) val |= (argv[3] << 14);
      analogWriteCallback(argv[0], val);
    }
    break;
  case CAPABILITY_QUERY:
    Firmata.write(START_SYSEX);
    Firmata.write(CAPABILITY_RESPONSE);
    for (byte pin=0; pin < TOTAL_PINS; pin++) {
      if (IS_PIN_DIGITAL(pin)) {
        Firmata.write((byte)INPUT);
        Firmata.write(1);
        Firmata.write((byte)OUTPUT);
        Firmata.write(1);
      }
      if (IS_PIN_ANALOG(pin)) {
        Firmata.write(ANALOG);
        Firmata.write(10);
      }
      if (IS_PIN_PWM(pin)) {
        Firmata.write(PWM);
        Firmata.write(8);
      }
      if (IS_PIN_SERVO(pin)) {
        Firmata.write(SERVO);
        Firmata.write(14);
      }
      if (IS_PIN_I2C(pin)) {
        Firmata.write(I2C);
        Firmata.write(1);  // to do: determine appropriate value
      }
      Firmata.write(127);
    }
    Firmata.write(END_SYSEX);
    break;
  case PIN_STATE_QUERY:
    if (argc > 0) {
      byte pin=argv[0];
      Firmata.write(START_SYSEX);
      Firmata.write(PIN_STATE_RESPONSE);
      Firmata.write(pin);
      if (pin < TOTAL_PINS) {
        Firmata.write((byte)pinConfig[pin]);
  Firmata.write((byte)pinState[pin] & 0x7F);
  if (pinState[pin] & 0xFF80) Firmata.write((byte)(pinState[pin] >> 7) & 0x7F);
  if (pinState[pin] & 0xC000) Firmata.write((byte)(pinState[pin] >> 14) & 0x7F);
      }
      Firmata.write(END_SYSEX);
    }
    break;
  case ANALOG_MAPPING_QUERY:
    Firmata.write(START_SYSEX);
    Firmata.write(ANALOG_MAPPING_RESPONSE);
    for (byte pin=0; pin < TOTAL_PINS; pin++) {
      Firmata.write(IS_PIN_ANALOG(pin) ? PIN_TO_ANALOG(pin) : 127);
    }
    Firmata.write(END_SYSEX);
    break;
  }
}

void enableI2CPins()
{
  byte i;
  // is there a faster way to do this? would probaby require importing
  // Arduino.h to get SCL and SDA pins
  for (i=0; i < TOTAL_PINS; i++) {
    if(IS_PIN_I2C(i)) {
      // mark pins as i2c so they are ignore in non i2c data requests
      setPinModeCallback(i, I2C);
    }
  }

  isI2CEnabled = true;

  // is there enough time before the first I2C request to call this here?
  Wire.begin();
}

/* disable the i2c pins so they can be used for other functions */
void disableI2CPins() {
    isI2CEnabled = false;
    // disable read continuous mode for all devices
    queryIndex = -1;
    // uncomment the following if or when the end() method is added to Wire library
    // Wire.end();
}

/*==============================================================================
 * SETUP()
 *============================================================================*/

void systemResetCallback()
{
  // initialize a defalt state
  // TODO: option to load config from EEPROM instead of default
  if (isI2CEnabled) {
    disableI2CPins();
  }
  for (byte i=0; i < TOTAL_PORTS; i++) {
    reportPINs[i] = false;      // by default, reporting off
    portConfigInputs[i] = 0;  // until activated
    previousPINs[i] = 0;
  }
  // pins with analog capability default to analog input
  // otherwise, pins default to digital output
  for (byte i=0; i < TOTAL_PINS; i++) {

    // skip SPI pins for Ethernet/Wifi Shield //JJR
    if ( (i==2) || (i==4) || (i==7) || (i==MOSI) || (i==MISO) || (i==SCK) || (i==SS) )
      continue;

    if (IS_PIN_ANALOG(i)) {
      // turns off pullup, configures everything
      setPinModeCallback(i, ANALOG);
    } else {
      // sets the output to 0, configures portConfigInputs
      setPinModeCallback(i, OUTPUT);
    }
  }
  // by default, do not report any analog inputs
  analogInputsToReport = 0;

  /* send digital inputs to set the initial state on the host computer,
   * since once in the loop(), this firmware will only send on change */
  /*
  TODO: this can never execute, since no pins default to digital input
        but it will be needed when/if we support EEPROM stored config
  for (byte i=0; i < TOTAL_PORTS; i++) {
    outputPort(i, readPort(i, portConfigInputs[i]), true);
  }
  */
}

//we'll run this if anyone messages us
void onMessage(char* topic, byte* payload, unsigned int length) {

  // handle incoming messages, well just print it for now
  Serial.println(topic);
  for(int i =0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();

  b64::decode((char*)payload, length, externalaccess);
}


void setup()
{

  Serial.begin(9600);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    // don't continue:
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if ( fv != "1.1.0" )
    Serial.println(F("Please upgrade the firmware"));

  Firmata.setFirmwareVersion(FIRMATA_MAJOR_VERSION, FIRMATA_MINOR_VERSION);

  Firmata.attach(ANALOG_MESSAGE, analogWriteCallback);
  Firmata.attach(DIGITAL_MESSAGE, digitalWriteCallback);
  Firmata.attach(REPORT_ANALOG, reportAnalogCallback);
  Firmata.attach(REPORT_DIGITAL, reportDigitalCallback);
  Firmata.attach(SET_PIN_MODE, setPinModeCallback);
  Firmata.attach(START_SYSEX, sysexCallback);
  Firmata.attach(SYSTEM_RESET, systemResetCallback);

  Firmata.begin(stream);

  pinMode(LED, OUTPUT);
  pinMode(BAC_SENSOR, INPUT);
  Serial.begin(9600);
  Serial.println("Please ready your chest belt.");
  delay(5000);
  arrayInit();
  Serial.println("Heart rate test begin.");
  attachInterrupt(0, interrupt, RISING);//set interrupt 0,digital port 2

  systemResetCallback();  // reset to default config
}

void loop() {

  bac = analogRead(BAC_SENSOR);
  digitalWrite(LED, led_state);//Update the state of the indicator
  //we need to call loop for the mqtt library to do its thing and send/receive our messages
  bool loopSuccess = microblu.loop();
  // Serial.print(F("microblu.loop(): "));
  // Serial.println(loopSuccess);

  if(loopSuccess){
    currentMillis = millis();
    if(currentMillis - previousMillis > samplingInterval) {
      previousMillis += samplingInterval;

      if(previous_heart_rate != heart_rate || previous_bac != bac) {
        previous_heart_rate = heart_rate;
        previous_bac = bac;
        char number[8];

        char json[] = "{\"devices\": [\"*\"], \"payload\": [";

        itoa(heart_rate, number, 10);
        strcat(json, number);
        strcat(json, ",");

        itoa(bac, number, 10);
        strcat(json, number);
        strcat(json, "]}");

        microblu.publish("message", json);
        Serial.println(json);
      }
    }
  }else
  {
    //oops we're not connected yet or we lost connection
    Serial.println(F("connecting..."));

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
      Serial.print(F("Attempting to connect to SSID: "));
      Serial.println(ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
    }

    String clientIdStr = "microblu_" + String(random(500000)) + "_" + String(random(500000));
    int clientId_len = clientIdStr.length() + 1;
    char clientId[clientId_len];
    clientIdStr.toCharArray(clientId, clientId_len);

    if (microblu.connect("", UUID, TOKEN)){
      //success!
      Serial.println(F("connected"));

      //you need to subscribe to your uuid to get messages for you
      microblu.subscribe(UUID);

      Serial.println(F("subscribed"));
    }
  }
}

/*Function: calculate the heart rate*/
void sum()
{
 if(data_effect)
    {
      heart_rate=1200000/(temp[20]-temp[0]);//60*20*1000/20_total_time
      Serial.print("Heart_rate_is:\t");
      Serial.println(heart_rate);
    }
   data_effect=1;//sign bit
}
/*Function: Interrupt service routine.Get the sigal from the external interrupt*/
void interrupt()
{
    temp[counter]=millis();
  // Serial.println(counter,DEC);
  //   Serial.println(temp[counter]);
    switch(counter)
  {
    case 0:
      sub=temp[counter]-temp[20];
      // Serial.println(sub);
      break;
    default:
      sub=temp[counter]-temp[counter-1];
      // Serial.println(sub);
      break;
  }
    if(sub>max_heartpluse_duty)//set 2 seconds as max heart pluse duty
  {
    data_effect=0;//sign bit
    counter=0;
    Serial.println("Heart rate measure error,test will restart!" );
    arrayInit();
  }
    if (counter==20&&data_effect)
    {
    counter=0;
    sum();
    }
    else if(counter!=20&&data_effect)
    counter++;
    else
    {
    counter=0;
    data_effect=1;
    }
}

/*Function: Initialization for the array(temp)*/
void arrayInit()
{
  for(unsigned char i=0;i < 20;i ++)
  {
    temp[i]=0;
  }
  temp[20]=millis();
}
