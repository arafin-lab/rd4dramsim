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
		setCPUClock(0);
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
		{
			while (clockDomainTREE->clockcycle < simIO->cycleNum)
			{
				clockDomainTREE->tick();
			}
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


	void Simulator::setCPUClock(uint64_t cpuClkFreqHz)
	{
		uint64_t dramsimClkFreqHz = (uint64_t)(1.0/(tCK*1e-9));
		clockDomainDRAM->clock = dramsimClkFreqHz;
		clockDomainCPU->clock = (cpuClkFreqHz == 0) ? dramsimClkFreqHz : cpuClkFreqHz;
	}

	void Simulator::setClockRatio(double ratio)
	{
		// Compute numerator and denominator for ratio, then pass that to other constructor.
		double x = ratio;

		const int MAX_ITER = 15;
		size_t i;
		unsigned ns[MAX_ITER], ds[MAX_ITER];
		double zs[MAX_ITER];
		ds[0] = 0;
		ds[1] = 1;
		zs[1] = x;
		ns[1] = (int)x;

		for (i = 1; i<MAX_ITER-1; i++)
		{
			if (fabs(x - (double)ns[i]/(double)ds[i]) < 0.00005)
			{
				//printf("ANSWER= %u/%d\n",ns[i],ds[i]);
				break;
			}
			//TODO: or, if the answers are the same as the last iteration, stop

			zs[i+1] = 1.0f/(zs[i]-(int)floor(zs[i])); // 1/(fractional part of z_i)
			ds[i+1] = ds[i]*(int)floor(zs[i+1])+ds[i-1];
			double tmp = x*ds[i+1];
			double tmp2 = tmp - (int)tmp;
			ns[i+1] = tmp2 >= 0.5 ? ceil(tmp) : floor(tmp); // ghetto implementation of a rounding function
			//printf("i=%lu, z=%20f n=%5u d=%5u\n",i,zs[i],ns[i],ds[i]);
		}

		//printf("APPROXIMATION= %u/%d\n",ns[i],ds[i]);
		clockDomainDRAM->clock = ns[i];
		clockDomainCPU->clock = ds[i];

		//cout << "CTOR: callback address: " << (uint64_t)(this->callback) << "\t ratio="<<clock1<<"/"<<clock2<< endl;
	}


} // end of DRAMSim