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

unsigned char receive_buffer[MAX_BUFFER];
unsigned char send_buffer[MAX_BUFFER];
unsigned int current_send = 0;
unsigned int current_index = 0;
unsigned int current_read = 0;
unsigned int size = 0;

struct gps_coords my_coord;

char *phone_number;

char read_buffer[MAX_BUFFER];

int set_phone_number(char *phone_num)
{
    phone_number = phone_num;
}

void send_sms(char *message)
{
    //send at command to put it in SMS mode
    send_check_reply(AT_SMS_MODE, AT_OK);

    //Code taken from Adafruit FONA library
    char sendcmd[30] = "AT+CMGS=\"";
    strncpy(sendcmd+9, phone_number, 10);  // 9 bytes beginning, 2 bytes for close quote + null
    sendcmd[strlen(sendcmd)] = '\"';
    sendcmd[strlen(sendcmd)] = '\r';

    //wait for mark >
    send_check_reply(sendcmd, AT_MARK);
    send(message);

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
    int comma=0,j=0,i;
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
            gps_coord->lat[j++] = read_buffer[i];
            gps_coord->lat[j] = '\0';
        }

        if(comma==2)
        {
            gps_coord->lon[j++] = read_buffer[i];
            gps_coord->lon[j] = '\0';
        }

        if(comma>2)
            break;
    }
}

int initialize_fona()
{
    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for first AT\n");
    else
        printf("Got no reply for first AT\n");

    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for second AT\n");
    else
        printf("Got no reply for second AT\n");

    if (send_check_reply(AT, AT_RESPONSE))
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
        printf("GPS powered on\n");
    else
        printf("Couldn't power GPS on\n");

    if (send_check_reply(AT_GPSRST_WRM, AT_OK))
        printf("GPS reset: warm\n");
    else
        printf("Couldn't reset GPS\n");

    if (send_check_reply(AT_GPSOUT, AT_OK))
        printf("GPS output set to 1\n");
    else
        printf("Couldn't set GPS output\n");

    get_gps(&my_coord);
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

    if (strncmp(read_buffer, reply, num_read) == 0)
       return 1;
    else
       return 0;
}
