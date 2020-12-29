/*
  functions.h - header file for reading smartmeter

  Copyright (C) 2020  CommanderTux

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#define RXD2 16                                                       // GPIO16 = Rx
#define TXD2 17                                                       // GPIO17 = Tx

#define ETX '\x03'                                                    // ETX char
#define STX '\x02'                                                    // STX char
#define CR  '\x0D'                                                    // CR char
#define LF  '\x0A'                                                    // LF char

char Requestmsg[] = {0x2F,0x3F,0x21,0x0D,0x0A,0x00};                  // Request message /?!/r/n
char Acknowledge[] = {0x06,0x30,0x30,0x30,0x0D,0x0A,0x00};            // Acknowledge message 000/r/n
char id_byte=0;                                                       // stores received byte
char id_message[25];                                                  // store identification message
String datablock[25];                                                 // datablock buffer (need to change to char[])

Influxdb strato(INFLUXDB_STRATO);                                     // Influxdb Strato
Influxdb ubuntu(INFLUXDB_UBUNTU);                                     // Influxdb Ubuntu 

void process() { 
  //----- Write request message to serial port ----------------
  bool timeout = false;
  int len, howLongToWait = 4000;
  char id_byte=0;
  Serial1.begin(300, SERIAL_7E1, RXD2, TXD2);                         // U2UXD serial port 300
  delay (1000);                                                       // Wait ......
  do {
    digitalWrite(LED_BUILTIN, HIGH);                                  // LED on
    Serial1.write(Requestmsg);                                        // write Request message to smartmeter
    Serial1.flush();                                                  // Wait for Request message to finish
    digitalWrite(LED_BUILTIN, LOW);                                   // LED off
    Serial.println("Request message send: " + String(Requestmsg));

    //----- Read identification message from serial port --------

    timeout = false;
    len=0;
    memset(id_message, 0, sizeof(id_message));
    unsigned long startedWaiting = millis();
//    id_message[] = Serial1.readStringUntil(CR);                 // it's a fail
    do {
      if (Serial1.available() > 0) {                                  // If new character available ...
        id_byte = Serial1.read();                                     // Read next char
        id_message[len++] = id_byte;                                  // append to ID message
      }
      if((millis() - startedWaiting > howLongToWait) || (len >= 25 )) {
        timeout = true;
        Serial.println("Timeout!");
        Serial.println(id_message);
        break;
      }
    }
    while (id_byte != CR);                                            // Until CR 
  }
  while(timeout);                                                     // Timeout occured?

  if (len < 10 || len > 20 ) {
    Serial.println("Acknowledge message length incorrect\n");
    Serial.end();
    return;
  }
  if (id_message[0] != 47) {
    Serial.println("Identification message incorrect!\n");
    Serial.end();
    return;
  }
  Serial.println("ID message = " + String(id_message));

  //---------- Detect new baudrate -------------------
  char speedChar = id_message[4];
  Serial.println("Speedchar = " + String(speedChar));
  int serial_speed = 300 << (speedChar-48);
  Serial.println("New serial speed = " + String(serial_speed));
  
  //---------- Select new baudrate ---------------------
  Acknowledge[2] = speedChar;                                       // Change speedchar value
  Serial.println("Acknowledge message = " + String(Acknowledge));       
  digitalWrite(LED_BUILTIN, HIGH);                                  // LED on
  Serial1.write(Acknowledge);                                       // send acknowledge message to smartmeter
  Serial1.flush();                                                  // Wait for Acknowledge message to finish
  digitalWrite(LED_BUILTIN, LOW);                                   // LED off
  Serial1.begin(serial_speed, SERIAL_7E1, RXD2, TXD2);              // select new serial speed to smartmeter

  //---------- Read datablock ---------------------

  char rx_byte = 0;                                                 // Clear Rx
  int line = 0;                                                     // Clear linenumber
  memset(datablock, 0, sizeof(datablock));                          // Clear datablock
  digitalWrite(LED_BUILTIN, HIGH);                                  // LED on
  while (rx_byte != ETX) {                                          // While not ETX continue reading
    if (Serial1.available()> 0) {                                   // If new character available ...
      rx_byte = Serial1.read();                                     // Read next char
      datablock[line] += rx_byte;                                   // append character to datablock
      if (rx_byte == LF) {                                          // increment line number if LF (use to be CR)
        Serial.print(datablock[line++]);                            // remove after.........
      }
    }
  }
//  Serial1.end();
  digitalWrite(LED_BUILTIN, LOW);                                   // LED off
  Serial.println("Done reading..");

  //---------- Split datablock message from buffer ----------
  String map[] = {"1.8.0", "2.8.0", "32.7", "31.7", "36.7"};        // compare buffer against these values
  float sql_data[5];                                                // Number of element in map array
  for (int b = 0; b < 5; b++) {                                     // Test against all elements in map array
    for (int a = 0; a < line; a++) {                                // Test all datablock members
      if (datablock[a].startsWith(map[b])) {
        sql_data[b] = (datablock[a].substring(datablock[a].indexOf("(") + 1, datablock[a].indexOf("*"))).toFloat();
      }
    }
  }
  Serial.println("\n\nValues found:");
  for (int a = 0; a < 5; a++) {                                     // Print all found values
    Serial.print(map[a]);
    Serial.print(": \t");
    Serial.println(sql_data[a],3);
  }

  /*------------------- InfluxDB export --------------------------------*/
  strato.setDbAuth(INFLUXDB_DATABASE, INFLUXDB_USER, INFLUXDB_PASS);    // influxdb Strato
  ubuntu.setDbAuth(INFLUXDB_DATABASE, INFLUXDB_USER, INFLUXDB_PASS);    // influxdb Ubuntu
  
  char fields[200];
  char formatString[] = "solar_data power_used=%0.3f,power_delivered=%0.3f,voltage_metered=%0.3f,amperage_metered=%0.3f,active_power=%0.3f";
  sprintf(fields, formatString, sql_data[0], sql_data[1], sql_data[2], sql_data[3],sql_data[4]);
  
  // write it into db's
  //  strato.write(fields);   // Uncomment after testing!!!!!!!!!!!
  ubuntu.write(fields);   // Uncomment after testing!!!!!!!!!!!
}
  
