//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <map>

using namespace std;

//
// Student Information
//
const char *studentName = "Ruochao Yan, Bohan Zhang";
const char *studentID   = "A53247716, A53247416";
const char *email       = "ruy007@ucsd.edu, boz082@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;


//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

uint32_t ghistory;
map<uint32_t, uint32_t> lhistory;
uint32_t ghistoryMask;
uint32_t lhistoryMask;
uint32_t pcIndexMask;
map<uint32_t, uint8_t> BHT;
map<uint32_t, uint8_t> localBHT;
map<uint32_t, uint8_t> globalBHT;
map<uint32_t, uint8_t> selectors; // used for choice predictor

// helper functions modifying the 2-bit counters
void move_down(map<uint32_t, uint8_t>& BHT, uint32_t address){
    if (BHT[address] == ST)
        BHT[address] = WT;
    else if (BHT[address] == WT)
        BHT[address] = WN;
    else if (BHT[address] == WN)
        BHT[address] = SN;
}
void move_up(map<uint32_t, uint8_t>& BHT, uint32_t address){
    if (BHT[address] == SN)
        BHT[address] = WN;
    else if (BHT[address] == WN)
        BHT[address] = WT;
    else if (BHT[address] == WT)
        BHT[address] = ST;
}

// initialization functions of three predictors
void init_predictor_gshare() {
  for (int i = 0; i < ghistoryBits; i++)
    ghistoryMask = (ghistoryMask << 1) + 1;
}
void init_predictor_tournament() {
    for (int i = 0; i < ghistoryBits; i++)
        ghistoryMask = (ghistoryMask << 1) + 1;
    for (int i = 0; i < lhistoryBits; i++)
        lhistoryMask = (lhistoryMask << 1) + 1;
    for (int i = 0; i < pcIndexBits; i++)
        pcIndexMask = (pcIndexMask << 1) + 1;
}
void init_predictor_custom() {
    ghistoryBits = 11;
    lhistoryBits = 7;
    pcIndexBits = 10;
    init_predictor_tournament();
}

// make prediction functions of three predictors
uint8_t make_prediction_gshare(uint32_t pc) {
  uint32_t address = (ghistoryMask & pc) ^ (ghistoryMask & ghistory);
  if (BHT.find(address) != BHT.end()) {
    uint8_t counters = BHT[address];
    if (counters == SN || counters == WN)
      return NOTTAKEN;
    else
      return TAKEN;
  }
  else
    return NOTTAKEN;
}

uint8_t make_prediction_tournament(uint32_t pc) {
    uint32_t selectorAddress = ghistory & ghistoryMask;
    uint8_t selector = selectors.find(selectorAddress) == selectors.end() ? WT : selectors[selectorAddress];
    if(selector == WN || selector == SN){  // local predictor
        uint32_t address = (pc & pcIndexMask);
        uint32_t validLocalHistory = lhistory.find(address) == lhistory.end() ? 0 : lhistory[address];
        uint32_t localHistory = validLocalHistory & lhistoryMask;
        if(localBHT.find(localHistory) != localBHT.end()){
            uint8_t counter = localBHT[localHistory];
            if(counter == SN || counter == WN)
                return NOTTAKEN;
            else
                return TAKEN;
        }
        else
            return NOTTAKEN;
    }
    else{ // global predictor
        uint32_t globalHistory = ghistory & ghistoryMask;
        if(globalBHT.find(globalHistory) != globalBHT.end()){
            uint8_t counter = globalBHT[globalHistory];
            if(counter == SN || counter == WN)
                return NOTTAKEN;
            else
                return TAKEN;
        }
        else
            return NOTTAKEN;
    }
}

uint8_t make_prediction_custom(uint32_t pc) {
    uint32_t selectorAddress = ghistory & ghistoryMask;
    uint8_t selector = selectors.find(selectorAddress) == selectors.end() ? WT : selectors[selectorAddress];
    if(selector == WN || selector == SN){  // gshare predictor
        return make_prediction_gshare(pc);
    }
    else{ // tournament local predictor
        uint32_t address = (pc & pcIndexMask);
        uint32_t validLocalHistory = lhistory.find(address) == lhistory.end() ? 0 : lhistory[address];
        uint32_t localHistory = validLocalHistory & lhistoryMask;
        if(localBHT.find(localHistory) != localBHT.end()){
            uint8_t counter = localBHT[localHistory];
            if(counter == SN || counter == WN)
                return NOTTAKEN;
            else
                return TAKEN;
        }
        else
            return NOTTAKEN;
    }
}

// trainning functions of three predictors
void train_predictor_gshare(uint32_t pc, uint8_t outcome) {
  uint32_t address = (ghistoryMask & pc) ^ (ghistoryMask & ghistory);
  if (BHT.find(address) == BHT.end()) {
    BHT[address] = WN;
  }
  if (outcome == TAKEN) {
      move_up(BHT, address);
  }
  else if (outcome == NOTTAKEN) {
      move_down(BHT, address);
  }
  ghistory <<= 1;
  ghistory += outcome;
}

void train_predictor_tournament(uint32_t pc, uint8_t outcome) {
    uint32_t address = pc & pcIndexMask;
    uint32_t selectorAddress = ghistory & ghistoryMask;
    // initialization
    if(lhistory.find(address) == lhistory.end()){
        lhistory[address] = 0;
    }
    uint32_t localHistory = lhistory[address] & lhistoryMask;
    if(localBHT.find(localHistory) == localBHT.end()){
        localBHT[localHistory] = WN;
    }
    uint32_t globalHistory = ghistory & ghistoryMask;
    if(globalBHT.find(globalHistory) == globalBHT.end()){
        globalBHT[globalHistory] = WN;
    }
    if(selectors.find(selectorAddress) == selectors.end()){
        selectors[selectorAddress] = WT;
    }
    // train the choose predictor
    uint8_t localResult = localBHT[localHistory] <= 1 ? NOTTAKEN : TAKEN;
    uint8_t globalResult = globalBHT[globalHistory] <= 1 ? NOTTAKEN : TAKEN;
    if(localResult != globalResult){
        if(localResult == outcome)
            move_down(selectors, selectorAddress);
        else
            move_up(selectors, selectorAddress);
    }
    // train the local predictor and the global predictor
    if(outcome == TAKEN){
        move_up(localBHT, localHistory);
        move_up(globalBHT, globalHistory);
    }
    else if(outcome == NOTTAKEN){
        move_down(localBHT, localHistory);
        move_down(globalBHT, globalHistory);
    }
    // update local history and global history
    lhistory[address] <<= 1;
    lhistory[address] += outcome;
    ghistory <<= 1;
    ghistory += outcome;
}

/*
    Our customized predictor uses a choice predictor to choose between a gshare predictor and a local predictor in tournament predictor.
    The gshare predictor uses 11-bit pattern for the global history, therefore costs 2^12 = 4KB.
    The local predictor uses 10-bit address and 7-bit local history, therefore costs 2^10*7 + 2^7*2 = 7KB + 256.
    The choice predictor uses 11-bit pattern, therefore costs 2^12 = 4KB.
    The total size is 4KB + 7KB + 256 + 4KB + 32 = 15KB + 288 < 16KB + 256.
*/
void train_predictor_custom(uint32_t pc, uint8_t outcome) {
    // initialization
    uint32_t selectorAddress = ghistory & ghistoryMask;
    if(selectors.find(selectorAddress) == selectors.end()){
        selectors[selectorAddress] = WT;
    }
    uint32_t address = pc & pcIndexMask;
    if(lhistory.find(address) == lhistory.end()){
        lhistory[address] = 0;
    }
    uint32_t localHistory = lhistory[address] & lhistoryMask;
    if(localBHT.find(localHistory) == localBHT.end()){
        localBHT[localHistory] = WN;
    }
    
    // train the selector
    uint8_t localResult = localBHT[localHistory] <= 1 ? NOTTAKEN : TAKEN;
    uint8_t gshareResult = make_prediction_gshare(pc);
    
    if(gshareResult != localResult){
        if(gshareResult == outcome)
            move_down(selectors, selectorAddress);
        else
            move_up(selectors, selectorAddress);
    }
    
    // train the gshare predictor
    train_predictor_gshare(pc, outcome);

    // train the local predictor
    if(outcome == TAKEN)
        move_up(localBHT, localHistory);
    else if(outcome == NOTTAKEN)
        move_down(localBHT, localHistory);

    // update local history and global history
    lhistory[address] <<= 1;
    lhistory[address] += outcome;
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //TODO: Initialize Branch Predictor Data Structures
  switch (bpType) {
    case STATIC:
      return;
    case GSHARE:
      return init_predictor_gshare();
    case TOURNAMENT:
      return init_predictor_tournament();
    case CUSTOM:
      return init_predictor_custom();
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return make_prediction_gshare(pc);
    case TOURNAMENT:
      return make_prediction_tournament(pc);
    case CUSTOM:
      return make_prediction_custom(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //Predictor training
  switch (bpType) {
    case STATIC:
      return;
    case GSHARE:
      return train_predictor_gshare(pc, outcome);
    case TOURNAMENT:
      return train_predictor_tournament(pc, outcome);
    case CUSTOM:
      return train_predictor_custom(pc, outcome);
    default:
      break;
  }
}

