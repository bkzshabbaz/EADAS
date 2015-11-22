#ifndef FONA808_H_
#define FONA808_H_


#define AT          "AT\r"
#define AT_OK       "\r\nOK\r\n"
#define AT_RESPONSE "AT\r\r\nOK\r\n"
#define AT_ECHO_OFF "ATE0\r"
#define AT_SMS_MODE "AT+CMGF=1\r"
#define AT_SEND_MSG "AT+CMGS=\""
#define AT_MARK     "\r\n>"

char * get_location();

void send_sms(char *message);

static void send(char *send);

int set_phone_number(char *phone_number);

int initialize_fona();

static void busy_wait();

static int read(unsigned char *buffer);
static int send_check_reply(const char* send, const char* reply);

#endif /* FONA808_H_ */
