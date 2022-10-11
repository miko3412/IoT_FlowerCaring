#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PROTOCOL_VERSION 1
#define MAX_BUF 4
#define RULES 6
#define RULEBUFF 50
#define OBJECTS 10
#define LOCATIONS 3

typedef struct sender {
    int address;
    int message;
}sender;

typedef struct rules {
	char location;
	char subject;
	char cond;
	char numb[5];
	char action;
}dict;

typedef struct object{
    char type;
    char id;
    char location;
    char state;
    int address;
}object;

typedef struct temperature{
    char location;
    int temp;
}temperature;


dict rules[RULES];
object objects[OBJECTS];

//Initialize default temperatures in all locations
temperature temp[LOCATIONS] = {
    [0].location = '1',
    [0].temp = 0,
    [1].location = '2',
    [1].temp = 0,
    [2].location = '3',
    [2].temp = 0,
};

unsigned char t[MAX_BUF];
int a=0, objnr=0;

void printRules(dict values[]) {
	for (int i = 0; i < RULES; i++) {

		printf("IF (%c) %c %c %s THEN %c\n", values[i].location, values[i].subject, values[i].cond, values[i].numb, values[i].action);
	}
}

void printObjects(object objects[]) {
	for (int i = 0; i < OBJECTS; i++) {

		printf("Object id: %c, location: %c, type: %c, state: %c\n", objects[i].id, objects[i].location, objects[i].type, objects[i].state);
	}
}

void printTemp(temperature temperatures[]) {
	for (int i = 0; i < LOCATIONS; i++) {

		printf("Location: %c, temperature: %d\n", temperatures[i].location, temperatures[i].temp);
	}
}


//Reads rules from rulefile.csv and store them in array of structs
void readRules(){

    char buff[RULEBUFF];
	int row_count = 0;
	int field_count = 0;

    FILE* fp = fopen("rulefile.csv", "r");
	if (!fp) {
		printf("Cannot open rule file");
		return;
	}

    int i = 0;
    //tlumaczenie rule file
	while (fgets(buff, RULEBUFF, fp)){
		//printf("%s\n", buff);
		field_count = 0;
		row_count++;
		char* field = strtok(buff, ";");
		while (field) {
			if (field_count == 1) {
				switch (field[1])
				{
				case'B'://Bedroom
					rules[i].location='1';
					break;
				case'K'://Kitchen
					rules[i].location='2';
					break;
				case'L'://Livingroom
					rules[i].location='3';
					break;
				default:
					rules[i].location='0';
					break;
				}
			}
			if (field_count == 2) {
				switch (field[0])
				{
				case'q'://quantity
					rules[i].subject = '2';
					break;
				case's'://state
					if (field[1] == 'W' || field[8] == 'T' || field[5] == 'N') {
						rules[i].subject = '1';
					}
					else {
						rules[i].subject = '0';
					}
					break;
				case't'://time
					rules[i].subject = '3';
					break;
				default:
					rules[i].subject = '9';
					break;
				}
			}
			if (field_count == 3) {

				switch (field[0])
				{
				case'<':
					rules[i].cond = '<';
					break;
				case'>':
					rules[i].cond = '>';
					break;
				case'=':
					rules[i].cond = '=';
					break;
				default:
					rules[i].cond = ' ';
					break;
				}
			}
			if (field_count == 4) {
				strcpy(rules[i].numb, field);
			}
			if (field_count == 6) {

				switch (field[6])
				{
				case'N'://aHeatON
					rules[i].action = '1';
					break;
				case'F'://aHeatOff
					rules[i].action = '2';
					break;
				case'e'://aTapOpen
					rules[i].action = '3';
					break;
				case'o'://aTapClose
					rules[i].action = '4';
					break;
				default:
					rules[i].action = '0';
					break;
				}
			}

			field = strtok(NULL, ";");
			field_count++;
		}
		i++;
		
	}
	fclose(fp);
	printRules(rules);
}

//Creates ALP protocol with given values
uint32_t createALP(uint32_t objectType, uint32_t objectId, uint32_t logicalLocation, uint32_t messageType, uint32_t quantity, uint32_t state, uint32_t action){

    uint32_t message = 0;
    message |= PROTOCOL_VERSION;
    message = message << 2;
    message |= messageType;
    message = message << 2;
    message |= objectType;
    message = message << 4;
    message |= objectId;
    message = message << 3;
    message |= logicalLocation;
    message = message << 12;
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

//Checks message considering rules
struct sender checkMessage(char msgtype, char objtype, char objid, char location, char state,  int quantity, int address)
{
	struct sender sen;
	if (msgtype == '0') {

		//Struct Registration
		objects[objnr].type = objtype;
		objects[objnr].id = objid;
		objects[objnr].location = location;
		objects[objnr].state = state;
        objects[objnr].address = address;
		objnr++;
	}
	
	else if (msgtype == '1') {
		if (objtype == '1') {

			//Updating structures
			int objiterator = 0;
			int tempiterator = 0;
			while (objiterator < objnr) {
				if (objects[objiterator].type == '1' && objects[objiterator].location == location && objects[objiterator].id == objid) {
					objects[objiterator].state = state;
					break;
				}
				objiterator++;
			}
			
			while (tempiterator < LOCATIONS) {
				if (temp[tempiterator].location == location) {
					temp[tempiterator].temp = quantity;
					break;
				}
				tempiterator++;
			}

			//CHECK RULE FILE
			for (int i = 0; i < RULES; i++) {
				if (rules[i].subject == '2' && rules[i].location == location) {
					switch (rules[i].cond)
					{
					case '<':
						switch (rules[i].action)
						{
						case '1':
							if (temp[tempiterator].temp < atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
								printf("action aHeatON\n");
								sen.address=objects[objiterator].address;
								sen.message=message;
								return sen;
							}
							break;
						case '2':
							if (temp[tempiterator].temp < atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
								printf("action aHeatOFF\n");
								sen.address=objects[objiterator].address;
								sen.message=message;
								return sen;
							}
							break;
						case '3':
							if (temp[tempiterator].temp < atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
								printf("action aTapOpen\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '4':
							if (temp[tempiterator].temp < atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
								printf("action aTapClose\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						default:
							break;
						}

						break;
					case '>':
						switch (rules[i].action)
						{
						case '1':
							if (temp[tempiterator].temp > atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
								printf("action aHeatON\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '2':
							if (temp[tempiterator].temp > atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
								printf("action aHeatOFF\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '3':
							if (temp[tempiterator].temp > atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
								printf("action aTapOpen\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '4':
							if (temp[tempiterator].temp > atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
								printf("action aTapClose\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						default:
							break;
						}
						break;
					case '=':
						switch (rules[i].action)
						{
						case '1':
							if (temp[tempiterator].temp = atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
								printf("action aHeatON\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '2':
							if (temp[tempiterator].temp = atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
								printf("action aHeatOFF\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '3':
							if (temp[tempiterator].temp = atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
								printf("action aTapOpen\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						case '4':
							if (temp[tempiterator].temp = atoi(rules[i].numb)) {
								uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
								printf("action aTapClose\n");
								 sen.address=objects[objiterator].address;
								 sen.message=message;
								return sen;
							}
							break;
						default:
							break;
						}
						break;
					default:
						break;
					}
				}
			}

		}

		else if (objtype == '2') {

			//Updating Structs
			int objiterator = 0;

			while (objiterator < objnr) {
				if (objects[objiterator].type == '2' && objects[objiterator].location == location && objects[objiterator].id == objid) {
					objects[objiterator].state = state;
					break;
				}
				objiterator++;
			}

			int tapiterator = 0;
			int found = 0;

			while (tapiterator < objnr) {
				if (objects[tapiterator].type == '3' && objects[tapiterator].location == location && objects[tapiterator].id == objid) {
					found = 1;
					break;
				}
				tapiterator++;
			}

			if (found==0){
				sen.address=-1;
				sen.message=-1;
				return sen;
			}
			//CHECK RULE FILE
			for (int i = 0; i < RULES; i++) {
				if ((rules[i].subject == '0' || rules[i].subject == '1') && rules[i].location == location) {


					switch (rules[i].action)
					{
					case '1':
						if (objects[objiterator].state == rules[i].subject) {
							uint32_t message = createALP((uint32_t)(objects[tapiterator].type) - 48, (uint32_t)(objects[tapiterator].id) - 48, (uint32_t)(objects[tapiterator].location) - 48, 2,0,0,1);
							printf("action aHeatON\n");
							 sen.address=objects[tapiterator].address;
							 sen.message=message;
							return sen;
						}
						break;
					case '2':
						if (objects[objiterator].state == rules[i].subject) {
							uint32_t message = createALP((uint32_t)(objects[tapiterator].type) - 48, (uint32_t)(objects[tapiterator].id) - 48, (uint32_t)(objects[tapiterator].location) - 48, 2,0,0,0);
							printf("action aHeatOFF\n");
							sen.address=objects[tapiterator].address;
							sen.message=message;
							return sen;
						}
						break;
					case '3':
						if (objects[objiterator].state == rules[i].subject) {
							uint32_t message = createALP((uint32_t)(objects[tapiterator].type) - 48, (uint32_t)(objects[tapiterator].id) - 48, (uint32_t)(objects[tapiterator].location) - 48, 2,0,0,1);
							printf("action aTapOpen\n");
							objects[tapiterator].state = '3';
							 sen.address=objects[tapiterator].address;
							 sen.message=message;
							return sen;
						}
						break;
					case '4':
						if (objects[objiterator].state == rules[i].subject) {
							uint32_t message = createALP((uint32_t)(objects[tapiterator].type) - 48, (uint32_t)(objects[tapiterator].id) - 48, (uint32_t)(objects[tapiterator].location) - 48, 2,0,0,0);
							objects[tapiterator].state = '4';
							printf("action aTapClose\n");
							 sen.address=objects[tapiterator].address;
							 sen.message=message;
							return sen;
						}
						break;
					default:
						break;
					}
				}
			}
		}
		else {
			printf("Undefined Object\n");
		}
	}
	else {
		printf("Undefined message type\n");
	}
	sen.address=-1;
	sen.message=-1;
	return sen;

}

//Writes ALP messages into ALPlog.txt with timestamp
void writeLog(uint32_t alp){
	time_t timeNow = time(NULL);
	struct tm* tmp = gmtime(&timeNow);

	int h = (timeNow / 3600) % 24;
	int m = (timeNow / 60) % 60;
	int s = timeNow  % 60;

	uint32_t version=((alp>>29) & 7);
	uint32_t objectId = ((alp>>21) & 15);
	uint32_t logicalLocation = ((alp>>18) & 7);
	uint32_t messageType = ((alp>>27) & 3);
	uint32_t objectType = ((alp>>25) & 3);
	uint32_t quantity = ((alp>>8) & 1023);
	uint32_t state = ((alp>>7) & 1);
	uint32_t action = ((alp>>6)&1);

	unsigned char typson[10];

	if(messageType==0 || messageType==1){
		snprintf(typson, 11, "[Recieved]");
	}
	else{
		snprintf(typson, 11, "[Sent]    ");
	}

	FILE* fp;
	fp = fopen("ALPlog.txt", "a");

	fprintf(fp, "%s %02d:%02d:%02d ALP: %d -> Version: %d, ObjectID: %d, Location: %d, MessageType: %d, ObjectType: %d, Quantity: %d, State: %d, Action: %d\n", typson, h, m, s, alp, version, objectId, logicalLocation, messageType, objectType, quantity, state, action);
	fclose(fp);
}

//Checks content of ALP message in terms of parity and version
struct sender checkALP(uint32_t message, int addr){
    uint32_t objectId, logicalLocation, messageType, objectType, quantity, state;
    uint32_t temp=message >> 6;
    uint32_t parity=0;
    for(int i=0;i<26;i++){
        parity += (temp & 0x1);
        temp= temp >> 1;
    }
    
    if(parity==(message & 63) && PROTOCOL_VERSION==((message>>29) & 7)){


        objectId = ((message>>21) & 15) + 48 ;
        logicalLocation = ((message>>18) & 7) + 48;
        messageType = ((message>>27) & 3) + 48;
        objectType = ((message>>25) & 3) + 48;
        quantity = ((message>>8) & 1023);
        state = ((message>>7) & 1) + 48;
        
		return checkMessage((char)messageType, (char)objectType, (char)objectId, (char) logicalLocation, (char)state, quantity, addr);

    }
    else{
        printf("ERROR: Version or/and Parity not matching \n");
		struct sender sen;
		 sen.address=-1;
		 sen.message=-1;
		return sen;
    }

}


int main()
{
    struct sender sen;
    struct addrinfo h, *r;
    struct sockaddr_in clients[10];
    int s, pos, len=sizeof(clients[0]);
    uint32_t recieved;
    unsigned char
    m[MAX_BUF], t[MAX_BUF];
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec=0;
    tv.tv_usec=5000;
    int d, index=0;
	int counter, lastcounter=0;
	time_t tim = time(NULL);
	struct tm *tmp = gmtime(&tim);

    
    readRules();
    memset(&h, 0, sizeof(struct addrinfo));
    h.ai_family=PF_INET; h.ai_socktype=SOCK_DGRAM; h.ai_flags=AI_PASSIVE;
    if(getaddrinfo(NULL, "12345", &h, &r)!=0){
        printf("Error gettingAddress...");
    }
    if((s=socket(r->ai_family, r->ai_socktype, r->ai_protocol))==-1){
        printf("Error getting socket...");
    }
    if(bind(s, r->ai_addr, r->ai_addrlen)!=0){
        printf("Error binding...");
    }

    for(;;){
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);
		tim = time(NULL);
		counter = ((tim / 3600) % 24)*60 + (tim / 60) % 60;
		if(counter!=lastcounter){
			lastcounter=counter;
			for (int i = 0; i < RULES; i++) {
				if (rules[i].subject == '3') {
					switch (rules[i].action)
					{
					case '1':
						if (counter==atoi(rules[i].numb)) {
							int objiterator = 0;
							while (objiterator < objnr) {
								if (objects[objiterator].type == '1' && objects[objiterator].location == rules[i].location) {
									objects[objiterator].state = '1';
									uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
									printf("action HeatON\n");
									uint8_t mess[4];
									uint32_t tempo=message;
									mess[0]= tempo & 255;
									tempo = tempo >> 8;
									mess[1] = tempo & 255;
									tempo = tempo >> 8;
									mess[2] = tempo & 255;
									tempo = tempo >> 8;
									mess[3] = tempo & 255;
									t[0]=mess[0];
									t[1]=mess[1];
									t[2]=mess[2];
									t[3]=mess[3];
									writeLog(message);
									len=sizeof(clients[(objects[objiterator].address)]);
									if((pos=sendto(s, t, MAX_BUF, 0, (const struct sockaddr *)&clients[(objects[objiterator].address)], len ))<0){
										printf("ERROR: %s\n", strerror(errno));
									}

								}
								objiterator++;
							}
						}
						break;
					case '2':
						if (counter==atoi(rules[i].numb)) {
							int objiterator = 0;
							while (objiterator < objnr) {
								if (objects[objiterator].type == '1' && objects[objiterator].location == rules[i].location) {
									objects[objiterator].state = '2';
									uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
									printf("action HeatOFF\n");
									uint8_t mess[4];
									uint32_t tempo=message;
									mess[0]= tempo & 255;
									tempo = tempo >> 8;
									mess[1] = tempo & 255;
									tempo = tempo >> 8;
									mess[2] = tempo & 255;
									tempo = tempo >> 8;
									mess[3] = tempo & 255;
									t[0]=mess[0];
									t[1]=mess[1];
									t[2]=mess[2];
									t[3]=mess[3];
									writeLog(message);
									len=sizeof(clients[(objects[objiterator].address)]);
									if((pos=sendto(s, t, MAX_BUF, 0, (const struct sockaddr *)&clients[(objects[objiterator].address)], len ))<0){
										printf("ERROR: %s\n", strerror(errno));
									}

								}
								objiterator++;
							}
						}
						break;
					case '3':
						if (counter==atoi(rules[i].numb)) {
							int objiterator = 0;
							while (objiterator < objnr) {
								if (objects[objiterator].type == '3' && objects[objiterator].location == rules[i].location) {
									objects[objiterator].state = '3';
									uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,1);
									printf("action TapON\n");
									uint8_t mess[4];
									uint32_t tempo=message;
									mess[0]= tempo & 255;
									tempo = tempo >> 8;
									mess[1] = tempo & 255;
									tempo = tempo >> 8;
									mess[2] = tempo & 255;
									tempo = tempo >> 8;
									mess[3] = tempo & 255;
									t[0]=mess[0];
									t[1]=mess[1];
									t[2]=mess[2];
									t[3]=mess[3];
									writeLog(message);
									len=sizeof(clients[(objects[objiterator].address)]);
									if((pos=sendto(s, t, MAX_BUF, 0, (const struct sockaddr *)&clients[(objects[objiterator].address)], len ))<0){
										printf("ERROR: %s\n", strerror(errno));
									}

								}
								objiterator++;
							}
						}
						break;
					case '4':
						if (counter==atoi(rules[i].numb)) {
							int objiterator = 0;
							while (objiterator < objnr) {
								if (objects[objiterator].type == '3' && objects[objiterator].location == rules[i].location) {
									objects[objiterator].state = '4';
									uint32_t message = createALP((uint32_t)(objects[objiterator].type) - 48, (uint32_t)(objects[objiterator].id) - 48, (uint32_t)(objects[objiterator].location) - 48, 2,0,0,0);
									printf("action TapOFF\n");
									uint8_t mess[4];
									uint32_t tempo=message;
									mess[0]= tempo & 255;
									tempo = tempo >> 8;
									mess[1] = tempo & 255;
									tempo = tempo >> 8;
									mess[2] = tempo & 255;
									tempo = tempo >> 8;
									mess[3] = tempo & 255;
									t[0]=mess[0];
									t[1]=mess[1];
									t[2]=mess[2];
									t[3]=mess[3];
									writeLog(message);
									len=sizeof(clients[(objects[objiterator].address)]);
									if((pos=sendto(s, t, MAX_BUF, 0, (const struct sockaddr *)&clients[(objects[objiterator].address)], len ))<0){
										printf("ERROR: %s\n", strerror(errno));
									}

								}
								objiterator++;
							}
						}
						break;
					default:
						break;
					
					}
				}
			}
		}
        
        if((d=select(s+1, &readfds, NULL, NULL, &tv))<0){
            printf("ERROR: %s\n", strerror(errno));
        }
        else if(d>0){
            if(FD_ISSET(s, &readfds)){
                len=sizeof((clients[index]));
                if((pos=recvfrom(s, m, MAX_BUF, 0, (struct sockaddr*)&(clients[index]), &len ))<0){
                    printf("ERROR: %s\n", strerror(errno));exit(-4);
                }
                //printf("[%d]Recv(%s:%d): Index:%d Lenght:%d pos:%d\n", __LINE__, inet_ntoa(clients[index].sin_addr), ntohs(clients[index].sin_port), index, len, pos);
                m[pos]='\0';
		        recieved=0;
                for(int j=MAX_BUF-1;j>=0;j--){
                    recieved |= (uint32_t)(m[j]);
                    if(j!=0){
                        recieved = recieved << 8;
                    }
                }
                writeLog(recieved);
                sen= checkALP(recieved, index);
                
                if(((recieved>>27) & 3)==0){
                    index++;
                }
                if (sen.address!=-1){
                    uint8_t mess[4];
                    uint32_t tempo=sen.message;
                    mess[0]= tempo & 255;
                    tempo = tempo >> 8;
                    mess[1] = tempo & 255;
                    tempo = tempo >> 8;
                    mess[2] = tempo & 255;
                    tempo = tempo >> 8;
                    mess[3] = tempo & 255;
                    t[0]=mess[0];
                    t[1]=mess[1];
                    t[2]=mess[2];
                    t[3]=mess[3];
					writeLog(sen.message);
                    len=sizeof(clients[sen.address]);
                    //printf("[%d]Send to(%s:%d): Index:%d Lenght:%d\n", __LINE__, inet_ntoa(clients[sen.address].sin_addr), ntohs(clients[sen.address].sin_port), sen.address, len);
                    if((pos=sendto(s, t, MAX_BUF, 0, (const struct sockaddr *)&clients[sen.address], len ))<0){
                        printf("ERROR: %s\n", strerror(errno));
                    }
                }
            }
        }
	    
    }
    freeaddrinfo(r);
    close(s);
}

