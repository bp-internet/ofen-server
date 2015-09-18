// Wiring
//
// SI  -> 11
// SO  -> 12
// SCK -> 13
// CS  -> /
// VCC -> 3V3 !!!
// GND -> GND
//
// (c) by bp-inter.net
//


#include <EtherCard.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x02
};
static byte myip[] = {
  192, 168, 1, 7
};

byte Ethernet::buffer[500];
BufferFiller bfill;

float temp1, temp2;

void setup () {
  Serial.begin(9600);
  Serial.println("start");
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  ether.staticSetup(myip);


  Serial.println(F("Setting up DHCP"));
  // if (!ether.dhcpSetup())
  Serial.println(F("DHCP failed"));
  Serial.println("aaa");
  ether.printIp("My IP: ", ether.myip);
  Serial.println(ether.myip[3]);
  Serial.println("bbb");


  sensors.begin();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1");



  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(outsideThermometer);
  Serial.println();

  // set the resolution to 9 bit
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC);
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(outsideThermometer), DEC);
  Serial.println();
}

static word homePage(int temp1, int temp2) {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
                 "HTTP/1.0 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Pragma: no-cache\r\n"
                 "\r\n"
                 "<title>Temp-Server</title>"
                 "$D-$D"),
               temp1, temp2);
  return bfill.position();
}

static word homePageJSON(int temp1, int temp2) {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
                 "HTTP/1.0 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Pragma: no-cache\r\n"
                 "\r\n"
                 "{\"temp1\":$D,\"temp2\":$D}"),
               temp1, temp2);
  return bfill.position();
}

static word notFound() {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
                 "HTTP/1.0 404 Not Found\r\n\r\n"));
  return bfill.position();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

void loop () {
 word len = ether.packetReceive();
 word pos = ether.packetLoop(len);
//delay(1000);
  if (pos) //check if valid tcp data is received
  {
    char* data = (char *) Ethernet::buffer + pos;
    //Serial.print("data: ");
    //Serial.println(data);

    String tmp = String(data);
    String value = tmp.substring(tmp.indexOf('/') + 1, tmp.indexOf('HTTP') - 4);
    if (value.startsWith("fav")) return;
    Serial.println("get temp");
    /*
    char* data = (char *) Ethernet::buffer + pos;
    //Serial.print("data: ");
    //Serial.println(data);
    */

    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures();
    Serial.println("DONE");

    // print the device information
    //printData(insideThermometer);
    //printData(outsideThermometer);

    temp1 = sensors.getTempC(insideThermometer);
    temp2 = sensors.getTempC(outsideThermometer);

    temp1 = (int) (temp1*100);
    temp2 = (int) (temp2*100);
    if (value.startsWith("json"))
      ether.httpServerReply(homePageJSON(temp1, temp2));
    else
      ether.httpServerReply(homePage(temp1, temp2));
  }
}

