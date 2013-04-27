/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
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

#ifndef TRANSACTION_H
#define TRANSACTION_H

//Transaction.h
//
//Header file for transaction object

#include "SystemConfiguration.h"
#include "BusPacket.h"
#include "DataPacket.h"

#include <map>
#include <list>


namespace DRAMSim
{

	using namespace std;

	class Transaction
	{
	public:
		Transaction();
		typedef enum
		{
			DATA_READ,
			DATA_WRITE,
			RETURN_DATA
		} TransactionType;

		//fields
		TransactionType transactionType;
		uint64_t address;
		size_t len;
		DataPacket *data;
		uint64_t timeAdded;
		uint64_t timeReturned;
		uint64_t timeTraced;

		/////// clc add: module_id to indicates the memory module it would access  ///////////
		uint64_t transactionID;   ///< the ID of each transaction

		int memory_module_id;
		uint64_t timeSended;     //sended to the target memory module through bus
		uint64_t timeArrived;  ///< arrive at the targeted memory module, put into its command queue
	    uint64_t timeScheduled;            ///< col read cmd scheduled by the buffer scheduler, send commands to DRAM     uint64_t timeActiveScheduled;
	    uint64_t timeReturnToBufferScheduler;   ///< read data return to buffer scheduler, just for read requests
		/////// ~end clc added

		unsigned threadID;

		/// wangyn add:		///
		bool readDone;
		bool scheduling;
		uint64_t subCommandID;		///< the ID of the command that was put in the data buffer
		unsigned subDRAMCommandCount;	///< the amount of DRAM command that one item in the data buffer was devided into has returned
		/// ~end wangyn added	///

		//functions
		Transaction(TransactionType transType, uint64_t addr, DataPacket *data, size_t len=TRANS_DATA_BYTES/SUBRANK_DATA_BYTES, uint64_t time = 0);
		Transaction(const Transaction &t);

		void alignAddress();
		BusPacket::BusPacketType getBusPacketType();

		/// clc add:    ///
		void setTimeSended(uint64_t sendedTime){ timeSended = sendedTime; }
		void setTimeArrived(uint64_t ArrivedMMTime) { timeArrived = ArrivedMMTime;}
		void setTimeScheduled(uint64_t scheduledTime) { timeScheduled = scheduledTime; }
		void setTimeReturnToBufferScheduler(uint64_t returnBSTime){timeReturnToBufferScheduler = returnBSTime;}
		/// ~end clc added

		void print();
	};


#ifdef RETURN_TRANSACTIONS

	class TransactionReceiver
	{
		private:
			map<uint64_t, list<uint64_t> > pendingReadRequests;
			map<uint64_t, list<uint64_t> > pendingWriteRequests;
			unsigned counter;

		public:
			TransactionReceiver():counter(0){};

			void addPending(const Transaction *t, uint64_t cycle);
			void read_complete(unsigned id, uint64_t address, uint64_t done_cycle);
			void write_complete(unsigned id, uint64_t address, uint64_t done_cycle);

			bool pendingTrans();

	};
#endif

}

#endif

