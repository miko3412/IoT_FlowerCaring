#include <ZsutDhcp.h>           
#include <ZsutEthernet.h>       
#include <ZsutEthernetUdp.h>    
#include <ZsutFeatures.h>       


#define UDP_SERVER_PORT         12345
#define PERIOD       (5000UL)   

byte MAC[]={0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01}; 

#define PROTOCOL_VERSION 1
#define OBJECT_TYPE 2
#define OBJECT_ID 1
#define LOGICAL_LOCATION 2

unsigned int localPort=UDP_SERVER_PORT;   
unsigned long time; 

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

    time=ZsutMillis()+PERIOD;

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
    Udp.write(mess, 4);
    Udp.endPacket();
}

void loop() {

    uint32_t d = ZsutDigitalRead();
    uint32_t flooded=(d >> 13) & 0x1;

    if(time<ZsutMillis()){
        if(flooded){
            uint32_t report = createALP(1, 0, 1, 0);
            Serial.print("FLOODED!!  ");
            Serial.print(report);
            Serial.print("\n");
            char messr[4];
            messr[0]= report & 255;
            report = report >> 8;
            messr[1] = report & 255;
            report = report >> 8;
            messr[2] = report & 255;
            report = report >> 8;
            messr[3] = report & 255;
            Udp.beginPacket(ZsutIPAddress(192,168,56,101), 12345);
            Udp.write(messr, 4);
            Udp.endPacket();
            time=ZsutMillis()+PERIOD;
        }
        else{
            uint32_t report = createALP(1, 0, 0, 0);
            Serial.print("No water detected!  ");
            Serial.print(report);
            Serial.print("\n");
            char messr[4];
            messr[0]= report & 255;
            report = report >> 8;
            messr[1] = report & 255;
            report = report >> 8;
            messr[2] = report & 255;
            report = report >> 8;
            messr[3] = report & 255;
            Udp.beginPacket(ZsutIPAddress(192,168,56,101), 12345);
            Udp.write(messr, 4);
            Udp.endPacket();
            time=ZsutMillis()+PERIOD;
        }
    }
     
}