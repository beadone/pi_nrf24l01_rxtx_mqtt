# pi_nrf24l01_rxtx_mqtt
pi code to recieve radio signals from nrf24l01 radios and transmit these messages to mqtt 
9mosquitto then openhab)and to subscribe to mqtt messages 
and transmit these mqtt message to the specifc device via the nrf24l01 radio. The Radios receive temperature and humidity data and 
parse the data into mqtt message which get sent in a primative fashion using the command line. 

An interupt is setup for the mqtt subscription, which when fired, turns the Pi into a transmitor which then transmits the payload and then switches back to listerning.

There is lots to do here to improve the code and to allow it to contorl more devices and remove hard coding, but it works so worth commiting.

It uses two main libraries, the TMRh20 nrf24 library  https://github.com/TMRh20/RF24 for the radio and the 
paho library for MQTT http://www.eclipse.org/paho/files/mqttdoc/Cclient/
https://github.com/eclipse/paho.mqtt.c
You will need to use Cmake to build the mqtt library.
Your install may differ from mine, so you may need to edit the compile command

arm-linux-gnueabihf-g++ -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -Ofast -Wall -fpermissive -pthread  -I/usr/local/include/RF24/.. -I..  -I /root/paho.mqtt.c/src -L/usr/local/lib pi_nrf24l01_rxtx_mqtt.cpp -lpaho-mqtt3a -lrf24  -o pi_nrf24l01_rxtx_mqtt

check the includes, I worked out of the RF24/examples_linux directory and installed the paho code in the home directory
my includes where
-I/usr/local/include/RF24/.. -I..
-I /root/paho.mqtt.c/src
I put all the librarys in /usr/local/lib
-L/usr/local/lib

