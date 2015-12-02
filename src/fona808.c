/*
 * fona.c
 *
 *  Created on: Nov 21, 2015
 *      Author: sammy
 */
#include "usci.h"
#include "fona808.h"
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER 256

char sms_buffer[160];

volatile unsigned char receive_buffer[MAX_BUFFER];
volatile unsigned char send_buffer[MAX_BUFFER];
volatile unsigned int current_send = 0;
volatile unsigned int current_index = 0;
volatile unsigned int current_read = 0;
volatile unsigned int size = 0;

char *phone_number;

struct gps_coords my_coord;

char read_buffer[MAX_BUFFER];

int set_phone_number(char *phone_num)
{
    phone_number = phone_num;
}

void send_sms(char *message)
{
    get_gps(&my_coord);
    //send at command to put it in SMS mode
    send_check_reply(AT_SMS_MODE, AT_OK);

    //Code taken from Adafruit FONA library
    char sendcmd[30] = "AT+CMGS=\"";
    strncpy(sendcmd+9, phone_number, 10);  // 9 bytes beginning, 2 bytes for close quote + null
    sendcmd[strlen(sendcmd)] = '\"';
    sendcmd[strlen(sendcmd)] = '\r';

    //wait for mark >
    send_check_reply(sendcmd, AT_MARK);
    sprintf(sms_buffer, "%s Location - Lat: %s Long: %s Time: %s\r",
            message, my_coord.lat,
            my_coord.lon, my_coord.time);
    send(sms_buffer);
    busy_wait();
    busy_wait();
    busy_wait();
    char endcmd[3];
    endcmd[0] = 0x1A;
    endcmd[1] = '\r';
    endcmd[2] = '\0';
    send(endcmd);
    busy_wait();

    //wait for OK? nah, FIRE AND FORGET!
}

static int get_gps(struct gps_coords *gps_coord)
{
    transmit_uart(AT_GPSINFO);

    busy_wait();
    busy_wait();
    busy_wait();
    busy_wait();
    int num_read = read(read_buffer);
    unsigned int comma=0,j=0,i;
    for(i=0; read_buffer[i] != '\0' && i < num_read;i++)
    {
        if(read_buffer[i]==',')
        {
            comma++;
            i++;
            j=0;
        }

        if(comma<1) continue;

        if(comma==1)
        {
            if (j==2) {
                gps_coord->lat[j++] = ' ';
            }
            gps_coord->lat[j++] = read_buffer[i];
            gps_coord->lat[j] = ' ';
            gps_coord->lat[j+1] = 'N';
            gps_coord->lat[j+2] = '\0';

        }

        if(comma==2)
        {
            if (j==2) {
                gps_coord->lon[j++] = ' ';
            }
            gps_coord->lon[j++] = read_buffer[i];
            gps_coord->lon[j] = ' ';
            gps_coord->lon[j+1] = 'W';
            gps_coord->lon[j+2] = '\0';
        }

        if(comma==4)
        {
           if(j==4 || j==7)
           {
               gps_coord->time[j++]='-';
           }

           if(j==10)
           {
               gps_coord->time[j++]=' ';
           }

           if(j==13 || j==16)
           {
               gps_coord->time[j++]=':';
           }

           gps_coord->time[j++]=read_buffer[i];
           gps_coord->time[j+1] = ' ';
           gps_coord->time[j+2] = 'U';
           gps_coord->time[j+3] = 'T';
           gps_coord->time[j+4] = 'C';

           gps_coord->time[j+5]='\0';
        }

        if(comma>4)
            break;
    }
}

int initialize_fona()
{
    int errors = 0;
    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for first AT\n");
    else
        printf("Got no reply for first AT\n");

    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for second AT\n");
    else
        printf("Got no reply for second AT\n");

    if (send_check_reply("ATZ\r", AT_RESPONSE))
        printf("Got a good reply for third AT\n");
    else
        printf("Got no reply for third AT\n");

    if (send_check_reply(AT_ECHO_OFF, AT_ECHO_OFF AT_OK))
        printf("Echo turned off successfully\n");
    else
        printf("Could not turn off echo\n");

    if (send_check_reply(AT, AT_OK))
        printf("Got a good reply for fourth AT\n");
    else
        printf("Got no reply for fourth AT\n");

    if (send_check_reply(AT, AT_OK))
        printf("Got a good reply for fifth AT\n");
    else
        printf("Got no reply for fifth AT\n");
    /*
#define AT_GPSPWR_ON    "AT+CGPSPWR=1"  //Response: OK
#define AT_GPSRST_WRM   "AT+CGPSRST=2"  //Response: OK
#define AT_GPSINFO      "AT+CGPSINF=0"  //Response:
#define AT_GPSOUT       "AT+CGPSOUT=1"  //Response: OK
     */
    if (send_check_reply(AT_GPSPWR_ON, AT_OK))
        errors++;
        //printf("GPS powered on\n");
    else
        printf("Couldn't power GPS on\n");

    if (send_check_reply(AT, AT_OK))
        errors++;
        //printf("Got a good reply for sixth AT\n");
    else
        printf("Got no reply for sixth AT\n");

    if (send_check_reply(AT_GPSOUT, AT_OK))
        errors++;
        //printf("GPS output set to 1\n");
    else
        printf("Couldn't set GPS output\n");

    if (send_check_reply(AT, AT_OK))
        errors++;
        //printf("Got a good reply for seventh AT\n");
    else
        printf("Got no reply for seventh AT\n");

    return 0;
}

static int read(unsigned char *buffer)
{
    int read = 0;
    if (size == 0) return read;

    //Read max contents in to buffer
    while (current_read < current_index) {
        *buffer++ = receive_buffer[current_read++];
        read++;
    }
    *buffer = '\0';
    current_read = 0;
    current_index = 0;
    size = 0;
    return read;
}

static void send(char *send)
{
    transmit_uart(send);
}

static void busy_wait()
{
    //Busy wait for a response.
   volatile int j = 0;
   int i;
   for (i = 0; i < 10000;i++ )
   {
      j++;
   }
}

static int send_check_reply(const char* send, const char* reply)
{
    transmit_uart(send);

    busy_wait();

    int num_read = read(read_buffer);

    if (strcmp(read_buffer, reply) == 0)
       return 1;
    else
       return 0;
}
