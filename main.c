//
// ir_receiver
// Receive NEC IR codes from a demodulator connected to a GPIO pin
//
// Written by Larry Bank - 4/15/2018
// Copyright (c) 2018 BitBank Software, Inc.
// bitbank@pobox.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <armbianio.h>

static int iIRPin = IR_PIN;
static int fdui; // file handle for uinput

// These are the NEC codes produced from keys 0-9 on my cheap little
// ir remote which was included in some kit
#define NUM_KEYS 10
static uint32_t u32Codes[NUM_KEYS] = {0xff6897, 0xff30cf, 0xff18e7, 0xff7a85,
	0xff10ef, 0xff38c7, 0xff5aa5, 0xff42bd, 0xff4ab5, 0xff52ad};
static int iKeys[NUM_KEYS] = {11,2,3,4,5,6,7,8,9,10}; // keys 0-9
//
// Measure the amount of time the IR receiver is in its current state
// Returns an integer representing the number of microseconds
//
int IRWait(int iPin, int iState)
{
int iTime, iTick;
struct timespec ts, ts1;

   clock_gettime(0, &ts);
   iTick = 0;
   while (AIOReadGPIO(iPin) == iState && iTick < 10000000)
   {
      iTick++;
   }
   if (iTick >= 10000000) // timeout
      return 20000; // return an invalid time
   clock_gettime(0, &ts1);
   iTime = (int)((ts1.tv_nsec - ts.tv_nsec) / 1000UL); // convert ns to us
   return iTime;
} /* IRWait() */
//
// Capture an N-bit IR code
// Measures the signal+gaps and accumulates the bits (LSB first)
// until a long gap or 32-bits are captured (whichever comes first)
//
uint32_t IRGetCode(int iPin, int *pBits)
{
int i, left, right;
uint32_t u32;

    i = 0; // assume all is well and we'll capture a code
    // Look for IR leader (roughly 9ms on and 4.5ms off)
      left = IRWait(iPin, 0); // time in this state (active)
//      if (left < 7500 || left > 10000) // some kind of glitch (< 7.5ms || > 10ms) reject it
      if (0)
      {
         i = 33; // skip the code search
      }
      else
      {
         right = IRWait(iPin, 1);
         if (right < 3000 || right > 6000) // some kind of glitch (< 3ms || > 6ms) reject it
            i = 33; // skip the code search
      }
    // Capture up to 32 data bits
    u32 = 0L;
    for (; i<32; i++)
    {
      u32 <<= 1L; // code is received LSB to MSB
      left = IRWait(iPin, 0); // time active
      right = IRWait(iPin, 1); // time inactive
      // check for short/long period on the low/high pulses
      if (left < 400 || left > 750 || right < 400) // something glitched
      {
          i = 33; // indicate an invalid code
          continue;
      }
      if (right >= 350 && right < 750) // 560/560 = 0
      { } //   u32 |= 0L; // a zero
      else if (right >= 1350 && right < 1950) // 560/1690 = 1
         u32 |= 1L;
      else if (right > 2000) // stop bit
         break;
    } // for i
    
    // Return length and value of IR code
    *pBits = i;
    return u32; 
} /* IRGetCode() */
//
// Program entry point
//
int main(int argc, char *argv[])
{
struct timespec res;
int iLen, i;
uint32_t u32;
struct uinput_user_dev uidev;

   clock_getres(0, &res);
   if (res.tv_nsec > 25000) // if resolution worse than 25us, abort
   {
      printf("system clock resolution is too low: %ld ns\n", res.tv_nsec);
      return -1;
   }
// Set up the keypress simulator device
   fdui = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
   if (fdui < 0)
   {
      fprintf(stderr, "Error opening /dev/uinput device\n");
      return -1;
   }
   ioctl(fdui, UI_SET_EVBIT, EV_KEY);
   for (i=0; i<NUM_KEYS; i++)
   {
      ioctl(fdui, UI_SET_KEYBIT, iKeys[i]); // enable each key we will use
   }
   memset(&uidev, 0, sizeof(uidev));
   snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "IR receiver keyboard");
   uidev.id.bustype = BUS_VIRTUAL;
   write(fdui, &uidev, sizeof(uidev));
   if (ioctl(fdui, UI_DEV_CREATE) < 0)
   {
      fprintf(stderr, "Error creating virtual keyboard device\n");
      return -1;
   }
   if (!AIOInit())
   {
      printf("Unable to initialize ArmbianIO library\n");
      return -1;
   }
   if (AIOAddGPIO(iIRPin, GPIO_IN) != 1)
   {
      fprintf(stderr, "Error registering pin %d as an output\n", iIRPin);
      return -1;
   }
   while (1)
   {
      while (AIOReadGPIO(iIRPin) != 0)
      {
         usleep(1000); // go easy on the CPU :)
      }
      u32 = IRGetCode(iIRPin, &iLen);
      if (iLen <= 32)
      {
         printf("code received = %08x\n", u32);
         for (i=0; i<NUM_KEYS; i++)
         {
            if (u32 == u32Codes[i]) // found it
            {
            struct input_event ie;
            // press the key corresponding to that code
               ie.value = 1; // send press
               ie.type = EV_KEY; // key event
               ie.code = iKeys[i];
               write(fdui, &ie, sizeof(ie)); // send
               ie.value = 0; // key release
               write(fdui, &ie, sizeof(ie));
               ie.type = EV_SYN;
               ie.code = SYN_REPORT;
               ie.value = 0;
               write(fdui, &ie, sizeof(ie)); // send a report
            } // found code
         } // for i
      } // if valid code
   } // while

  return 0;
} /* main() */
