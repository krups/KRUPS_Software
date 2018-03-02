/*
 * Basic control flow functions for main code
 * Functions in this header complete a simple task that happens 
 * repeatedley throughout the code base
 * 
 * Author: Collin Dietz
 * Email: c4dietz@gmail.com
 */

#ifndef CONTROL_H
#define CONTROL_H


#include "Packet.h"
#include "Config.h"

/*
 * Prints a message over Serial if the settings allow it w/o newline
 */
template<typename T>
void printMessage(T s)
{
    #if OUTPUT_MESSAGES
      Serial.print(s);
    #endif
}

/*
 * Prints a message over Serial if the settings allow it w newline
 */
template<typename T>
void printMessageln(T s)
{
    #if OUTPUT_MESSAGES
      Serial.println(s);
    #endif
}

/*
 * Prints a newline over Serial if the settings allow it w newline
 */
void printMessageln()
{
    #if OUTPUT_MESSAGES
      Serial.println();
    #endif
}

#endif