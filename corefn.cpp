#include <iostream>
#include "codegen.h"
#include "node.h"

using namespace std;

extern int yyparse();
extern NBlock* programBlock;


llvm::Function* createPrintfFunction(CodeGenContext& context)
{
    std::vector<llvm::Type*> printf_arg_types;
    printf_arg_types.push_back(llvm::Type::getInt8PtrTy(MyContext)); //char*

    std::cout << "printf" << std::endl;

    llvm::FunctionType* printf_type =
        llvm::FunctionType::get(
            llvm::Type::getInt32Ty(MyContext), printf_arg_types, true);

    llvm::Function *func = llvm::Function::Create(
                printf_type, llvm::Function::ExternalLinkage,
                llvm::Twine("printf"),
                context.module
           );
    func->setCallingConv(llvm::CallingConv::C);
    return func;
}

void createPrintFunction(CodeGenContext& context, llvm::Function* printfFn, std::string functionName, std::string formatSpecifier) {
    std::vector<llvm::Type*> print_arg_types;
    if (formatSpecifier == "%d\n") {
        print_arg_types.push_back(llvm::Type::getInt64Ty(MyContext));
    } else if (formatSpecifier == "%f\n") {
        print_arg_types.push_back(llvm::Type::getDoubleTy(MyContext));
    }

    llvm::FunctionType* print_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(MyContext), print_arg_types, false);

    llvm::Function* func = llvm::Function::Create(
        print_type, llvm::Function::InternalLinkage, llvm::Twine(functionName), context.module);

    llvm::BasicBlock* bblock = llvm::BasicBlock::Create(MyContext, "entry", func, 0);
    context.pushBlock(bblock);

    const char* constValue = formatSpecifier.c_str();
    llvm::Constant* format_const = llvm::ConstantDataArray::getString(MyContext, constValue);
    llvm::GlobalVariable* var = new llvm::GlobalVariable(
        *context.module,
        llvm::ArrayType::get(llvm::IntegerType::get(MyContext, 8), strlen(constValue) + 1),
        true,
        llvm::GlobalValue::PrivateLinkage,
        format_const,
        ".str");

    llvm::Constant* zero = llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(MyContext));
    std::vector<llvm::Constant*> indices;
    indices.push_back(zero);
    indices.push_back(zero);

    llvm::Constant* var_ref = llvm::ConstantExpr::getGetElementPtr(
        llvm::ArrayType::get(llvm::IntegerType::get(MyContext, 8), strlen(constValue) + 1),
        var,
        indices);

    std::vector<llvm::Value*> args;
    args.push_back(var_ref);

    Function::arg_iterator argsValues = func->arg_begin();
    Value* toPrint = &*argsValues++;
    toPrint->setName("toValue");
    args.push_back(toPrint);

    CallInst* call = CallInst::Create(printfFn, makeArrayRef(args), "", bblock);
    ReturnInst::Create(MyContext, bblock);

    context.popBlock();
}

void createCoreFunctions(CodeGenContext& context) {
    llvm::Function* printfFn = createPrintfFunction(context);
    createPrintFunction(context, printfFn, "print", "%d\n");
    createPrintFunction(context, printfFn, "printDouble", "%f\n");
}

