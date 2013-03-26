#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "SimulatorIO.h"
#include "ClockDomain.h"
#include "MemorySystem.h"


namespace DRAMSim
{
	class Simulator
	{
	public:
		Simulator(SimulatorIO *simIO) : simIO(simIO),trans(NULL),pendingTrace(true) {};
		~Simulator();

		void setup();
		void start();
		void update();
		void report();

		static ClockDomain* clockDomainCPU;
		static ClockDomain* clockDomainDRAM;
		static ClockDomain* clockDomainTREE;

	private:
		void initClockDomain(double clockRatio);


		SimulatorIO *simIO;
		MemorySystem *memorySystem;
		Transaction *trans;

		bool pendingTrace;

#ifdef RETURN_TRANSACTIONS
		TransactionReceiver *transReceiver;
#endif

	};
}

#endif /* SIMULATOR_H_ */
