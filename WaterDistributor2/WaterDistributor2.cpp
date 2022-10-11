
#include <ZsutDhcp.h>           
#include <ZsutEthernet.h>       
#include <ZsutEthernetUdp.h>    
#include <ZsutFeatures.h>       


#define UDP_SERVER_PORT         12345

byte MAC[]={0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02}; 

#define PROTOCOL_VERSION 1
#define OBJECT_TYPE 3
#define OBJECT_ID 2
#define LOGICAL_LOCATION 2

#define MAX_BUFFER_LENGTH 4
unsigned char packetBuffer[MAX_BUFFER_LENGTH];


unsigned int localPort=UDP_SERVER_PORT;    

uint8_t tap=0;

uint16_t t;

ZsutEthernetUDP Udp;      

//Creates ALP protocol with given values
uint32_t createALP(uint32_t messageType, uint32_t quantity, uint32_t state, uint32_t action){

    uint32_t message = 0;
    message |= PROTOCOL_VERSION;
    message = message << 2;
    message |= messageType;
    message = message << 2;
    message |= OBJECT_TYPE;
    message = message << 4;
    message |= OBJECT_ID;
    message = message << 3;
    message |= LOGICAL_LOCATION;
    message = message << 10;
    message |= quantity;
    message = message << 1;
    message |= state;
    message = message << 1;
    message |= action;
    uint32_t temp=message;
    uint32_t parity=0;
    for(int i=0;i<26;i++){
        parity += (temp & 0x1);
        temp= temp >> 1;
    }
    
    message = message << 6;
    message |= parity;
    return message;
}

//Checks content of ALP message in terms of parity, version and if the message is right adressed
void checkALP(uint32_t message){
    uint32_t temp=message >> 6;
    uint32_t parity=0;
    for(int i=0;i<26;i++){
        parity += (temp & 0x1);
        temp= temp >> 1;
    }
    if(((message>>27) & 3)==2 && parity==(message & 63) && PROTOCOL_VERSION==((message>>29) & 7 ) && OBJECT_TYPE==((message>>25) & 3) && OBJECT_ID==((message>>21) & 15)&& LOGICAL_LOCATION==((message>>18) & 7)){
        if(((message>>6) & 1)==1){
            tap=((message>>6) & 1);
            Serial.print("TAP Open\n");
        }
        if(((message>>6) & 1)==0){
            tap=((message>>6) & 1);
            Serial.print("TAP Close\n");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.print(F("Zsut eth udp server init... ["));Serial.print(F(__FILE__));
    Serial.print(F(", "));Serial.print(F(__DATE__));Serial.print(F(", "));Serial.print(F(__TIME__));Serial.println(F("]")); 

    ZsutEthernet.begin(MAC);

    Serial.print(F("My IP address: "));
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        Serial.print(ZsutEthernet.localIP()[thisByte], DEC);Serial.print(F("."));
    }
    Serial.println();

    uint32_t regis = createALP(0,0,0,0);
    char mess[4];
    mess[0]= regis & 255;
    regis = regis >> 8;
    mess[1] = regis & 255;
    regis = regis >> 8;
    mess[2] = regis & 255;
    regis = regis >> 8;
    mess[3] = regis & 255;

    Udp.begin(localPort);
    Udp.beginPacket(ZsutIPAddress(192,168,56,101), 12345);
    Udp.write(mess, MAX_BUFFER_LENGTH);
    Udp.endPacket();
}

void loop() {
    int packetSize=Udp.parsePacket(); 
    if(packetSize>0){
        
        memset(packetBuffer, 0, MAX_BUFFER_LENGTH); 

        int len=Udp.read(packetBuffer, MAX_BUFFER_LENGTH);

        Serial.print("Recieved: ");
        uint32_t getting = 0;
        for(int j=MAX_BUFFER_LENGTH-1;j>=0;j--){
            getting |= (uint32_t)(packetBuffer[j]);
            if(j!=0){
                getting = getting << 8;
            }
        }
        Serial.print(getting);
        Serial.print("\n");
        checkALP(getting);
    }
}