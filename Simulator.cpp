/*
 * Simulator.cpp
 *
 *  Created on: Jul 16, 2012
 *      Author: Shawn GAO
 */

#include "Simulator.h"
#include "PrintMacros.h"
#include "Callback.h"
#include "ClockDomain.h"
#include "Transaction.h"


namespace DRAMSim
{

	using namespace std;

	ClockDomain* Simulator::clockDomainCPU = NULL;
	ClockDomain* Simulator::clockDomainDRAM = NULL;
	ClockDomain* Simulator::clockDomainTREE = NULL;


	Simulator::~Simulator()
	{
		if (trans != NULL)
		{
			delete trans;
		}

		delete simIO;
		delete clockDomainDRAM;
		delete clockDomainCPU;
		delete (memorySystem);
	}


	void Simulator::setup()
	{
		simIO->loadInputParams();
		simIO->initOutputFiles();

		memorySystem= new MemorySystem();

#ifdef RETURN_TRANSACTIONS
		transReceiver = new TransactionReceiver;
		/* create and register our callback functions */
		TransactionCompleteCB *read_cb = new CallbackP3<TransactionReceiver, void, unsigned, uint64_t, uint64_t>(transReceiver, &TransactionReceiver::read_complete);
		TransactionCompleteCB *write_cb = new CallbackP3<TransactionReceiver, void, unsigned, uint64_t, uint64_t>(transReceiver, &TransactionReceiver::write_complete);
		memorySystem->registerCallbacks(read_cb, write_cb, NULL);
#endif


		clockDomainCPU = new ClockDomain(new CallbackP0<Simulator,void>(this, &Simulator::update));
		clockDomainDRAM = new ClockDomain(new CallbackP0<MemorySystem,void>(memorySystem, &MemorySystem::update));
		clockDomainCPU->nextDomain = clockDomainDRAM;
		clockDomainDRAM->previousDomain = clockDomainCPU;
		clockDomainTREE = clockDomainCPU;


		// for compatibility with the old marss code which assumed an sg15 part with a
		// 2GHz CPU, the new code will reset this value later
		//setCPUClockSpeed(2000000000UL);
		// Initialize the ClockDomainCrosser to use the CPU speed
		// If cpuClkFreqHz == 0, then assume a 1:1 ratio (like for TraceBasedSim)
		// set the frequency ratio to 1:1
		initClockDomain(4);
		PRINT("DRAMSim2 Clock Frequency ="<<clockDomainDRAM->clock<<"Hz, CPU Clock Frequency="<<clockDomainCPU->clock<<"Hz");

	}


	void Simulator::start()
	{
#ifdef RETURN_TRANSACTIONS
		if (simIO->cycleNum == 0)
		{
			while (pendingTrace == true || transReceiver->pendingTrans() == true)
			{
				clockDomainTREE->tick();
			}
		}
		else
#endif
		if (simIO->cycleNum != 0)
		{
			while (clockDomainTREE->clockcycle < simIO->cycleNum)
			{
				clockDomainTREE->tick();
			}
		}
		else
		{
			ERROR("Please specify exact cycle numbers(!=0) to run!");
			exit(0);
		}
	}



	void Simulator::update()
	{
		if (trans == NULL)
		{
			trans = simIO->nextTrans();
			if (trans == NULL)
			{
				pendingTrace = false;
				return;
			}
		}

		if ( clockDomainCPU->clockcycle >= trans->timeTraced)
		{
			if(memorySystem->addTransaction(trans))
			{
#ifdef RETURN_TRANSACTIONS
				transReceiver->addPending(trans, clockDomainCPU->clockcycle);
#endif
				// the memory system accepted our request so now it takes ownership of it
				trans=NULL;
			}
		}
	}


	void Simulator::report()
	{
		memorySystem->printStats();
	}


	void Simulator::initClockDomain(double clockRatio)
	{
		clockDomainDRAM->time = tCK;
		clockDomainCPU->time = tCK/clockRatio;

		clockDomainDRAM->clock = (uint64_t)(1.0/(clockDomainDRAM->time*1e-9));
		clockDomainCPU->clock = (uint64_t)(1.0/(clockDomainCPU->time*1e-9));

		clockDomainTREE->setClockRatio(clockRatio);

	}


} // end of DRAMSim
