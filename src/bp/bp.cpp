#include "pin.H"
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>

using std::cerr;
using std::endl;
using std::string;

/* ================================================================== */
// Global variables
/* ================================================================== */

std::ostream *out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "",
                            "specify file name for bp output");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage() {
  cerr << "This tool prints out the branch prediction accuracy of the "
          "application."
       << endl
       << endl;

  cerr << KNOB_BASE::StringKnobSummary() << endl;

  return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

enum BranchState { STRONG_NOT_TAKEN, WEAK_NOT_TAKEN, WEAK_TAKEN, STRONG_TAKEN };

struct BranchInfo {
  BranchState state;
  UINT32 miss;
  UINT32 hit;
};

// Simple 2-bit saturating counter branch predictor
class BranchPredictor {
  std::unordered_map<ADDRINT, BranchInfo> branchInfos;

public:
  BranchPredictor() {}

  void Prediction(ADDRINT pc, bool taken) {
    if (branchInfos.find(pc) == branchInfos.end()) {
      // initialize branch state
      branchInfos[pc] = {WEAK_TAKEN, 0, 0};
    }

    if (taken) {
      if (branchInfos[pc].state == STRONG_NOT_TAKEN) {
        branchInfos[pc].state = WEAK_NOT_TAKEN;
        branchInfos[pc].miss++;
      } else if (branchInfos[pc].state == WEAK_NOT_TAKEN) {
        branchInfos[pc].state = WEAK_TAKEN;
        branchInfos[pc].miss++;
      } else if (branchInfos[pc].state == WEAK_TAKEN) {
        branchInfos[pc].state = STRONG_TAKEN;
        branchInfos[pc].hit++;
      } else if (branchInfos[pc].state == STRONG_TAKEN) {
        branchInfos[pc].hit++;
      }
    } else {
      if (branchInfos[pc].state == STRONG_NOT_TAKEN) {
        branchInfos[pc].hit++;
      } else if (branchInfos[pc].state == WEAK_NOT_TAKEN) {
        branchInfos[pc].state = STRONG_NOT_TAKEN;
        branchInfos[pc].hit++;
      } else if (branchInfos[pc].state == WEAK_TAKEN) {
        branchInfos[pc].state = WEAK_NOT_TAKEN;
        branchInfos[pc].miss++;
      } else if (branchInfos[pc].state == STRONG_TAKEN) {
        branchInfos[pc].state = WEAK_TAKEN;
        branchInfos[pc].miss++;
      }
    }
  }

  void Print() {
    UINT32 totalMiss = 0;
    UINT32 totalHit = 0;

    auto cmp = [](const std::pair<ADDRINT, BranchInfo> &a,
                  const std::pair<ADDRINT, BranchInfo> &b) {
      return a.second.miss + a.second.hit < b.second.miss + b.second.hit;
    };

    std::priority_queue<std::pair<ADDRINT, BranchInfo>,
                        std::vector<std::pair<ADDRINT, BranchInfo>>,
                        decltype(cmp)>
        pq(cmp);
    for (const auto &kv : branchInfos) {
      pq.push(kv);
    }

    *out << "addr,state,miss,hit,accuracy" << endl;

    while (!pq.empty()) {
      auto const &[key, val] = pq.top();

      *out << "0x" << std::hex << key << std::dec << ",";
      if (val.state == STRONG_NOT_TAKEN) {
        *out << "STRONG_NOT_TAKEN";
      } else if (val.state == WEAK_NOT_TAKEN) {
        *out << "WEAK_NOT_TAKEN";
      } else if (val.state == WEAK_TAKEN) {
        *out << "WEAK_TAKEN";
      } else if (val.state == STRONG_TAKEN) {
        *out << "STRONG_TAKEN";
      }
      *out << "," << val.miss << "," << val.hit;

      // Calculate accuracy
      double accuracy = (double)val.hit / (double)(val.hit + val.miss);
      *out << "," << accuracy << endl;

      totalMiss += val.miss;
      totalHit += val.hit;

      pq.pop();
    }

    // Calculate total accuracy
    double totalAccuracy = (double)totalHit / (double)(totalHit + totalMiss);
    *out << "Total,N/A," << totalMiss << "," << totalHit << "," << totalAccuracy
         << endl;
  }
};

BranchPredictor bp;

VOID ProcessBranch(ADDRINT pc, bool taken) { bp.Prediction(pc, taken); }

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

VOID Instruction(INS ins, VOID *v) {
  if (INS_IsBranch(ins) && INS_HasFallThrough(ins)) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessBranch, IARG_INST_PTR,
                   IARG_BRANCH_TAKEN, IARG_END);
  }
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v) {
  // *out << "===============================================" << endl;
  // *out << "Branch Prediction Result" << endl;
  bp.Print();
  // *out << "===============================================" << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet
 * started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[]) {
  // Initialize PIN library. Print help message if -h(elp) is specified
  // in the command line or the command line is invalid
  if (PIN_Init(argc, argv)) {
    return Usage();
  }

  string fileName = KnobOutputFile.Value();

  if (!fileName.empty()) {
    out = new std::ofstream(fileName.c_str());
  }

  // Register Instruction to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, 0);

  // Register function to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  cerr << "===============================================" << endl;
  cerr << "This application is instrumented by CSPM bp" << endl;
  if (!KnobOutputFile.Value().empty()) {
    cerr << "See file " << KnobOutputFile.Value() << " for analysis results"
         << endl;
  }
  cerr << "===============================================" << endl;

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
