#ifndef FONA808_H_
#define FONA808_H_

#define AT 				"AT\r"
#define ATINFO 			"ATI\r"
#define MODULE_NAME 	"SIM800 R13.08"
#define ATECHO_ON 		"ATE1\r"
#define ATECHO_OFF 		"ATE0\r"
#define ATGPS_ON 		"AT\rAT+CGPSPWR=1\r"
#define ATGPS_OFF 		"AT\rAT+CGPSPWR=0\r"
#define ATGPS_INFO 		"AT+CGPSINF=0\r"
#define ATGPS_STATUS 	"AT+CGPSSTATUS?\r"
#define ATSIM 			"AT+CCID\r"  //SIM number
#define ATNETWORK 		"AT+COPS?\r"  //Get network
#define ATBAT 			"AT+CBC\r"
#define ATTEXT_MODE 	"AT+CMGF=1\r"
#define ATSMS 			"ATCMGS=\""

char * get_location();

/*
 * phone number can only send a text to a number that's 11 digits, check for validity
 * Return 0 if everything went well and you got a confirmation the sms was sent, otherwise
 * send back 1.
 */
int send_sms(char* phone_number, char* message);



#endif /* FONA808_H_ */
