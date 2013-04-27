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

//Transaction.cpp
//
//Class file for transaction object
//	Transaction is considered requests sent from the CPU to
//	the memory controller (read, write, etc.)...

#include "Transaction.h"
#include "PrintMacros.h"
#include "BusPacket.h"


namespace DRAMSim
{
	using namespace std;

	Transaction::Transaction(TransactionType transType, uint64_t addr, DataPacket *dat, size_t len, uint64_t time) :
		transactionType(transType),	address(addr), data(dat), len(len), timeTraced(time)
	{
		alignAddress();
	}


	void Transaction::alignAddress()
	{
		// zero out the low order bits which correspond to the size of a transaction
		unsigned throwAwayBits = dramsim_log2(TRANS_DATA_BYTES);

		address >>= throwAwayBits;
		address <<= throwAwayBits;
	}

	Transaction::Transaction(const Transaction &t)
		: transactionType(t.transactionType),
		  address(t.address),
		  data(NULL),
		  timeAdded(t.timeAdded),
		  timeReturned(t.timeReturned),
		  timeTraced(t.timeTraced)
	{
#ifdef DATA_STORAGE
		ERROR("Data storage is really outdated and these copies happen in an \n improper way, which will eventually cause problems. Please send an \n email to dramninjas [at] gmail [dot] com if you need data storage");
		abort();
#endif
	}

	void Transaction::print()
	{
		if (transactionType == DATA_READ)
		{
			PRINT("T [Read] [0x" << hex << address << "]" << dec );
		}
		else if (transactionType == DATA_WRITE)
		{
			PRINT("T [Write] [0x" << hex << address << "] [" << dec << data << "]" );
		}
		else if (transactionType == RETURN_DATA)
		{
			PRINT("T [Data] [0x" << hex << address << "] [" << dec << data << "]" );
		}
		cout<<" ["<<threadID<<"] ";
		//cout<<":MModID="<<memory_module_id<<",addMC="<<timeAdded<<",sendMMod="<<timeSended<<",arrivedMMod="<<timeArrived<<",scheduled="<<timeScheduled<<",returnBS="<<timeReturnToBufferScheduler<<",returnMC="<<timeReturned;
		cout<<endl;
	}

	BusPacket::BusPacketType Transaction::getBusPacketType()
	{
		switch (transactionType)
		{
			case DATA_READ:
			if (rowBufferPolicy == ClosePage)
			{
				return BusPacket::READ_P;
			}
			else if (rowBufferPolicy == OpenPage)
			{
				return BusPacket::READ;
			}
			else
			{
				ERROR("Unknown row buffer policy");
				abort();
			}
			break;
		case DATA_WRITE:
			if (rowBufferPolicy == ClosePage)
			{
	#ifdef ICDP_LONG_WRITE
				return BusPacket::ICDP_WRITE_P;
	#else
				return BusPacket::WRITE_P;
	#endif
			}
			else if (rowBufferPolicy == OpenPage)
			{
	#ifdef ICDP_LONG_WRITE
			return BusPacket::ICDP_WRITE;
	#else
			return BusPacket::WRITE;
	#endif
			}
			else
			{
				ERROR("Unknown row buffer policy");
				abort();
			}
			break;
		default:
			ERROR("This transaction type doesn't have a corresponding bus packet type");
			//abort();
			break;
		}
		exit(1);
	}


#ifdef RETURN_TRANSACTIONS

	void TransactionReceiver::addPending(const Transaction *t, uint64_t cycle)
	{
		// C++ lists are ordered, so the list will always push to the back and
		// remove at the front to ensure ordering
		if (t->transactionType == Transaction::DATA_READ)
		{
			pendingReadRequests[t->address].push_back(cycle);
		}
		else if (t->transactionType == Transaction::DATA_WRITE)
		{
			pendingWriteRequests[t->address].push_back(cycle);
		}
		else
		{
			ERROR("This should never happen");
			exit(-1);
		}
		counter++;
	}

	void TransactionReceiver::read_complete(unsigned id, uint64_t address, uint64_t done_cycle)
	{
		map<uint64_t, list<uint64_t> >::iterator it;
		it = pendingReadRequests.find(address);
		if (it == pendingReadRequests.end())
		{
			ERROR("Cant find a pending read for this one");
			exit(-1);
		}
		else
		{
			if (it->second.size() == 0)
			{
				ERROR("Nothing here, either");
				exit(-1);
			}
		}
		if (DEBUG_TRANS_LATENCY == true)
		{
			uint64_t added_cycle = pendingReadRequests[address].front();
			uint64_t latency = (done_cycle - added_cycle);

			cout << "Read Callback:  0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
		}
		pendingReadRequests[address].pop_front();
		counter--;
	}

	void TransactionReceiver::write_complete(unsigned id, uint64_t address, uint64_t done_cycle)
	{
		map<uint64_t, list<uint64_t> >::iterator it;
		it = pendingWriteRequests.find(address);
		if (it == pendingWriteRequests.end())
		{
			ERROR("Cant find a pending read for this one");
			exit(-1);
		}
		else
		{
			if (it->second.size() == 0)
			{
				ERROR("Nothing here, either");
				exit(-1);
			}
		}
		if (DEBUG_TRANS_LATENCY == true)
		{
			uint64_t added_cycle = pendingWriteRequests[address].front();
			uint64_t latency = done_cycle - added_cycle;

			cout << "Write Callback: 0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
		}
		pendingWriteRequests[address].pop_front();
		counter--;
	}

	bool TransactionReceiver::pendingTrans()
	{
		return (counter==0)?false:true;
	}

#endif
}



