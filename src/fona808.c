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

unsigned char receive_buffer[100];
unsigned char send_buffer[100];
unsigned int current_send = 0;
unsigned int current_index = 0;
unsigned int current_read = 0;
unsigned int size = 0;

char *phone_number;

char read_buffer[100];

int set_phone_number(char *phone_num)
{
    phone_number = phone_num;
}

void send_sms(char *message)
{
    //send at command to put it in SMS mode
    send_check_reply(AT_SMS_MODE, AT_OK);
    //wait for mark >

    //Code taken from Adafruit FONA library
    char sendcmd[30] = "AT+CMGS=\"";
    strncpy(sendcmd+9, phone_number, 10);  // 9 bytes beginning, 2 bytes for close quote + null
    sendcmd[strlen(sendcmd)] = '\"';
    sendcmd[strlen(sendcmd)] = '\r';

    send_check_reply(sendcmd, AT_MARK);
    send(message);
    //busy_wait();

    busy_wait();
    char endcmd[3];
    endcmd[0] = 0x1A;
    endcmd[1] = '\r';
    endcmd[2] = '\0';
    send(endcmd);
    busy_wait();

    //wait for OK? nah, FIRE AND FORGET!
}

int initialize_fona()
{
    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for first AT\n");
    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for first AT\n");
    if (send_check_reply(AT, AT_RESPONSE))
        printf("Got a good reply for first AT\n");

    if (send_check_reply(AT_ECHO_OFF, AT_ECHO_OFF AT_OK))
        printf("Echo turned off successfully\n");

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

    read(read_buffer);

    if (strcmp(read_buffer, reply) == 0)
       return 1;
    else
       return 0;
}
