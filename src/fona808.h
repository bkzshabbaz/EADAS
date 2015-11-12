#ifndef FONA808_H_
#define FONA808_H_

char * get_location();

/*
 * phone number can only send a text to a number that's 11 digits, check for validity
 * Return 0 if everything went well and you got a confirmation the sms was sent, otherwise
 * send back 1.
 */
int send_sms(char* phone_number, char* message);



#endif /* FONA808_H_ */
