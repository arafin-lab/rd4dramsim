#ifndef CLOCKDOMAIN_H
#define CLOCKDOMAIN_H

#include <iostream>
#include <cmath>
#include <stdint.h>
#include "Callback.h"


namespace DRAMSim
{
	class ClockDomain
	{
	public:
		ClockUpdateCB *callback;
		ClockDomain *previousDomain;
		ClockDomain *nextDomain;
		unsigned numerator;
		unsigned denominator;
		unsigned upCounter;
		unsigned downCounter;
		uint64_t clockcycle;
		uint64_t clock;
		double time;


		ClockDomain(ClockUpdateCB *callback, uint64_t clock = 0);

		void setClockRatio(double ratio);

		void tick();
	};
}

#endif
