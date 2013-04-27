#include "ReorderBuffer.h"
#include "Simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#define NON_MEM_BITS 9
#define MEM_ADDR_BITS 48
#define RW_BITS 1
#define SUB_BLOCK_BITS 4
#define CACHE_ACCESS_TYPE_BITS 2
 
#define NONMEM_MASK ((1UL<<NON_MEM_BITS) - 1)
#define MEM_ADDR_MASK ((1UL<<MEM_ADDR_BITS) - 1)
#define RW_MASK ((1UL<<RW_BITS) - 1)
#define SUB_BLOCK_MASK ((1UL<<SUB_BLOCK_BITS) - 1)
#define CACHE_ACCESS_TYPE_MASK ((1UL<<CACHE_ACCESS_TYPE_BITS) - 1)

#define CAT_ENTRY_BITS 0
#define SB_ENTRY_BITS 2       //(CAT_ENTRY_BITS + CACHE_ACCESS_TYPE_BITS)
#define RW_ENTRY_BITS 6       //(SB_ENTRY_BITS + SUB_BLOCK_BITS)
#define ADDR_ENTRY_BITS 7     //(RW_ENTRY_BITS + RW_BITS)
#define NONMEM_ENTRY_BITS 55    //(ADDR_ENTRY_BITS + MEM_ADDR_BITS)

#define BTRACE_BUF_SIZE (4UL<<20)

//#define TRACE_PROCESS
#define StatisticsROB 
//#define NO_TRACE_PROCESS

#ifdef TRACE_PROCESS
#define NUM_TRACES_PROCESS 1024   //the size of window we can preprocess
typedef struct _TraceStruct
{
    unsigned int nonmem;
    unsigned int rw;
    unsigned int sub_block;
    unsigned int cacheAT;
    unsigned long addr;
    bool isMerged;
} TraceStruct;

TraceStruct traceStruct[NUM_TRACES_PROCESS];
#endif



using namespace DRAMSim;
using namespace std;

ReorderBuffer::ReorderBuffer(int id, string file_name, Simulator *sim):core_id(id),head(0),tail(0),inflight(0),tracedone(0)
{
	assert(ROB_SIZE >= 32);
	core_id = id;
	simulator = sim;
    fname_btrace = file_name;
    cout<<"ReorderBuffer"<<core_id<<" fname_btrace: "<<fname_btrace<<endl;
    f_btrace = NULL;
    btrace_buf = NULL;
#ifndef TRACE_PROCESS
    btrace_buf_index = 0;
#else
    btrace_buf_index = BTRACE_BUF_SIZE;
    traceStructIndex = NUM_TRACES_PROCESS;
#endif
    btrace_buf_capacity = 0;
    reach_eof = 0;
    MSHRcounter = 0;
    btrace_buf = (unsigned long*)malloc(sizeof(unsigned long) * BTRACE_BUF_SIZE);
    init_btrace();

    
    waiting_nonmem = 0;
    pending_tran = 0;
    TotalCommitinROB = 0;
    TotalFetchinROB = 0;
    TotalAddinROB = 0;

    waiting_transaction = NULL;

    for(int i = 0; i < 16; i++)
    {
        datalength[i] = 0;
    }

	comptime = (uint64_t*)malloc(sizeof(uint64_t) * ROB_SIZE);
	mem_address = (uint64_t *)malloc(sizeof(uint64_t) * ROB_SIZE);
	instrpc = (uint64_t *)malloc(sizeof(uint64_t) * ROB_SIZE);
	optype = (int *)malloc(sizeof(int) * ROB_SIZE);

	assert(comptime && mem_address && instrpc && optype); ///the space has been malloced successfully
}

ReorderBuffer::~ReorderBuffer()
{
#ifdef StatisticsROB
    cout<<"In ROB: ";
    for(int i = 0; i <= 8; i++)
    {
        cout<<datalength[i]<<", ";
    }
    cout<<endl;
#endif
	free(comptime);
	free(mem_address);
	free(instrpc);
	free(optype);
}

void ReorderBuffer::init_btrace()
{
    if(!btrace_buf)
    {   
        fprintf(stderr, "## Failed to malloc space for binary trace buf\n");
        exit(-8);
    }
     
    assert(fname_btrace != "");
    f_btrace = fopen(fname_btrace.c_str(),"rb");
    if(!f_btrace)
    {   
        fprintf(stderr, "## Failed to open file %s\n", fname_btrace.c_str());
        exit(-8);
    }
    for(int i = 0; i < 16; i++)
        fread(btrace_buf, sizeof(unsigned long), BTRACE_BUF_SIZE, f_btrace);
    reach_eof = 0;
    //while(!reach_eof)
    //    read_next_trace();
}
                     
void ReorderBuffer::close_btrace()
{   
    fclose(f_btrace);
    free(btrace_buf);
}

void ReorderBuffer::update()
{
	//First, commit the head instrs in the ROB if they could in this cycle
	//Note, the max number of commit instr is MAX_RETIRE
	unsigned num_ret = 0, num_fet = 0;
    while((num_ret < MAX_RETIRE) && !this->isEmpty())
    {
        //keep retiring until retire width is comsumed or ROB is empty
        //whether the head instr could be committed
        if(isCommitted())
        {
            //ok, the head could be committed
            //cout<<"succeed commit!"<<endl;
            head = (head + 1) % ROB_SIZE;
            inflight--;
          	num_ret++;
            TotalCommitinROB++;
        }
        else
        {
           	//the head instr could not be committed, so stop committing
            break;
        }
    }
	//printf("ROB commit OK!\n");
	while((num_fet < MAX_FETCH) && !this->isFull() && MSHRcounter <= 16 && waiting_transaction == NULL)
	{
        if(!reach_eof && waiting_nonmem <= 0)
        {
            if(pending_tran == 0)
            {
                int result = read_next_trace(&nonmem, &addr, &rw, &sub_block, &cacheAT);
                if(result == 2 )
                {
                    result = read_next_trace(&nonmem, &addr, &rw, &sub_block, &cacheAT);
                }
                //if(sub_block > 8)cout<<"coarn memory access:"<<sub_block<<" "<<addr<<endl;
                //cout<<"nonmem: "<<nonmem<<" addr: "<<addr<<"    rw: "<<rw<<"    sub_block: "<<sub_block<<"  cacheAT:"<<cacheAT<<endl;
                //assert(result == 1);
                waiting_nonmem = nonmem;
            }
            if(waiting_nonmem <= 0)
            {
                if(rw == 0)
                {
                    issueInstr(MEM_WRITE, 0);
                    //TotalAddinROB++;
                }
                else
                {
                    assert(rw == 1);
                    switch(cacheAT)
                    {
                        case 0:
                            {
                                issueInstr(L1_CACHE_HIT, 0);
                                assert(sub_block == 0);
                                //TotalAddinROB++;
                                break;
                            }
                        case 1:
                            {
                                issueInstr(L2_CACHE_HIT, 0);
                                assert(sub_block == 0);
                                //TotalAddinROB++;
                                break;
                            }
                        case 2:
                            {
                                issueInstr(L3_CACHE_HIT, 0);
                                assert(sub_block == 0);
                                //TotalAddinROB++;
                                break;
                            }
                        case 3:
                            {
                                issueInstr(ALL_CACHE_MISS, addr);
                                //TotalAddinROB++;
                                break;
                            }
                        default:
                            {
                                cout<<"error cacheAT!"<<endl;
                            }
                    }
                }
                num_fet++;
                if(cacheAT == 3)
                {
                    if(rw == 0)
                    {
                        generated_transaction = generateTransaction(addr, sub_block, Transaction::DATA_WRITE);
                    }
                    else
                    {
                        generated_transaction = generateTransaction(addr, sub_block, Transaction::DATA_READ);
                    }
                    //cout<<"go here!"<<endl;
                    if(simulator->addTransaction(generated_transaction) == false)
                    {
                        //cout<<"addTransaction exit!"<<endl;
                        waiting_transaction = generated_transaction;
                        break;
                        //exit(0);
                    }
                    else
                    {
                        TotalAddinROB++;
                    }

                }
                pending_tran = 0;
            }
            else
            {
                issueInstr(NON_MEM, 0);
                //TotalAddinROB++;
                num_fet++;
                waiting_nonmem--;
                pending_tran = 1;
            }
        }
        else
        {
            issueInstr(NON_MEM, 0);
            //TotalAddinROB++;
            num_fet++;
            waiting_nonmem--;
        }
	}
    if(waiting_transaction != NULL)
    {
        if(simulator->addTransaction(generated_transaction) == true)
        {
            waiting_transaction = NULL;
            TotalAddinROB++;
        }
    }
}


#ifndef TRACE_PROCESS
int ReorderBuffer::read_next_trace(unsigned int *nonmem, unsigned long *addr, unsigned int *rw, unsigned int *sub_block, unsigned int *cacheAT)
{
    static unsigned int debug_count = 0;
    unsigned long btrace;
    unsigned long tmp;

    if(btrace_buf_index == btrace_buf_capacity)
    {
        if(!reach_eof)
        {
            btrace_buf_capacity = (unsigned long)fread(btrace_buf, sizeof(unsigned long), BTRACE_BUF_SIZE, f_btrace);
            btrace_buf_index = 0;

            if(btrace_buf_capacity == 0)
            {
                cout<<"Trace file is over!"<<endl;
                fclose(f_btrace);
                init_btrace();
                read_next_trace(nonmem, addr, rw, sub_block, cacheAT);
                //return 0;   //end of the file
            }
            if(btrace_buf_capacity != BTRACE_BUF_SIZE)
            {
                reach_eof = 1;
                cout<<"Trace file is over!"<<endl;
            }
            printf("get %lu MB btrace from file\n", btrace_buf_capacity >> 20);
        }
        else 
        {
            cout<<"Trace file is over!"<<endl;
            fclose(f_btrace);
            init_btrace();
            read_next_trace(nonmem, addr, rw, sub_block, cacheAT);
            //return 0;
        }
    }
    btrace = btrace_buf[btrace_buf_index];
    btrace_buf_index++;

    tmp = btrace;
    *cacheAT = (unsigned int)(tmp & CACHE_ACCESS_TYPE_MASK);
    tmp >>= CACHE_ACCESS_TYPE_BITS;     
    *sub_block = (unsigned int)(tmp & SUB_BLOCK_MASK);
    tmp >>= SUB_BLOCK_BITS; 
    *rw = (unsigned int)(tmp & RW_MASK);
    tmp >>= RW_BITS; 
    *addr = (tmp & MEM_ADDR_MASK);
    tmp >>= MEM_ADDR_BITS; 
    *nonmem = (unsigned int)(tmp & NONMEM_MASK);
    tmp >>= NON_MEM_BITS;
#ifdef StatisticsROB
    if(*sub_block != 0)
        datalength[*sub_block]++;
    else
        datalength[0] += *nonmem + 1;
#endif
    if(debug_count < 16)
    {
        //fprintf(stdout, "ROB%d: %lx--%u, %lx, %u, %u, %u\n", core_id, btrace, *nonmem, *addr, *rw, *sub_block, *cacheAT);
        debug_count++;
    }
    else
    {
        ;
    }
    return 1;
}
#else
int ReorderBuffer::read_next_trace(unsigned int *nonmem, unsigned long *addr, unsigned int *rw, unsigned int *sub_block, unsigned int *cacheAT)
{
    //static unsigned int debug_count = 0;
    unsigned long tmp;
read_trace:
    if(btrace_buf_index >= btrace_buf_capacity)
    {
        if(!reach_eof)
        {
            btrace_buf_capacity = (unsigned long)fread(btrace_buf, sizeof(unsigned long), BTRACE_BUF_SIZE, f_btrace);
            btrace_buf_index = 0;

            if(btrace_buf_capacity == 0)
            {
                cout<<"Trace file is over!"<<endl;
                fclose(f_btrace);
                init_btrace();
                read_next_trace(nonmem, addr, rw, sub_block, cacheAT);
                //return 0;   //end of the file
            }
            if(btrace_buf_capacity != BTRACE_BUF_SIZE)
            {
                reach_eof = 1;
                cout<<"Trace file is over!"<<endl;
            }

            printf("get %lu MB btrace from file with trace merge!\n", btrace_buf_capacity >> 20);
            cout<<"Window Size: "<<NUM_TRACES_PROCESS<<endl;
        }
        else 
        {
            cout<<"Trace file is over!"<<endl;
            fclose(f_btrace);
            init_btrace();
            read_next_trace(nonmem, addr, rw, sub_block, cacheAT);
            //return 0;
        }
    }


trace_process:
    if(traceStructIndex >= NUM_TRACES_PROCESS)
    {
        traceStructIndex = 0;
        if(btrace_buf_index >= btrace_buf_capacity)
        {
            goto read_trace;
        }
        for(size_t i = 0, j = btrace_buf_index; j < btrace_buf_index + NUM_TRACES_PROCESS && i < NUM_TRACES_PROCESS; i++, j++)
        {
            tmp = btrace_buf[j];
            traceStruct[i].cacheAT = (unsigned int)(tmp & CACHE_ACCESS_TYPE_MASK);
            tmp >>= CACHE_ACCESS_TYPE_BITS;
            traceStruct[i].sub_block = (unsigned int)(tmp & SUB_BLOCK_MASK);
            tmp >>= SUB_BLOCK_BITS;
            traceStruct[i].rw = (unsigned int)(tmp & RW_MASK);
            tmp >>= RW_BITS;
            traceStruct[i].addr = (tmp & MEM_ADDR_MASK);
            tmp >>= MEM_ADDR_BITS;
            traceStruct[i].nonmem = (unsigned int)(tmp & NONMEM_MASK);
            tmp >>= NON_MEM_BITS;
            traceStruct[i].isMerged = false;
            //cout<<"addr: "<<traceStruct[i].addr<<"  cacheAT:"<<traceStruct[i].cacheAT<<"    sub_block:  "<<traceStruct[i].sub_block<<endl;
         
#ifdef StatisticsROB
            if(traceStruct[i].sub_block != 0)
                datalength[traceStruct[i].sub_block]++;
            else
                datalength[0] += traceStruct[i].nonmem + 1;
#endif
                //cout<<"nonmem: "<<traceStruct[i].nonmem<<" addr: "<<traceStruct[i].addr<<"    rw: "<<traceStruct[i].rw<<"    sub_block: "<<traceStruct[i].sub_block<<"  cacheAT:"<<traceStruct[i].cacheAT<<endl;
        }
        //cout<<"coarn nonmem: "<<traceStruct[0].nonmem<<" addr: "<<traceStruct[0].addr<<"    rw: "<<traceStruct[0].rw<<"    sub_block: "<<traceStruct[0].sub_block<<"  cacheAT:"<<traceStruct[0].cacheAT<<endl;

        //cout<<"btrace_buf_index: "<<btrace_buf_index<<endl;
        btrace_buf_index += NUM_TRACES_PROCESS;

        //we can do sth to get coarn memory acces with traceStruct here.
        for(size_t j = 0; j < NUM_TRACES_PROCESS; j++)
        {
            if(traceStruct[j].cacheAT == 3 && traceStruct[j].isMerged == false)
            {
                if(traceStruct[j].rw == 0)
                {
                    for(size_t k = j + 1; k < NUM_TRACES_PROCESS; k++)
                    {
                        if(traceStruct[k].cacheAT == 3 && traceStruct[k].isMerged == false && traceStruct[k].rw == 0)
                        {
                            //we can change this with different sub_block meaning
                            if(traceStruct[j].addr + traceStruct[j].sub_block * 8 == traceStruct[k].addr && traceStruct[j].sub_block + traceStruct[k].sub_block <= 64)
                            {
                                bool dependencyFound = false;
                                for(size_t i = j + 1; i < k; i++)
                                {
                                    if((traceStruct[i].cacheAT == 3 && traceStruct[i].rw == 1) && ( (traceStruct[k].addr >= traceStruct[i].addr && traceStruct[k].addr <= traceStruct[i].addr + traceStruct[i].sub_block * 8) ||
                                                (traceStruct[i].addr >= traceStruct[k].addr && traceStruct[i].addr <= traceStruct[k].addr + traceStruct[k].sub_block * 8)))
                                    {
                                        dependencyFound = true;
                                    }
                                }
                                if(!dependencyFound)
                                {
                                    traceStruct[j].sub_block += traceStruct[k].sub_block;
                                    traceStruct[k].isMerged = true;
                                }
                            }
                            else if(traceStruct[k].addr + traceStruct[k].sub_block * 8 == traceStruct[j].addr && traceStruct[j].sub_block + traceStruct[k].sub_block <= 64)
                            {
                                bool dependencyFound = false;
                                for(size_t i = j + 1; i < k; i++)
                                {
                                    if((traceStruct[i].cacheAT == 3 && traceStruct[i].rw == 1) && ( (traceStruct[k].addr >= traceStruct[i].addr && traceStruct[k].addr <= traceStruct[i].addr + traceStruct[i].sub_block * 8) ||
                                                (traceStruct[i].addr >= traceStruct[k].addr && traceStruct[i].addr <= traceStruct[k].addr + traceStruct[k].sub_block * 8)))
                                    {
                                        dependencyFound = true;
                                    }
                                }
                                if(!dependencyFound)
                                {
                                    traceStruct[j].addr = traceStruct[k].addr;
                                    traceStruct[j].sub_block += traceStruct[k].sub_block;
                                    traceStruct[k].isMerged = true;
                                }
                            }
                        }
                    }
                }
                if(traceStruct[j].rw == 1)
                {
                    for(size_t k = j + 1; k < NUM_TRACES_PROCESS; k++)
                    {
                        if(traceStruct[k].cacheAT == 3 && traceStruct[k].isMerged == false && traceStruct[k].rw == 1)
                        {
                            //we can change this with different sub_block meaning
                            if(traceStruct[j].addr + traceStruct[j].sub_block * 8 == traceStruct[k].addr && traceStruct[j].sub_block + traceStruct[k].sub_block <= 512)
                            {
                                bool dependencyFound = false;
                                for(size_t i = j + 1; i < k; i++)
                                {
                                    if((traceStruct[i].cacheAT == 3 && traceStruct[i].rw == 0) && ( (traceStruct[k].addr >= traceStruct[i].addr && traceStruct[k].addr <= traceStruct[i].addr + traceStruct[i].sub_block * 8) ||
                                                (traceStruct[i].addr >= traceStruct[k].addr && traceStruct[i].addr <= traceStruct[k].addr + traceStruct[k].sub_block * 8)))
                                    {
                                        dependencyFound = true;
                                    }
                                }
                                if(!dependencyFound)
                                {
                                    traceStruct[j].sub_block += traceStruct[k].sub_block;
                                    traceStruct[k].isMerged = true;
                                }
                            }
                            else if(traceStruct[k].addr + traceStruct[k].sub_block * 8 == traceStruct[j].addr && traceStruct[j].sub_block + traceStruct[k].sub_block <= 512)
                            {
                                bool dependencyFound = false;
                                for(size_t i = j + 1; i < k; i++)
                                {
                                    if((traceStruct[i].cacheAT == 3 && traceStruct[i].rw == 0) && ( (traceStruct[k].addr >= traceStruct[i].addr && traceStruct[k].addr <= traceStruct[i].addr + traceStruct[i].sub_block * 8) ||
                                                (traceStruct[i].addr >= traceStruct[k].addr && traceStruct[i].addr <= traceStruct[k].addr + traceStruct[k].sub_block * 8)))
                                    {
                                        dependencyFound = true;
                                    }
                                }
                                if(!dependencyFound)
                                {
                                    traceStruct[j].addr = traceStruct[k].addr;
                                    traceStruct[j].sub_block += traceStruct[k].sub_block;
                                    traceStruct[k].isMerged = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        for(size_t i = 0, tempnonmem = 0; i < NUM_TRACES_PROCESS; i++)
        {
            tempnonmem += traceStruct[i].nonmem;
            if(traceStruct[i].isMerged == false)
            {
                traceStruct[i].nonmem = tempnonmem;
                tempnonmem = 0;
            }
        }
    }

    for(size_t i = traceStructIndex; i < NUM_TRACES_PROCESS; i++)
    {
        if(traceStruct[i].isMerged == false)
        {
            *cacheAT = traceStruct[i].cacheAT;
            *sub_block = traceStruct[i].sub_block;
            *rw = traceStruct[i].rw;
            *addr = traceStruct[i].addr;
            *nonmem = traceStruct[i].nonmem;
            traceStructIndex++;
            break;
        }
        else
        {
            traceStructIndex++;
            if(traceStructIndex >= NUM_TRACES_PROCESS)
            {
                return 2;
                /*
                cout<<"before"<<endl;
                goto trace_process; 
                cout<<"after"<<endl;
                */
            }
        }
    }

    
/*    if(debug_count < 16)
    {
        fprintf(stdout, "ROB%d: %lx--%u, %lx, %u, %u, %u\n", core_id, btrace, *nonmem, *addr, *rw, *sub_block, *cacheAT);
        debug_count++;
    }
    else
    {
        ;
    }*/
    return 1;
}
#endif

Transaction* ReorderBuffer::generateTransaction(unsigned long addr, unsigned gran, Transaction::TransactionType type)
{
	Transaction* tran = new Transaction(type,addr,NULL,gran*8);
	tran->threadID = core_id;
	return tran;
}

int ReorderBuffer::issueInstr(InstrType instrType, uint64_t maddr)
{
    //Before issuing Instr, it need to check whether the ROB is full,
    if(this->isFull())
	{
		return 0;
	}

    uint64_t currentClockCycle = Simulator::clockDomainCPU->clockcycle;

    	//the maddr is only useful for all cache miss
    	switch(instrType)
    	{
        	case NON_MEM:
            		comptime[tail] = currentClockCycle + NON_MEM_LATENCY;
            		break;
        	case MEM_WRITE:
            		comptime[tail] = currentClockCycle + NON_MEM_LATENCY;  /// just the same with non mem latency
            		break;
        	case L1_CACHE_HIT:
            		comptime[tail] = currentClockCycle + L1_CACHE_HIT_LATENCY;
            		break;
        	case L2_CACHE_HIT:
            		comptime[tail] = currentClockCycle + L2_CACHE_HIT_LATENCY;
            		break;
        	case L3_CACHE_HIT:
            		comptime[tail] = currentClockCycle + L3_CACHE_HIT_LATENCY;
            		break;
        	case ALL_CACHE_MISS:
            		comptime[tail] = currentClockCycle + ALL_CACHE_MISS_LATENCY; /// it is BIG_LATENCY
            		mem_address[tail] = maddr;
                    MSHRcounter++;
            		break;
        	default:
            		//invalid instr type
            		return -1;
    	}
        //cout<<"ROB"<<core_id<<" issueInstr: "<<instrType<<endl;
    	tail = (tail+1) % ROB_SIZE;
    	inflight++;
        TotalFetchinROB++;
    	return 1;
}

void ReorderBuffer::retireMemInstr(Transaction tran)
{
	int pos = head;
    unsigned MatchOK = 0;
	for(unsigned i = 0; i < inflight; i++)
	{
		if(mem_address[pos] == tran.address)
		{
            if(comptime[pos] != 0)
            {
                comptime[pos] = 0;
                MSHRcounter--;
                MatchOK = 1;
			    break;
            }
		}
		pos = (pos+1) % ROB_SIZE;
	}
    if(MatchOK == 0)
        cout<<"Match not OK in ROB!"<<endl;
}

int ReorderBuffer::isCommitted() {return (comptime[head] <= Simulator::clockDomainCPU->clockcycle);}
