#include "cpu/pred/GApPredictor.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"
#include <cmath>

namespace gem5
{

namespace branch_prediction
{

GApPred::GApPred(const GApPredParams &params)
    : BPredUnit(params),
    historySize(params.historySize),
    ptableHeight(params.ptableHeight),
    ptableWidth(params.ptableWidth),
    predSize(params.predSize),
    indexMaskWidth(ptableWidth - 1),
    indexMaskHeight(ptableHeight - 1),
    pCtrs(ptableHeight , std::vector<SatCounter8>(ptableWidth , SatCounter8(predSize))),
    gHistory(1 ,0),
    globalHistoryMask(historySize - 1)
{
    // you may want to add some checks of the passed parameters here
    if(!isPowerOf2(ptableHeight)){
        fatal("Invalid predictor table height (not power of 2)\n");
    }
    if((pow(2 , historySize) != ptableHeight)){
        fatal("Invalid predictor table height (table height must be equal to power(2 , historySize)");
    }
    if(!isPowerOf2(ptableWidth)){
        fatal("Invalid predictor table width\n");
    }
    historyRegisterMask = mask(globalHistoryMask);
}

// unused
void
GApPred::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
// Place holder for a function that is called to update predictor history when
// a BTB entry is invalid or not found.
}

bool
GApPred::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    // implement the lookup logic here
    bool taken;
    unsigned indexHeight = getIndexHeight(tid);
    unsigned indexWidth = getIndexWidth(branch_addr);

    //printf("Looking up indexHeight =  %#x\n", indexHeight);
    //printf( "Looking up indexWidth = %#x\n", indexWidth);

    uint8_t counter_value = pCtrs[indexHeight][indexWidth];

    //printf( "prediction is with 2-bit predictor (0-3)  = %i.\n", (int)counter_value);

    taken = getPrediction(counter_value);

    //printf( "prediction is taken/not taken  = %i.\n", (int)taken);

    return taken;

}

void
GApPred::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    // implement the update logic here
    unsigned indexHeight = getIndexHeight(tid);
    unsigned indexWidth = getIndexWidth(branch_addr);

    // No state to restore, and we do not update on the wrong
    // path.
    if (squashed) {
        //printf("squashed...........\n");
        return;
    }

    //printf( "Looking up indexHeight to update %#x\n", indexHeight);
    //printf( "Looking up indexWidth to update %#x\n", indexWidth);

    //DPRINTF(Fetch, "Looking up index %#x\n", local_predictor_idx);

    if (taken) {
        //printf( "Branch updated as taken.\n");
        pCtrs[indexHeight][indexWidth]++;
        gHistory[0] = (gHistory[0] << 1) | 1;         // left shifting and adding 1 in the end
        gHistory[0] = gHistory[0] & ((1 << historySize )-1 );   //to get only as many LSbits as the historySize
    } else {
        //printf("Branch updated as not taken.\n");
        pCtrs[indexHeight][indexWidth]--;
        gHistory[0] = (gHistory[0] << 1);           // left shifting and adding 0 to the end
        gHistory[0] = gHistory[0] & ((1 << historySize )-1 );   //to get only as many LSbits as the historySize
    }
}

inline
bool
GApPred::getPrediction(uint8_t &count){    // return the MSB of count
    return (count >> (predSize - 1));
}

inline
unsigned
GApPred::getIndexHeight(ThreadID tid){    // return the local predictro's index heaight according history bits
    return gHistory[0];
}

inline
unsigned
GApPred::getIndexWidth(Addr &branch_addr){    // return the local predicor's index width according address
    return (branch_addr >> instShiftAmt) & indexMaskWidth;
}

// unused
void
GApPred::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
}


}

}
