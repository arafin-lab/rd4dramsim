/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland
*                             dramninjas [at] umd [dot] edu
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/



#include "Bank.h"
#include "BusPacket.h"
#include <string.h> // for memcpy
#include <assert.h>


namespace DRAMSim
{
	using namespace std;


	int Bank::READ(BusPacket *busPacket)
	{
		if (busPacket->busPacketType == BusPacket::READ)
		{
			busPacket->busPacketType = BusPacket::DATA;
		}
#ifdef ICDP_PRE_WRITE
		else
		{
				busPacket->busPacketType = BusPacket::REG_DATA;
		}
#endif
		return 0;
	}

	int Bank::WRITE(BusPacket *busPacket)
	{
		return 0;
	}

	unsigned Bank::getByteOffsetInRow(const BusPacket *busPacket)
	{
		// This offset will simply be all of the bits in the unaligned address that
		// have been removed (i.e. the lower 6 bits for BL=8 and the lower 5 bits
		// for BL=4) plus the column offset


#ifdef DATA_RELIABILITY
		unsigned transOffset = 0;
#else
		uint64_t transactionMask = TRANS_DATA_BYTES - 1; //ex: (64 bit bus width) x (8 Burst Length) - 1 = 64 bytes - 1 = 63 = 0x3f mask
		unsigned transOffset = busPacket->data->getAddr() & transactionMask;
#endif

		unsigned columnOffset = busPacket->column * TRANS_TOTAL_BYTES/BL;
		unsigned byteOffset = columnOffset + transOffset;

		//FIXME: DEBUG DATA
		//DEBUG("[DPKT] "<< *(busPacket->data) << " \t r="<<busPacket->row<<" c="<<busPacket->column<<" transOffset="<< transOffset<< "  --> "<< byteOffset);

		return byteOffset;
	}

	void Bank::write(const BusPacket *busPacket)
	{
		unsigned byteOffset = getByteOffsetInRow(busPacket);
		RowMapType::iterator it;
		it = rowEntries.find(busPacket->row);
		byte *rowData;
		if (it == rowEntries.end())
		{
			// row doesn't exist yet, allocate it
			rowData = (byte *)calloc((NUM_COLS * TRANS_TOTAL_BYTES/BL),sizeof(byte));
			rowEntries[busPacket->row] = rowData;
		}
		else
		{
			rowData = it->second;
		}


	#ifdef DATA_RELIABILITY
		size_t transactionSize = TRANS_TOTAL_BYTES;
	#else
		size_t transactionSize = busPacket->data->getNumBytes();
	#endif

		// if we out of bound a row, this is a problem
		if (byteOffset + transactionSize > NUM_COLS * TRANS_TOTAL_BYTES/BL)
		{
			ERROR("Transaction out of bounds a row, check alignment of the address");
			exit(-1);
		}
		// copy data to the row
		memcpy(rowData + byteOffset, busPacket->data->getData(), transactionSize);
		delete busPacket->data;
	}


	void Bank::read(BusPacket *busPacket)
	{
		assert(busPacket->data == NULL);

		unsigned byteOffset = 0;
		//unsigned transOffset = 0;

#ifdef DATA_RELIABILITY
		unsigned transOffset = 0;
#else
		uint64_t transactionMask = TRANS_DATA_BYTES - 1; //ex: (64 bit bus width) x (8 Burst Length) - 1 = 64 bytes - 1 = 63 = 0x3f mask
		unsigned transOffset = busPacket->physicalAddress & transactionMask;
#endif

		busPacket->busPacketType = BusPacket::DATA;
		busPacket->data = new DataPacket(NULL, SUBRANK_DATA_BYTES*busPacket->len, busPacket->physicalAddress);

		RowMapType::iterator it = rowEntries.find(busPacket->row);
		if (it != rowEntries.end())
		{
			byte *rowData = it->second;
			byte *dataBuf = (byte *)calloc(sizeof(byte), SUBRANK_DATA_BYTES*busPacket->len);
			byteOffset = busPacket->column * TRANS_TOTAL_BYTES/BL + transOffset;
			memcpy(dataBuf, rowData + byteOffset, SUBRANK_DATA_BYTES*busPacket->len);
			busPacket->data->setData(dataBuf, SUBRANK_DATA_BYTES*busPacket->len, false);
			//FIXME: DEBUG DATA
			//DEBUG("[DPKT] Rank returning: "<<*(busPacket->data) << " \t r="<<busPacket->row<<" c="<<busPacket->column<<" transOffset="<< transOffset << "  --> "<< byteOffset);
		}
		else
		{
#ifdef RETURN_TRANSACTIONS
	#ifdef DATA_STORAGE
			byte *dataBuf = (byte *)calloc(sizeof(byte), SUBRANK_DATA_BYTES*busPacket->len);
			//FIXME: DEBUG DATA
			//DEBUG("[DPKT] Rank returning: "<<*(busPacket->data) << " \t r="<<busPacket->row<<" c="<<busPacket->column<<" transOffset="<< transOffset);
	#else
			byte *dataBuf = NULL;
	#endif
			busPacket->data->setData(dataBuf, SUBRANK_DATA_BYTES*busPacket->len, false);
#endif
		}


	}

} // end of namespace DRAMSim
