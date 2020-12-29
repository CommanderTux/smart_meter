# smart_meter
Smartmeter readout using optical port according to IEC 62056-21 (No DSMR).

My house is fitted with solar panels connected to the grid trough a electricity smartmeter on the attick (loft). This meter is not equipped with an P1 port (DSMR). So I needed to look for a different solution to read all the metered values.
I was able to use the optical interface on the smartmeter to read the data from the meter. It uses the EC  62056-21  data  transmission  protocol. Reading into the protocol, I thought it would be possible to build an IR optical interface and write an application to read all the data from the meter.

On multiple sites, optical probes are sold for exorbitant prices. So I decided to build one myself or buy a kit.


https://www.tindie.com/products/embedded4ever/62056-21-iec1107-optical-probe/

The first program I wrote was on a Raspberry Pi in Python. Which worked fine. Later I wrote a new verion in (posix) C. This version wrote all the values found into a CSV file. Next version wrote the values to an InfluxDB database. And even later to two InfluxDB databases on seperate servers. In the winter of 2019 I made a new version on the arduino patform for an ESP8266. This version was using a software serial library. I never could get this working.
And now in the winter of 2020, I rewrote the program to an ESP32 on the arduino platfom. Which did work as expected. And now I want to share my program to the ones who are interested.

Log story short.

A complete explanation is found here: https://github.com/lvzon/dsmr-p1-parser/blob/master/doc/IEC-62056-21-notes.md
