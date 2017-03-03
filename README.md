# IOT
IOT contain code and architecture of motes used in project. In this project we have use **Arduino UNO** as a sensing mote and **NodeMCU 12E** for transreciver, 
alternative low cost and power consumption IOT device can also be used.

![Image of Yaktocat](farm.jpg)

There are four types of IOT devices(i.e motes) used in project:
- Arduino_mote
- Mote
- Mote_central
- Arduino_central

## Arduino_mote
Arduino UNO is used to sense data and give it to NodeMCU in Json format by SoftwareSerial. The sensing data will be:
- Temperature
- Light
- Moisture
- Battery

## Mote
NodeMCU is used to transmit the data to neghbouring nodes. We have made the network topology. 
Every node at initilization will act as Acess point, when node has data to be transmitted(i.e form Arduion) or data has arived from neighbouring node. 
It will forward it to next strongest signal.

## Mote_central
NodeMCU is used to recive data and act as central Acess point and gives data to Arduino for desision making.

## Arduino_central
Arduion is used to make desision by analysing threshold(i.e moisture) and turning moter on or off. 

