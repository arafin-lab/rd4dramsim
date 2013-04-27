#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "SimulatorIO.h"
#include "ClockDomain.h"
#include "MemorySystem.h"
#include "ReorderBuffer.h"


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

		bool addTransaction(Transaction *trans);

		static ClockDomain* clockDomainCPU;
		static ClockDomain* clockDomainDRAM;
		static ClockDomain* clockDomainTREE;

		static ReorderBuffer *rob[100];
	#ifdef RETURN_TRANSACTIONS
		TransactionReceiver *transReceiver;
	#endif

	private:
		void initClockDomain(double clockRatio);


		SimulatorIO *simIO;
		MemorySystem *memorySystem;
		Transaction *trans;


		bool pendingTrace;



	};
}

#endif /* SIMULATOR_H_ */
