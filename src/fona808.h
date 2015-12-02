#ifndef FONA808_H_
#define FONA808_H_


#define AT          "AT\r"
#define AT_OK       "\r\nOK\r\n"
#define AT_RESPONSE "AT\r\r\nOK\r\n"
#define AT_ECHO_OFF "ATE0\r"
#define AT_SMS_MODE "AT+CMGF=1\r"
#define AT_SEND_MSG "AT+CMGS=\""
#define AT_MARK     "\r\n>"
#define AT_GPSPWR_ON    "AT+CGPSPWR=1\r"  //Response: OK
#define AT_GPSRST_WRM   "AT+CGPSRST=2\r"  //Response: OK
#define AT_GPSINFO      "AT+CGPSINF=0\r"  //Response:
#define AT_GPSOUT       "AT+CGPSOUT=1\r"  //Response: OK

struct gps_coords
{
    char lat[16];
    char lon[16];
    char time[30];
};

char * get_location();

void send_sms(char *message);

static void send(char *send);

int set_phone_number(char *phone_number);

int initialize_fona();

static void busy_wait();

static int read(unsigned char *buffer);
static int send_check_reply(const char* send, const char* reply);

#endif /* FONA808_H_ */
