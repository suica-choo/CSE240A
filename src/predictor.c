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
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

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

//
//TODO: Add your own Branch Predictor data structures here
//

uint32_t ghistory;
uint32_t ghistory2; // used for tournament predictor
map<uint32_t, uint32_t> lhistory;
map<uint32_t, uint8_t> BHT;
uint32_t ghistoryMask;
uint32_t lhistoryMask;
uint32_t pcIndexMask;
map<uint32_t, uint8_t> localBHT;
map<uint32_t, uint8_t> globalBHT;
map<uint32_t, uint8_t> selectors; // used for selecting between local and global predictor
map<uint32_t, uint8_t> customSelectors; // used for selecting between gshare and tournament predictor

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
    ghistoryBits = 9;
    lhistoryBits = 10;
    pcIndexBits = 10;
    init_predictor_tournament();
}

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
    uint32_t selectorAddress = ghistory2 & ghistoryMask;
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
        uint32_t globalHistory = ghistory2 & ghistoryMask;
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
    uint8_t selector = customSelectors.find(selectorAddress) == customSelectors.end() ? WT : customSelectors[selectorAddress];
    if(selector == WN || selector == SN){  // gshare predictor
        return make_prediction_gshare(pc);
    }
    else{ // tournament predictor
        return make_prediction_tournament(pc);
    }
}

void train_predictor_gshare(uint32_t pc, uint8_t outcome) {
  uint32_t address = (ghistoryMask & pc) ^ (ghistoryMask & ghistory);
  if (BHT.find(address) == BHT.end()) {
    BHT[address] = WN;
  }

  uint8_t counters = BHT[address];
  if (outcome == TAKEN) {
    if (counters == SN)
      BHT[address] = WN;
    else if (counters == WN)
      BHT[address] = WT;
    else if (counters == WT)
      BHT[address] = ST;
  }
  else if (outcome == NOTTAKEN) {
    if (counters == ST)
      BHT[address] = WT;
    else if (counters == WT)
      BHT[address] = WN;
    else if (counters == WN)
      BHT[address] = SN;
  }

  ghistory <<= 1;
  ghistory += outcome;
}

void train_predictor_tournament(uint32_t pc, uint8_t outcome) {
    uint32_t address = pc & pcIndexMask;
    uint32_t selectorAddress = ghistory2 & ghistoryMask;
    // initialization
    if(lhistory.find(address) == lhistory.end()){
        lhistory[address] = 0;
    }
    uint32_t localHistory = lhistory[address] & lhistoryMask;
    if(localBHT.find(localHistory) == localBHT.end()){
        localBHT[localHistory] = WN;
    }
    uint32_t globalHistory = ghistory2 & ghistoryMask;
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
        if(localResult == outcome){
            if (selectors[selectorAddress] == ST)
                selectors[selectorAddress] = WT;
            else if (selectors[selectorAddress] == WT)
                selectors[selectorAddress] = WN;
            else if (selectors[selectorAddress] == WN)
                selectors[selectorAddress] = SN;
        }
        else{
            if (selectors[selectorAddress] == SN)
                selectors[selectorAddress] = WN;
            else if (selectors[selectorAddress] == WN)
                selectors[selectorAddress] = WT;
            else if (selectors[selectorAddress] == WT)
                selectors[selectorAddress] = ST;
        }
    }
    
    // train the local predictor and the global predictor
    if(outcome == TAKEN){
        // local
        if (localBHT[localHistory] == SN)
            localBHT[localHistory] = WN;
        else if (localBHT[localHistory] == WN)
            localBHT[localHistory] = WT;
        else if (localBHT[localHistory] == WT)
            localBHT[localHistory] = ST;
        // global
        if (globalBHT[globalHistory] == SN)
            globalBHT[globalHistory] = WN;
        else if (globalBHT[globalHistory] == WN)
            globalBHT[globalHistory] = WT;
        else if (globalBHT[globalHistory] == WT)
            globalBHT[globalHistory] = ST;
    }
    else if(outcome == NOTTAKEN){
        // local
        if (localBHT[localHistory] == ST)
            localBHT[localHistory] = WT;
        else if (localBHT[localHistory] == WT)
            localBHT[localHistory] = WN;
        else if (localBHT[localHistory] == WN)
            localBHT[localHistory] = SN;
        // global
        if (globalBHT[globalHistory] == ST)
            globalBHT[globalHistory] = WT;
        else if (globalBHT[globalHistory] == WT)
            globalBHT[globalHistory] = WN;
        else if (globalBHT[globalHistory] == WN)
            globalBHT[globalHistory] = SN;
    }
    
    // update local history and global history
    lhistory[address] <<= 1;
    lhistory[address] += outcome;
    ghistory2 <<= 1;
    ghistory2 += outcome;
}

void train_predictor_custom(uint32_t pc, uint8_t outcome) {
    uint32_t selectorAddress = ghistory & ghistoryMask;
    if(customSelectors.find(selectorAddress) == customSelectors.end()){
        customSelectors[selectorAddress] = WT;
    }
    
    // train the selector
    uint8_t gshareResult = make_prediction_gshare(pc);
    uint8_t tournamentResult = make_prediction_tournament(pc);
    if(gshareResult != tournamentResult){
        if(gshareResult == outcome){
            if (customSelectors[selectorAddress] == ST)
                customSelectors[selectorAddress] = WT;
            else if (customSelectors[selectorAddress] == WT)
                customSelectors[selectorAddress] = WN;
            else if (customSelectors[selectorAddress] == WN)
                customSelectors[selectorAddress] = SN;
        }
        else{
            if (customSelectors[selectorAddress] == SN)
                customSelectors[selectorAddress] = WN;
            else if (customSelectors[selectorAddress] == WN)
                customSelectors[selectorAddress] = WT;
            else if (customSelectors[selectorAddress] == WT)
                customSelectors[selectorAddress] = ST;
        }
    }
    // train two predictors
    train_predictor_gshare(pc, outcome);
    train_predictor_tournament(pc, outcome);
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
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
  //
  //TODO: Implement prediction scheme
  //

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
  //
  //TODO: Implement Predictor training
  //
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

