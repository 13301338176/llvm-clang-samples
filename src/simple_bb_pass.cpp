//------------------------------------------------------------------------------
// simple_bb_pass LLVM sample. Demonstrates:
//
// * How to write a BasicBlockPass
// * How to iterate over instructions in a basic block and detect instructions
//   of a certain type.
// * How to specify that our pass requires an analysis pass to run before it.
// * How to query for a type's size.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;


class AllocaSizeDetect : public BasicBlockPass {
public:
  AllocaSizeDetect() : BasicBlockPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const {
    // By implementing this method, we can specify to the pass manager which
    // passes are required by this pass so it makes sure to run them in the
    // right order.
    AU.addRequired<DataLayout>();
  }

  virtual bool runOnBasicBlock(BasicBlock &BB) {
    const DataLayout &DL = getAnalysis<DataLayout>();
    for (BasicBlock::iterator ii = BB.begin(), ii_e = BB.end(); ii != ii_e;
         ++ii) {
      // Iterate over each instruction in the BasicBlock. If the instruction
      // is an alloca, dump its type and query the type's size.
      if (AllocaInst *allocainst = dyn_cast<AllocaInst>(ii)) {
        Type *alloctype = allocainst->getAllocatedType();
        alloctype->print(outs());
        outs() << " size " << DL.getTypeSizeInBits(alloctype) << " bits\n";
      }
    }

    return false;
  }

  // The address of this member is used to uniquely identify the class. This is
  // used by the LLVM's own RTTI mechanism.
  static char ID;
};

char AllocaSizeDetect::ID = 0;


int main(int argc, char **argv) {
  if (argc < 2) {
    report_fatal_error("Expected an argument - IR file name");
  }

  // Parse the input LLVM IR file into a module.
  SMDiagnostic Err;
  Module *Mod = ParseIRFile(argv[1], Err, getGlobalContext());
  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  // Create a pass manager and fill it with the passes we want to run. Add a
  // DataLayout pass because we use its analysis results.
  PassManager PM;
  PM.add(new DataLayout(Mod));
  PM.add(new AllocaSizeDetect());
  PM.run(*Mod);

  return 0;
}
