#ifndef REORDERBUFFER_H
#define REORDERBUFFER_H

#include <iostream>
//#include "Simulator.h"
#include "SystemConfiguration.h"
#include "Transaction.h"
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "assert.h"

using namespace std;
using namespace DRAMSim;


namespace DRAMSim
{

enum InstrType
{
	NON_MEM,		///non memory instruction, the latency is fixed: PIPELINEDEPTH
	MEM_WRITE,		///memory write instruction
    L1_CACHE_HIT,     	///memory instr and hit in L1 Cache, latency: PIPELINEDEPTH + L1_CACHE_HIT_LATENCY
    L2_CACHE_HIT,     	///memory instr, miss in L1 Cache but hit in L2 Cache, latency: PIPE + L2_CACHE_HIT_LATENCY
    L3_CACHE_HIT,     	///memory instr, miss in L1 Cache, L2 Cache, but hit in shared L3 Cache, PIPE + L3_CACHE_HIT_LATENCY
    ALL_CACHE_MISS,   	///memory instr, miss in all caches, need access DRAM system,
};

class ReorderBuffer
{
private:
	int core_id; ///the core id of the re-order buffer
	class Simulator *simulator;
    unsigned head;
    unsigned tail;
    uint64_t *comptime;
    uint64_t *mem_address;
    int *optype;
    uint64_t *instrpc;
    int tracedone;

    FILE *f_btrace;
    string fname_btrace;
    unsigned long *btrace_buf;
    unsigned long btrace_buf_index;

    ///< wangyn added
    size_t traceStructIndex;
    ///< wangyn added end
    //
    unsigned long btrace_buf_capacity;
    int reach_eof;
    unsigned int waiting_nonmem;

    unsigned int nonmem;
    unsigned long addr;
    unsigned int rw;
    unsigned int sub_block;
    unsigned int cacheAT;
    unsigned pending_tran;
    unsigned MSHRcounter;

    Transaction *waiting_transaction;
    Transaction *generated_transaction;

public:
    uint64_t datalength[16];
    unsigned inflight;
    uint64_t TotalCommitinROB;
    uint64_t TotalFetchinROB;
    uint64_t TotalAddinROB;
	ReorderBuffer(int id, string file_name, Simulator *sim);
    ~ReorderBuffer();
    unsigned getID() {return core_id;}
    unsigned getInflight() {return inflight;}
	int isEmpty() {return (inflight == 0);}
    int isFull() {return (inflight == ROB_SIZE);}
	int isCommitted();
    void init_btrace();
    void close_btrace();
    int read_next_trace(unsigned int *nonmem, unsigned long *addr, unsigned int *rw, unsigned int *sub_block, unsigned int *cacheAT);
    void update();

    int issueInstr(InstrType instrType, uint64_t maddr=0);
	void retireMemInstr(Transaction *tran);
	Transaction* generateTransaction(unsigned long addr, unsigned gran, Transaction::TransactionType type);

	bool isTraceOver() {return (reach_eof == 1);}
};
}

#endif
