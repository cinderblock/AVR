/* 
 * File:   BlockBuffer.h
 * Author: Cameron
 *
 * Created on June 23, 2016, 1:32 PM
 */

#ifndef BLOCKBUFFER_H
#define	BLOCKBUFFER_H

#include "basicTypes.h"

using namespace AVR;

template <u1 BlockSize, u1 Blocks, bool readInterrupt, bool writeInterrupt = !readInterrupt>
class BlockBuffer {
		u1 buffers[Blocks][BlockSize];
		/**
			* Index of which buffer we're currently writing to. A special value of 0xff
			* is used to indicate no room is available
			*/
		volatile u1 currentWrite;

		/**
			* Index of which buffer we're currently reading from. A special value of 0xff
			* is used to indicate no data is available
			*/
		volatile u1 currentRead;

	public:

		inline BlockBuffer() : currentWrite(0), currentRead(0xff) {
				static_assert(Blocks > 0, "Must use a positive, non-zero number of blocks");
				static_assert(Blocks != 255, "Maximum 254 blocks");
		}

		/**
			* Reserve the newest block for reading
			* @return
			*/
		void markCurrentReadBufferAsDone();

		/**
			* Mark the buffer the writer is using as the newest, indicating that data is
			* available and allowing the reader to access the next block
			* @return
			*/
		void markCurrentWriteBufferAsDone();

		/**
			* Check if there is new data available for the reader side
			* @return
			*/
		inline bool isReadableNow() const {
				return currentRead < Blocks;
		}

		/**
			* Check if there is room to write another block to the buffer
			* @return
			*/
		inline bool isWriteableNow() const {
				return currentWrite < Blocks;
		}

		/**
			* Get the buffer that should be written to
			* @return
			*/
		u1 * getWriteBuffer() {
				return isWriteableNow() ? buffers[currentWrite] : nullptr;
		}

		/**
			* Get the buffer that should be read from
			* @return
			*/
		u1 * getReadBuffer() {
				return isReadableNow() ? buffers[currentRead] : nullptr;
		}

		u1 constexpr getBlocks() {
				return Blocks;
		}

		u1 constexpr getBlockSize() {
				return BlockSize;
		}

		void fixInvaildState(){
			if(!isWriteableNow() && !isReadableNow()){
				currentWrite = 0;
			}
		}
};

#endif	/* BLOCKBUFFER_H */

