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


//BusPacket.cpp
//
//Class file for bus packet object
//
#include "SimulatorIO.h"
#include "BusPacket.h"
#include <cmath>


namespace DRAMSim
{
	using namespace std;

	BusPacket::~BusPacket()
	{
		if (ssrData != NULL)
		{
			delete ssrData;
		}

		/*
		if (data != NULL)
		{
			delete data;
		}
		*/
	}

	BusPacket::BusPacket(BusPacketType packtype, unsigned rk, unsigned bk, unsigned rw, unsigned col, uint64_t physicalAddr, DataPacket *dat, size_t len) :
		busPacketType(packtype),
		physicalAddress(physicalAddr),
		rank(rk),
		bank(bk),
		row(rw),
		column(col),
		data(dat),
		len(len),
		ssrData(NULL)
	{

#ifdef DATA_RELIABILITY_ECC
		this->len = TRANS_TOTAL_BYTES/SUBRANK_DATA_BYTES;
#endif

#ifdef DATA_RELIABILITY_CHIPKILL
		this->len = TRANS_TOTAL_BYTES/SUBRANK_DATA_BYTES;
#endif

	}

	void BusPacket::print(uint64_t currentClockCycle, bool dataStart)
	{
		if (this == NULL)
		{
			return;
		}

		if (VERIFICATION_OUTPUT)
		{
			switch (busPacketType)
			{
			case READ:
				SimulatorIO::verifyFile << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",0);"<<endl;
				break;
			case READ_P:
				SimulatorIO::verifyFile << currentClockCycle << ": read ("<<rank<<","<<bank<<","<<column<<",1);"<<endl;
				break;
			case WRITE:
				SimulatorIO::verifyFile << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",0 , 0, 'h0);"<<endl;
				break;
			case WRITE_P:
				SimulatorIO::verifyFile << currentClockCycle << ": write ("<<rank<<","<<bank<<","<<column<<",1, 0, 'h0);"<<endl;
				break;
			case ACTIVATE:
				SimulatorIO::verifyFile << currentClockCycle <<": activate (" << rank << "," << bank << "," << row <<");"<<endl;
				break;
			case PRECHARGE:
				SimulatorIO::verifyFile << currentClockCycle <<": precharge (" << rank << "," << bank << "," << row <<");"<<endl;
				break;
			case REFRESH:
				SimulatorIO::verifyFile << currentClockCycle <<": refresh (" << rank << ");"<<endl;
				break;
			case DATA:
				//TODO: data verification?
				break;
			default:
				ERROR("Trying to print unknown kind of bus packet");
				exit(-1);
			}
		}
	}
	void BusPacket::print()
	{
		if (this == NULL) //pointer use makes this a necessary precaution
		{
			return;
		}
		else
		{
			switch (busPacketType)
			{
			case READ:
				PRINT("BP [READ] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case READ_P:
				PRINT("BP [READ_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
#ifdef DATA_RELIABILITY_ICDP
	#ifdef ICDP_PRE_READ
			case PRE_READ:
				PRINT("BP [PRE_READ] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
	#endif
	#ifdef ICDP_LONG_WRITE
			case ICDP_WRITE:
				PRINT("BP [ICDP_WRITE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case ICDP_WRITE_P:
				PRINT("BP [ICDP_WRITE_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
	#endif
#endif
			case WRITE:
				PRINT("BP [WRITE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case WRITE_P:
				PRINT("BP [WRITE_P] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case ACTIVATE:
				PRINT("BP [ACT] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case PRECHARGE:
				PRINT("BP [PRE] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case REFRESH:
				PRINT("BP [REF] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"]");
				break;
			case DATA:
				PRINTN("BP [DATA] pa[0x"<<hex<<physicalAddress<<dec<<"] r["<<rank<<"] b["<<bank<<"] row["<<row<<"] col["<<column<<"] data["<<data<<"]=");
				if (data == (void*)(0x680a80) )
				{
					int helloworld=0;
					helloworld++;
				}
				printData();
				PRINT("");
				break;
			default:
				ERROR("Trying to print unknown kind of bus packet");
				exit(-1);
			}
		}
	}

	void BusPacket::printData() const
	{
		if (data == NULL)
		{
			PRINTN("NO DATA");
			return;
		}
		PRINTN("'" << hex);
		for (int i=0; i < 4; i++)
		{
			PRINTN(((uint64_t *)data)[i]);
		}
		PRINTN("'" << dec);
	}


#ifdef DATA_RELIABILITY


	void BusPacket::DATA_ENCODE()
	{

#ifdef DATA_RELIABILITY_ECC
		ECC_HAMMING_SECDED(ENCODE);
#endif

#ifdef DATA_RELIABILITY_CHIPKILL
		CHIPKILL(ENCODE);
#endif


#ifdef DATA_RELIABILITY_ICDP
		ICDP(ENCODE);
#endif
	}

	void BusPacket::DATA_DECODE()
	{

#ifdef DATA_RELIABILITY_CHIPKILL
		//CHIPKILL(DECODE);
#endif

#ifdef DATA_RELIABILITY_ECC
		//ECC_HAMMING_SECDED(DECODE);
#endif


#ifdef DATA_RELIABILITY_ICDP
		//ICDP(DECODE);
#endif
	}

	int BusPacket::DATA_CHECK(int error=0)
	{
		int result;

#ifdef DATA_RELIABILITY_CHIPKILL
		//CHIPKILL(DECODE);
#endif

#ifdef DATA_RELIABILITY_ECC
		//result = ECC_HAMMING_SECDED(CHECK);
#endif

#ifdef DATA_RELIABILITY_CHIPKILL
		//CHIPKILL(ENCODE);
#endif


#ifdef DATA_RELIABILITY_ICDP
		//result = ICDP(CHECK);
#endif
		result = error;

		return result;
	}

	int BusPacket::DATA_CORRECTION(int error=0)
	{
		int result;

#ifdef DATA_RELIABILITY_CHIPKILL
		//CHIPKILL(DECODE);
#endif

#ifdef DATA_RELIABILITY_ECC
		//result = ECC_HAMMING_SECDED(CORRECTION);
#endif

#ifdef DATA_RELIABILITY_CHIPKILL
		//CHIPKILL(ENCODE);
#endif


#ifdef DATA_RELIABILITY_ICDP
		//result = ICDP(CORRECTION);
#endif
		if (error < FT_BITS + 1)
		{
			result=0;
		}
		else
		{
			result=error;
		}
		return result;
	}


#ifdef DATA_RELIABILITY_ICDP

	int BusPacket::ICDP(RELIABLE_OP op)
	{
		/* Legency Codes
		int parityPosition = column/(DEVICE_WIDTH*(NUM_DEVICES-2*NUM_RANKS));

		switch (op)
		{
		case ENCODE:
		{
			byte *encodeData = new byte[TRANS_TOTAL_BYTES];
			memcpy(encodeData, ssrData->data->getData(), TRANS_TOTAL_BYTES);

			//update data
			for (int iEncode=0,iUpdate=0; iUpdate < len; iEncode++)
			{
				if (iEncode == parityPosition) continue;
				memcpy(encodeData+iEncode*SUBRANK_DATA_BYTES, data->getData()+iUpdate*SUBRANK_DATA_BYTES, SUBRANK_DATA_BYTES);
				iUpdate++;
			}

			//calculate parity
			for (int i=0; i < LEN_DEF; i++)
			{
				if (i == parityPosition) continue;
				for (int j=0; j < SUBRANK_DATA_BYTES;j++)
				{
					encodeData[parityPosition*SUBRANK_DATA_BYTES + j] ^= encodeData[i*SUBRANK_DATA_BYTES + j];
				}
			}

			//calculate checksum
			for (int i=0; i < LEN_DEF;i++)
			{
				for (int j=0; j < SUBRANK_DATA_BYTES; j++)
				{
					if (i == parityPosition) continue;
					encodeData[(LEN_DEF-1)*SUBRANK_DATA_BYTES + i] ^= encodeData[i*SUBRANK_DATA_BYTES + j];
				}
			}

			data->setData(encodeData,TRANS_TOTAL_BYTES,false);

			break;
		}
		case DECODE:
		{
			byte *decodeData = new byte[TRANS_DATA_BYTES];

			for (int i=0,j=0; i < len; j++)
			{
				if (j == parityPosition) continue;
				memcpy(decodeData+i*SUBRANK_DATA_BYTES, data->getData()+j*SUBRANK_DATA_BYTES, SUBRANK_DATA_BYTES);
				i++;
			}

			break;
		}
		case CHECK:
		{
			break;
		}
		case CORRECTION:
		{
			break;
		}
		default:
			DEBUG("Invalid ECC Operation Type");
			return false;
			break;
		}
		DEBUG("Unexpected Executing Location");
		return false;
		*/

		switch (op)
		{
		case ENCODE:
		{

			break;
		}
		case DECODE:
		{

			break;
		}
		case CHECK:
		{
			return 0;
			break;
		}
		case CORRECTION:
		{
			return 0;
			break;
		}
		default:
			DEBUG("Invalid ECC Operation Type");
			return false;
			break;
		}

		return 0;
	}

#endif



#ifdef DATA_RELIABILITY_ECC
	int BusPacket::ECC_HAMMING_SECDED(RELIABLE_OP eccop, int n, int m)
	{
		if (data->getData() == NULL) return -1;

		int c = n - m;
		int l = TRANS_TOTAL_BYTES*8 / n;

		bitset<TRANS_DATA_BYTES*8> transDataBits;
		bitset<TRANS_TOTAL_BYTES*8> transBits1;
		bitset<TRANS_TOTAL_BYTES*8> transBits2;
		bitset<ECC_WORD_BITS> busEccBits;

		switch (eccop)
		{
		case DECODE:
		case CHECK:
		case CORRECTION:
		{

			BitsfromByteArray(transBits1,TRANS_TOTAL_BYTES);

			for (int iLoop = 0; iLoop < l; iLoop++)
			{
				for (int iData = 0; iData < m; iData++)
				{
					transDataBits[iLoop*m + iData] = transBits1[iLoop*n + iData];
				}
			}
			if (eccop == DECODE)
			{
				data->setData(BitstoByteArray(transDataBits), TRANS_DATA_BYTES, false);
				return 1;
				break;
			}
		}
		//end of DECODE
		//begin of ENCODE
		case ENCODE:
		{
			if (eccop == ENCODE)
			{
				BitsfromByteArray(transDataBits,TRANS_DATA_BYTES);
			}

			for (int iLoop=0; iLoop < l; iLoop++)
			{
				busEccBits.reset();
				for (int iCheck = 0; iCheck < c-1; iCheck++)
				{
					int checkBitCounter = (iCheck == 0) ? (2) : (iCheck + 1);
					int positionIncrement = pow(2.0, iCheck);
					int iCounter = 1;
					int iData = POSITION_REVISE(iCheck) + iCounter;

					bool first = true;

					while (iData < m - 1)
					{
						if (first == true)
						{
							busEccBits[iCheck] = transDataBits[iLoop*m + iData];
							first = false;
						}
						else
						{
							busEccBits[iCheck] = busEccBits[iCheck]^transDataBits[iLoop*m + iData];
						}

						iData++;
						iCounter++;
						if (iCounter % positionIncrement == 0)
						{
							iData += positionIncrement;
							iCounter = 0;
						}
						while ( iData > POSITION_REVISE(checkBitCounter) )
						{
							checkBitCounter++;
							iData--;
						}
					}
				} // end of iCheck

				for (int iData = 0; iData < n-1; iData++)
				{
					int iCheck = iData - m;
					if (iCheck < 0)
					{
						transBits2[iLoop*n + iData] = transDataBits[iLoop*m + iData];
					}
					else
					{
						transBits2[iLoop*n + iData] = busEccBits[iCheck];
					}

					if (iData == 0)
					{
						transBits2[iLoop*n + (n-1)] = transBits2[iLoop*n];
					}
					else
					{
						transBits2[iLoop*n + (n-1)] = transBits2[iLoop*n + (n-1)] ^ transBits2[iLoop*n + iData];
					}
				}

			} // end of iLoop

			if (eccop == ENCODE)
			{
				data->setData(BitstoByteArray(transBits2), TRANS_TOTAL_BYTES, false);

				return 1;
				break;
			}
			// end of ENCODE
			//begin of CHECK&CORRECTION
			int wordErrorNum = 0;
			int dataErrorNum = 0;
			int killErrorNum = 0;

			bitset<ECC_WORD_BITS> busErrorBits;

			for (int iLoop=0; iLoop < l; iLoop++)
			{
				int iErrorPosition = 0;
				busErrorBits.reset();

				for (int iCheck = 0; iCheck < c; iCheck++)
				{
					busErrorBits[iCheck] = transBits1[iLoop*n + m + iCheck]^transBits2[iLoop*n + m + iCheck];
					iErrorPosition = (busErrorBits[iCheck] == 1) ? (iErrorPosition + pow(2.0,iCheck)) : iErrorPosition;
				}

				if (busErrorBits[c-1] == 0 && iErrorPosition == 0)
				{
					wordErrorNum = 0;
				}
				else if (busErrorBits[c-1] == 1 && iErrorPosition != 0)
				{
					wordErrorNum = 1;
				}
				else if (busErrorBits[c-1] == 0 && iErrorPosition != 0)
				{
					wordErrorNum = 2;
				}
				else if (busErrorBits[c-1] == 1 && iErrorPosition == 0)
				{
					wordErrorNum = 3;
				}
				else
				{
					wordErrorNum = 3;
				}

				if (eccop == CORRECTION && wordErrorNum == 1)
				{
					if (isPowerOfTwo(iErrorPosition))
					{
						iErrorPosition = m + dramsim_log2(iErrorPosition);
					}
					else
					{
						int checkBitCounter = 0;
						while (--iErrorPosition > POSITION_REVISE(checkBitCounter) )
						{
							checkBitCounter++;
						}
					}
					transBits1[iLoop*n + iErrorPosition].flip();
					killErrorNum++;
				}

				dataErrorNum += wordErrorNum;
				wordErrorNum = 0;
			}

			if (eccop == CHECK)
				return dataErrorNum;
			else
				if (eccop == CORRECTION)
					data->setData(BitstoByteArray(transBits1), TRANS_TOTAL_BYTES, false);
				return (dataErrorNum-killErrorNum);

			break;
		}
		default:
			DEBUG("Invalid ECC Operation Type");
			return -1;
			break;
		}
		DEBUG("Unexpected Executing Location");
		return false;
	}



#ifdef DATA_RELIABILITY_CHIPKILL
	void BusPacket::CHIPKILL(RELIABLE_OP op)
	{
		if (data->getData() == NULL) return;

		int loop = BL;

		switch (op)
		{
		case ENCODE:
		{
			bitset<TRANS_TOTAL_BYTES*8> chipkillDataBits;
			bitset<TRANS_TOTAL_BYTES*8> eccDataBits;
			BitsfromByteArray(eccDataBits,TRANS_TOTAL_BYTES);

			for (int iLoop=0; iLoop < loop; iLoop++)
			{
				for (int iWord = 0; iWord < (int)(DEVICE_WIDTH); iWord++)
				{
					for (int iData = 0; iData < TOTAL_WORD_BITS; iData++)
					{
						chipkillDataBits[iData*DEVICE_WIDTH + iWord] = eccDataBits[iWord*TOTAL_WORD_BITS + iData];
					}
				}
			}
			data->setData( BitstoByteArray(chipkillDataBits), TRANS_TOTAL_BYTES, false);
			break;
		}
		case DECODE:
		{
			bitset<TRANS_TOTAL_BYTES*8> chipkillDataBits;
			bitset<TRANS_TOTAL_BYTES*8> eccDataBits;
			BitsfromByteArray(chipkillDataBits,TRANS_TOTAL_BYTES);

			for (int iLoop=0; iLoop < loop; iLoop++)
			{
				for (int iWord = 0; iWord < (int)(DEVICE_WIDTH); iWord++)
				{
					for (int iData = 0; iData < TOTAL_WORD_BITS; iData++)
					{
						eccDataBits[iWord*TOTAL_WORD_BITS + iData] = chipkillDataBits[iData*DEVICE_WIDTH + iWord];
					}
				}
			}
			data->setData(BitstoByteArray(eccDataBits), TRANS_TOTAL_BYTES, false);
			break;
		}
		case CHECK:
		case CORRECTION:
			break;
		} // end of switch

	}
#endif //DATA_RELIABILITY_CHIPKILL

#endif //DATA_RELIABILITY_ECC

	byte *BusPacket::BitstoByteArray(bitset<TRANS_DATA_BYTES*8> &bits)
	{
		int len=bits.size()/8;
		byte *bytes = (byte *)calloc(len, sizeof(byte));
		for (int j=0;j<len;++j)
		{
			bytes[j]=0;
		}
		for (size_t i=0; i<bits.size(); i++)
		{
			if (bits.test(i))
			{
				bytes[i/8] |= 1<<(7-i%8);
			}
		}
		return bytes;
	}


	byte *BusPacket::BitstoByteArray(bitset<TRANS_TOTAL_BYTES*8> &bits)
	{
		int len=bits.size()/8;
		byte *bytes = (byte *)calloc(len, sizeof(byte));
		for (int j=0;j<len;++j)
		{
			bytes[j]=0;
		}
		for (size_t i=0; i<bits.size(); i++)
		{
			if (bits.test(i))
			{
				bytes[i/8] |= 1<<(7-i%8);
			}
		}
		return bytes;
	}


#endif //DATA_RELIABILITY

} // end of DRAMSim
