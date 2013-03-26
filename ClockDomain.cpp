#include "ClockDomain.h"
#include "Callback.h"

using namespace std;



namespace DRAMSim
{

	ClockDomain::ClockDomain(ClockUpdateCB *callback, uint64_t clock) :
			callback(callback),
			clock(clock),
			clockcycle(0),
			numerator(0),
			denominator(0),
			upCounter(0),
			downCounter(0),
			time(0),
			previousDomain(NULL),
			nextDomain(NULL){};


	void ClockDomain::tick()
	{
		// update current clock domain
		if (nextDomain == NULL)						// no more next Clock Domain
		{
			(*callback)();
			clockcycle++;
			return;
		}
		else if (clock == nextDomain->clock)		//short circuit case for 1:1 ratios
		{
			(*callback)();
			clockcycle++;
			nextDomain->tick();
		}
		else
		{
			// update counter
			if (numerator > denominator)
			{
				upCounter += numerator;
				while (upCounter > downCounter)
				{
					downCounter += denominator;
					(*callback)();
					clockcycle++;
				}
				nextDomain->tick();
			}
			else if (numerator < denominator)
			{
				downCounter += denominator;
				(*callback)();
				clockcycle++;
				while(downCounter > upCounter)
				{
					upCounter += numerator;
					nextDomain->tick();
				}
			}
		}

		// reset counters when numeratorCounter and denominatorCounter are equal
		if (upCounter == downCounter)
		{
			upCounter = 0;
			downCounter =0;
		}
	}

	void ClockDomain::setClockRatio(double ratio)
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
		numerator = ns[i];
		denominator = ds[i];

		//cout << "CTOR: callback address: " << (uint64_t)(this->callback) << "\t ratio="<<clock1<<"/"<<clock2<< endl;
	}

} // end of namespace DRAMSim
