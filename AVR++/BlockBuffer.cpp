/* 
 * File:   BlockBuffer.cpp
 * Author: Cameron
 * 
 * Created on June 23, 2016, 1:32 PM
 */

#include <avr/interrupt.h>

#include "BlockBuffer.h"

template<u1 BlockSize, u1 Blocks, bool readInterrupt, bool writeInterrupt>
void BlockBuffer<BlockSize, Blocks, readInterrupt, writeInterrupt>::markCurrentWriteBufferAsDone() {
	if (!isWriteableNow())
		return;

	if (writeInterrupt) cli();

	u1 const curr = currentWrite;
	u1 nextWrite = curr;

	nextWrite++;

	if (nextWrite >= Blocks)
		nextWrite = 0;

	if (currentRead == 0xff)
		currentRead = curr;

	if (nextWrite == currentRead)
		nextWrite = 0xff;

	currentWrite = nextWrite;

	if (writeInterrupt) sei();
}

template<u1 BlockSize, u1 Blocks, bool readInterrupt, bool writeInterrupt>
void BlockBuffer<BlockSize, Blocks, readInterrupt, writeInterrupt>::markCurrentReadBufferAsDone() {
	if (!isReadableNow())
		return;

	if (readInterrupt) cli();

	u1 const curr = currentRead;
	u1 nextRead = curr;

	nextRead++;

	if (nextRead >= Blocks)
		nextRead = 0;

	if (currentWrite == 0xff)
		currentWrite = curr;

	if (nextRead == currentWrite)
		nextRead = 0xff;

	currentRead = nextRead;

	if (readInterrupt) sei();
}
