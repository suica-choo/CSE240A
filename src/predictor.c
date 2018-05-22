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
map<uint32_t, uint32_t> lhistory;
map<uint32_t, uint8_t> BHT;
uint32_t ghistoryMask;

void init_predictor_gshare() {
  for (int i = 0; i < ghistoryBits; i++)
    ghistoryMask = (ghistoryMask << 1) + 1;
}

void init_predictor_tournament() {

}

void init_predictor_custom() {

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

}

uint8_t make_prediction_custom(uint32_t pc) {

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

}

void train_predictor_custom(uint32_t pc, uint8_t outcome) {

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

