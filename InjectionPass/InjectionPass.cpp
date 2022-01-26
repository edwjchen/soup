#include <set>
#include <string>
#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <iostream>

// referenced github.com/imdea-software/LLVM_Instrumentation_Pass

using namespace llvm;

cl::list<std::string> FunctionList(
	"injection",
	cl::desc("Functions to instrument"),
        cl::value_desc("function name"),
	cl::OneOrMore
);

namespace {
  class InjectionPass : public ModulePass {
  public:
    const char *INJ_FUNCTION_STR = "inj_function_call"; 
    const char *INIT_FUNCTION_STR = "inj_init";

    static char ID;
    std::set<std::string> funcsToInst;

    InjectionPass() : ModulePass(ID) { 
      for (unsigned i = 0; i != FunctionList.size(); ++i) {
        funcsToInst.insert(FunctionList[i]);	  
      }
    }

    // Print output for each function
    bool runOnModule(Module &M) override {
      declare_injection_functions(M);
      for (Module::iterator mi = M.begin(); mi != M.end(); ++mi) {
        Function &f = *mi;
        std::string fname = f.getName();
        errs() << fname << "\n";

        if (fname == "main") {
          initializeInjection(M, f);
        }
        if (funcsToInst.count(fname) != 0) {
          instrumentFunction(M, f);
        }
      }

      return true; // modifies
    }

    void initializeInjection(Module &M, Function &f) {
      BasicBlock &entryBlock = f.getEntryBlock();
      Function *initFunction = M.getFunction(INIT_FUNCTION_STR);
      CallInst::Create(initFunction, "", entryBlock.getFirstNonPHI());
    }

    void instrumentFunction(Module &M, Function &f) {
      BasicBlock &entryBlock = f.getEntryBlock();
      Instruction *firstInstr = entryBlock.getFirstNonPHI();

      IRBuilder<> builder(firstInstr);
      Value *strPointer = builder.CreateGlobalStringPtr(f.getName());
  
      Function *injFunction = M.getFunction(INJ_FUNCTION_STR);

      std::vector<Value *>args;
      args.push_back(strPointer);

      CallInst::Create(injFunction, args, "", entryBlock.getFirstNonPHI());
    }

    void declare_injection_functions(Module &m) {
      LLVMContext &C = m.getContext();
      Type *voidTy = Type::getVoidTy(C);
      Type *IntTy64 = Type::getInt64Ty(C);
      Type *StringType = Type::getInt8PtrTy(C);

      bool isVarArg = false;

      std::vector<Type*> functionCallParams;
      functionCallParams.push_back(StringType);
      FunctionType *functionCallType = FunctionType::get(
        voidTy, functionCallParams, isVarArg
        );
      FunctionType *initFunctionType = FunctionType::get(
        IntTy64, isVarArg
        );

      m.getOrInsertFunction(INJ_FUNCTION_STR, functionCallType);
      m.getOrInsertFunction(INIT_FUNCTION_STR, initFunctionType);
    }
  };
}

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char InjectionPass::ID = 0;
static RegisterPass<InjectionPass> X("inject_function_calls", "Inject Function Calls Pass", false, false);
