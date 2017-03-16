/**
 * Example for efficient call-response using ack-payloads
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well.
 * This allows very fast call-response communication, with the responding radio never having to
 * switch out of Primary Receiver mode to send back a payload, but having the option to switch to
 * primary transmitter if wanting to initiate communication instead of respond to a commmunication.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <RF24/RF24.h>



//>>>>>>>>>>>>>>>>pi radio

static const char                                       topic_backyardtemp1[] = "backyard/temp1";
static const char                                       topic_housetemp1[] = "house/temp1";
static const char                                       topic_househum1[] = "house/hum1";
static const char                                       topic_backyardhum1[] = "backyard/hum1";
static const char                                       topic_allLEDs[] = "house/led";
char *ptemp;       //global variable, holds the mqtt message in the payload
int   msglen;		// global mqtt message lenght


/****************** Raspberry Pi ***********************/

RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

//aaa

/********** User Config *********/
// Assign a unique identifier for this node, 0 or 1. Arduino example uses radioNumber 0 by default.
bool radioNumber = 1;

/********************************/


// Radio pipe addresses for the nodes to communicate.
//const uint8_t addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};
//const uint8_t addresses[][6] = {"1Node","2Node"};
const uint64_t addresses[6] = {0xF0F0F0F0E1LL,0xF0F0F0F0E2LL,0xF0F0F0F0E3LL,0xF0F0F0F0E4LL,0xF0F0F0F0E5LL,0xF0F0F0F0E6LL};
//uint64_t addresses[][6] = {0xF0F0F0F0E1LL,0xF0F0F0F0E2LL,0xF0F0F0F0E3LL,0xF0F0F0F0E4LL,0xF0F0F0F0E5LL,0xF0F0F0F0E6LL};
//const uint8_t addresses[][6] = {"1Node","2Node"};
//const uint8_t addresses[6] = {0xF0F0F0F0E1LL,0xF0F0F0F0E2LL,0xF0F0F0F0E3LL,0xF0F0F0F0E4LL,0xF0F0F0F0E5LL,0xF0F0F0F0E6LL};
//const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
bool role_ping_out = 1, role_pong_back = 0, role = 0;
//uint8_t counter = 1;                                // A single byte to keep track of the data being sent back and forth


uint8_t data[32];
unsigned long startTime, stopTime, counter, rxTimer=0;



//>>>>>>>>>>>>>>>>>>>> pi radio end


    extern "C" {
    #include "MQTTClient.h"
    #include "MQTTClientPersistence.h"
    #include "MQTTAsync.h"
    }

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>mqtt start >>>>>>>

#define ADDRESS     "tcp://192.168.0.8:1883"
#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "kitchen/led1"
#define TOPIC       "#"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L




volatile MQTTAsync_token deliveredtoken;

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
	    finished = 1;
	}
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;
    uint8_t gotByte;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);


    ptemp = (char *)message->payload;




    if (strncmp (topicName,topic_allLEDs,9) == 0)
    {
      printf ("LED topic arrived %s\n",topicName);
      msglen = message->payloadlen;
      role = 1;      //ping out



     } // if led message



    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}


void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}


void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;

	deliveredtoken = 0;

	if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>mqtt end >>>>>>>>>>>


using namespace std;

int main(int argc, char** argv){

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>mqtt start >>>>>>>>>>>>>>

	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_token token;
	int rc;
	int ch;

	MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}



//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>mqtt finish >>>>>>>>>>>>




int i;
int j;
char temp[]="00.0";
char hum[]="00.0";
uint8_t pipeNo;
uint8_t gotByte;
uint8_t len;
char receive_payload[33]; // 

//bool result;
char buffer [50];  // holds the mqtt message



  cout << "RPi/RF24/examples/gettingstarted_call_response\n";
  radio.begin();
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.enableDynamicPayloads();
radio.setRetries(5,15);  
radio.printDetails();                   // Dump the configuration of the rf unit for debugging

//const uint8_t addresses[][6] = {"1Node","2Node","3Node","4Node"}

      	radio.openWritingPipe(addresses[1]);
      	radio.openReadingPipe(1,addresses[0]);
      	radio.openReadingPipe(2,addresses[2]);  // open a second sender up
	radio.openReadingPipe(3,addresses[3]);
	radio.openReadingPipe(4,addresses[4]);
	radio.openReadingPipe(5,addresses[5]);
	radio.startListening();
//	radio.writeAckPayload(1,&counter,1);



// forever loop
//
 while (1){
// may reenable the ack to improve comms later
     // radio.writeAckPayload(pipeNo,&gotByte, 1 );   // This can be commented out to send empty payloads.	  
role = 0;

//printf("in pong loop \n\r");
//  while	(!subscribed) {
//printf("in not subscribed loop \n\r");


  if ( role == role_pong_back )
  {

     while(radio.available(&pipeNo)){
	len = radio.getDynamicPayloadSize();
	radio.read( &data, len );

	data[len] = 0;

  //    radio.read(&data,32);
  //    counter++;
	printf("Data received %s  pipe %d \n\r", data,pipeNo);


        if (pipeNo==0){

        radio.startListening();

		printf("we should not be in this loop, we have an ack %s \n\r",data);



	}



	if (pipeNo==1){

		for (i=0;i<4;i++){
			temp[i] = data[i];
		} //for i

		for(j=0;j<4;j++){
			hum[j] = data[j+6];
		
		}	 //for j
	//	printf("pipe 1 backyard %s  Hum  %s \n\r", temp, hum);
		//printf("mosquitto_pub -d  -t backyard/temp1 -h 192.168.0.8 -m \"%s\"\n\r", temp);
		sprintf(buffer, "mosquitto_pub  -t backyard/temp1 -h 192.168.0.8 -m \"%s\"", temp);
		system(buffer);
		sprintf(buffer, "mosquitto_pub  -t backyard/humidity1 -h 192.168.0.8 -m \"%s\"", hum);
		//printf("mosquitto_pub -d  -t backyard/hum1 -h 192.168.0.8 -m \"%s\"\n\r", hum);
		system(buffer);


	}

        if (pipeNo==2){

		for (i=0;i<4;i++){
			temp[i] = data[i];
		} //for i

		for(j=0;j<4;j++){
			hum[j] = data[j+6];
		
		} //for j
	//	printf("pipe 2 house  %s  Hum  %s \n\r", temp, hum);
		sprintf(buffer, "mosquitto_pub  -t house/temp1 -h 192.168.0.8 -m \"%s\"", temp);
		//printf("mosquitto_pub  -t house/temp1 -h 192.168.0.8 -m \"%s\"\n\r", temp);
		system(buffer);
		//printf("mosquitto_pub -d  -t house/hum1 -h 192.168.0.8 -m \"%s\"\n\r", hum);
		sprintf(buffer, "mosquitto_pub  -t house/humidity1 -h 192.168.0.8 -m \"%s\"", hum);
		system(buffer);


        }

        if (pipeNo==3){

		for (i=0;i<4;i++){
			temp[i] = data[i];
		} //for i

		for(j=0;j<4;j++){
			hum[j] = data[j+6];
		
		} //for j
		//printf("pipe 3 stm32 data  %s \n\r", data);
		//printf("pipe 3   %s  Hum  %s \n\r", temp, hum);
			//printf("pipe 3 stm32  %s  Hum  \n\r", data);
		sprintf(buffer, "mosquitto_pub   -t house/temp2 -h 192.168.0.8 -m \"%s\"", temp);
		//printf("mosquitto_pub  -t house/temp2 -h 192.168.0.8 -m \"%s\"\n\r", temp);
		system(buffer);
		//printf("mosquitto_pub -d  -t house/hum2 -h 192.168.0.8 -m \"%s\"\n\r", hum);
		sprintf(buffer, "mosquitto_pub  -t house/humidity2 -h 192.168.0.8 -m \"%s\"", hum);
		system(buffer);


        }



     }  //while(radio.available(&pipeNo))






   } //ping pong back



if (role == role_ping_out)
  {
    // The payload will always be the same, what will change is how much of it we send.
    //static char send_payload[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ789012";

    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    printf("Now sending length %i...",msglen);
    radio.write( ptemp, msglen );

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 1000 )
        timeout = true;

    // Describe the results
    if ( timeout )
    {
      printf("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      uint8_t len = radio.getDynamicPayloadSize();
      radio.read( receive_payload, len );

      // Put a zero at the end for easy printing
      receive_payload[len] = 0;

      // Spew it
      printf("Got response size=%i value=%s\n\r",len,receive_payload);
    }

    // Update size for next time.
    //next_payload_size += payload_size_increments_by;
    //if ( next_payload_size > max_payload_size )
    //  next_payload_size = min_payload_size;

    // Try again 1s later
   // delay(100);
  }















} //while 1



} //main
