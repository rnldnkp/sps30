/************************************************************************************
 *  Copyright (c) January 2019, version 1.0     Paul van Haastrecht
 *
 *  Version 1.1 Paul van Haastrecht
 *  - Changed the I2C information / setup.
 *
 *  Version 1.1.1 Paul van Haastrecht / March 2020
 *  - Fixed compile errors and warnings.
 *
 *  Version 1.1.2 Paul van Haastrecht / March 2020
 *  - added versions level to GetDeviceInfo()
 *
 *  Version 1.4.2 Paul van Haastrecht / May 2020
 *  - demonstrate the any I2C channel selection
 *
 *  Version 1.4.3 Paul van Haastrecht / October 2021
 *  - added selecting I2C Speed
 *
 *  =========================  Highlevel description ================================
 *
 *  This basic reading example sketch will connect to an SPS30 for getting data and
 *  display the available data. You can now select any I2C port.
 *
 *  =========================  Hardware connections =================================
 *
 *  //////////////////////////////////////////////////////////////////////////////////
 *  ## I2C I2C I2C  I2C I2C I2C  I2C I2C I2C  I2C I2C I2C  I2C I2C I2C  I2C I2C I2C ##
 *  //////////////////////////////////////////////////////////////////////////////////
 *  NOTE 1:
 *  Depending on the Wire / I2C buffer size we might not be able to read all the values.
 *  The buffer size needed is at least 60 while on many boards this is set to 32. The driver
 *  will determine the buffer size and if less than 64 only the MASS values are returned.
 *  You can manually edit the Wire.h of your board to increase (if you memory is larg enough)
 *  One can check the expected number of bytes with the I2C_expect() call as in this example
 *  see detail document.
 *
 *  NOTE 2:
 *  As documented in the datasheet, make sure to use external 10K pull-up resistor on
 *  both the SDA and SCL lines. Otherwise the communication with the sensor will fail random.
 *
 *  ..........................................................
 *  Successfully tested on ESP32
 *
 *  SPS30 pin     ESP32
 *  1 VCC -------- VUSB
 *  2 SDA -------- SDA (pin 21)
 *  3 SCL -------- SCL (pin 22)
 *  4 Select ----- GND (select I2c)
 *  5 GND -------- GND
 *
 *  The pull-up resistors should be to 3V3
 *  ..........................................................
 *  Successfully tested on ATMEGA2560, Due
 *
 *  SPS30 pin     ATMEGA
 *  1 VCC -------- 5V
 *  2 SDA -------- SDA
 *  3 SCL -------- SCL
 *  4 Select ----- GND  (select I2c)
 *  5 GND -------- GND
 *
 *  ..........................................................
 *  Successfully tested on UNO R3
 *
 *  SPS30 pin     UNO
 *  1 VCC -------- 5V
 *  2 SDA -------- A4
 *  3 SCL -------- A5
 *  4 Select ----- GND  (select I2c)
 *  5 GND -------- GND
 *
 *  When UNO-board is detected the UART code is excluded as that
 *  does not work on UNO and will save memory. Also some buffers
 *  reduced and the call to GetErrDescription() is removed to allow
 *  enough memory.
 *  ..........................................................
 *  Successfully tested on ESP8266
 *
 *  SPS30 pin     External     ESP8266
 *  1 VCC -------- 5V
 *  2 SDA -----------------------SDA
 *  3 SCL -----------------------SCL
 *  4 Select ----- GND --------- GND  (select I2c)
 *  5 GND -------- GND --------- GND
 *
 *  The pull-up resistors should be to 3V3 from the ESP8266.
 *
 *  ================================= PARAMETERS =====================================
 *
 *  From line 115 there are configuration parameters for the program
 *
 *  ================================== SOFTWARE ======================================
 *  Sparkfun ESP32
 *
 *    Make sure :
 *      - To select the Sparkfun ESP32 thing board before compiling
 *      - The serial monitor is NOT active (will cause upload errors)
 *      - Press GPIO 0 switch during connecting after compile to start upload to the board
 *
 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  ===================================================================================
 *
 *  NO support, delivered as is, have fun, good luck !!
 */

#include "sps30.h"

/////////////////////////////////////////////////////////////
//    define communication channel to use for SPS30
/////////////////////////////////////////////////////////////
#define SP30_COMMS Wire

/////////////////////////////////////////////////////////////
// Although the SPS30 (according to the datasheet) can handle 100K/s
// based on feedback from Urs Uzinger he could only the SPS30 to work
// for longer time stable at 50K.
// If you want to test that remove comments from define line below
/////////////////////////////////////////////////////////////
//#define USE_50K_SPEED 1

/////////////////////////////////////////////////////////////
/* define driver debug
 * 0 : no messages
 * 1 : request sending and receiving
 * 2 : request sending and receiving + show protocol errors */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

// function prototypes (sometimes the pre-processor does not create prototypes themself on ESPxx)
void serialTrigger(char * mess);
void ErrtoMess(char *mess, uint8_t r);
void Errorloop(char *mess, uint8_t r);
void GetDeviceInfo();
bool read_all();

// create constructor
SPS30 sps30;

void setup() {

  Serial.begin(115200);

  serialTrigger((char *) "SPS30-Example13: Basic reading with any I2C channel select. press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set driver debug level
  sps30.EnableDebugging(DEBUG);

  // Begin communication channel
  SP30_COMMS.begin();

  if (sps30.begin(&SP30_COMMS) == false) {
    Errorloop((char *) "Could not set I2C communication channel.", 0);
  }

  // check for SPS30 connection
  if (! sps30.probe()) Errorloop((char *) "could not probe / connect with SPS30.", 0);
  else  Serial.println(F("Detected SPS30."));

  // reset SPS30 connection
  if (! sps30.reset()) Errorloop((char *) "could not reset.", 0);

  // read device info
  GetDeviceInfo();

  // start measurement
  if (sps30.start()) Serial.println(F("Measurement started"));
  else Errorloop((char *) "Could NOT start measurement", 0);

  serialTrigger((char *) "Hit <enter> to continue reading.");

  if (sps30.I2C_expect() == 4)
    Serial.println(F(" !!! Due to I2C buffersize only the SPS30 MASS concentration is available !!! \n"));
}

void loop() {
  read_all();
  delay(3000);
}

/**
 * @brief : read and display device info
 */
void GetDeviceInfo()
{
  char buf[32];
  uint8_t ret;
  SPS30_version v;

  //try to read serial number
  ret = sps30.GetSerialNumber(buf, 32);
  if (ret == ERR_OK) {
    Serial.print(F("Serial number : "));
    if(strlen(buf) > 0)  Serial.println(buf);
    else Serial.println(F("not available"));
  }
  else
    ErrtoMess((char *) "could not get serial number. ", ret);

  // try to get product name
  ret = sps30.GetProductName(buf, 32);
  if (ret == ERR_OK)  {
    Serial.print(F("Product name  : "));

    if(strlen(buf) > 0)  Serial.println(buf);
    else Serial.println(F("not available"));
  }
  else
    ErrtoMess((char *) "could not get product name. ", ret);

  // try to get version info
  ret = sps30.GetVersion(&v);
  if (ret != ERR_OK) {
    Serial.println(F("Can not read version info."));
    return;
  }

  Serial.print(F("Firmware level: "));   Serial.print(v.major);
  Serial.print(".");  Serial.println(v.minor);

  Serial.print(F("Library level : "));  Serial.print(v.DRV_major);
  Serial.print(".");  Serial.println(v.DRV_minor);
}

/**
 * @brief : read and display all values
 */
bool read_all()
{
  static bool header = true;
  uint8_t ret, error_cnt = 0;
  struct sps_values val;

  // loop to get data
  do {

#ifdef USE_50K_SPEED                // update 1.4.3
    SP30_COMMS.setClock(50000);     // set to 50K
    ret = sps30.GetValues(&val);
    SP30_COMMS.setClock(100000);    // reset to 100K in case other sensors are on the same I2C-channel
#else
    ret = sps30.GetValues(&val);
#endif

    // data might not have been ready
    if (ret == ERR_DATALENGTH){

        if (error_cnt++ > 3) {
          ErrtoMess((char *) "Error during reading values: ",ret);
          return(false);
        }
        delay(1000);
    }

    // if other error
    else if(ret != ERR_OK) {
      ErrtoMess((char *) "Error during reading values: ",ret);
      return(false);
    }

  } while (ret != ERR_OK);

  // only print header first time
  if (header) {
    Serial.println(F("-------------Mass -----------    ------------- Number --------------   -Average-"));
    Serial.println(F("     Concentration [μg/m3]             Concentration [#/cm3]             [μm]"));
    Serial.println(F("P1.0\tP2.5\tP4.0\tP10\tP0.5\tP1.0\tP2.5\tP4.0\tP10\tPartSize\n"));
    header = false;
  }

  Serial.print(val.MassPM1);
  Serial.print(F("\t"));
  Serial.print(val.MassPM2);
  Serial.print(F("\t"));
  Serial.print(val.MassPM4);
  Serial.print(F("\t"));
  Serial.print(val.MassPM10);
  Serial.print(F("\t"));
  Serial.print(val.NumPM0);
  Serial.print(F("\t"));
  Serial.print(val.NumPM1);
  Serial.print(F("\t"));
  Serial.print(val.NumPM2);
  Serial.print(F("\t"));
  Serial.print(val.NumPM4);
  Serial.print(F("\t"));
  Serial.print(val.NumPM10);
  Serial.print(F("\t"));
  Serial.print(val.PartSize);
  Serial.print(F("\n"));

  return(true);
}

/**
 *  @brief : continued loop after fatal error
 *  @param mess : message to display
 *  @param r : error code
 *
 *  if r is zero, it will only display the message
 */
void Errorloop(char *mess, uint8_t r)
{
  if (r) ErrtoMess(mess, r);
  else Serial.println(mess);
  Serial.println(F("Program on hold"));
  for(;;) delay(100000);
}

/**
 *  @brief : display error message
 *  @param mess : message to display
 *  @param r : error code
 *
 */
void ErrtoMess(char *mess, uint8_t r)
{
  char buf[80];

  Serial.print(mess);

  sps30.GetErrDescription(r, buf, 80);
  Serial.println(buf);
}

/**
 * serialTrigger prints repeated message, then waits for enter
 * to come in from the serial port.
 */
void serialTrigger(char * mess)
{
  Serial.println();

  while (!Serial.available()) {
    Serial.println(mess);
    delay(2000);
  }

  while (Serial.available())
    Serial.read();
}
