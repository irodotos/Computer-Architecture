#include "cpu/pred/PAgPredictor.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"
#include <cmath>

namespace gem5
{

namespace branch_prediction
{

PAgPred::PAgPred(const PAgPredParams &params)
    : BPredUnit(params),
    ltableHeight(params.ltableHeight),
    lhistoryWidth(params.lhistoryWidth),
    gtableHeight(params.gtableHeight),
    gpredSize(params.gpredSize),
    pCtrs(gtableHeight , SatCounter8(gpredSize) ),
    localHistory(ltableHeight , 0),
    localIndexMask(ltableHeight - 1)
      // more parameters are needed to be passed here, check BranchPredictor.py
{
    // you may want to add some checks of the passed parameters here

    bitsAddress = sqrt(ltableHeight);

    if(!isPowerOf2(ltableHeight)){
        fatal("problem with ltableHeight (need to be power of 2) \n");
    }

    if(!isPowerOf2(gtableHeight)){
        fatal("problem with gtableHeight (need to be power of 2) \n");
    }
    if(pow(2 , lhistoryWidth) != gtableHeight){
        fatal("problem with gtableHeight (need to be equals to 2 ^ lhistoryWidth) \n");
    }


}

// unused
void
PAgPred::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
// Place holder for a function that is called to update predictor history when
// a BTB entry is invalid or not found.
}

bool
PAgPred::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    // implement the lookup logic here
    bool taken;

    unsigned localIndex = getLocalIndex(branch_addr);

    //printf("local index in histrory table = %#x \n" , localIndex);

    unsigned globalIndexPredictor = localHistory[localIndex];

    //printf("global predictor index in predictor = %#x \n" , globalIndexPredictor);

    uint8_t counter_value = pCtrs[globalIndexPredictor];

    //printf("prediction (0-3)  =  %i \n" , (int)counter_value);

    taken = getPrediction(counter_value);

    //printf("prediction 1 or 0  =  %i \n" , (int)taken);

    return taken;
}

void
PAgPred::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    // implement the update logic here
    if(squashed){
        //printf("squashed...........\n");
        return;
    }

    unsigned localIndex = getLocalIndex( branch_addr);
    unsigned globalIndexPredictor = localHistory[localIndex];


    if(taken){
        //printf("branch was taken \n");
        localHistory[localIndex] = (localHistory[localIndex] << 1) | 1;    // left shift local history and add 1 in the end
        localHistory[localIndex] = localHistory[localIndex] & ( (1 << lhistoryWidth) -1 );   // keep only historyWidth bits for history
        pCtrs[globalIndexPredictor]++;
    }
    else{
        //printf("brnahc was not taken \n");
        localHistory[localIndex] = (localHistory[localIndex] << 1);           // left shift local hisory and add 0 in the end
        localHistory[localIndex] = localHistory[localIndex] & ( (1 << lhistoryWidth) -1 );   // keep only historyWidth bits for history
        pCtrs[globalIndexPredictor]--;
    }

}

inline
unsigned
PAgPred::getLocalIndex(Addr &branch_addr){           // return local history index according to the address
    return (branch_addr >> instShiftAmt) & localIndexMask;
}

inline
bool
PAgPred::getPrediction(uint8_t &count){     // return the MSB of the count 1=taken , 0=not taken
    return (count >> (gpredSize - 1));
}

// unused
void
PAgPred::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
}

}

}
