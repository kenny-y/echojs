/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99 ft=cpp:
 */

#include "ejs-llvm.h"
#include "ejs-object.h"
#include "ejs-value.h"
#include "ejs-array.h"
#include "ejs-function.h"
#include "irbuilder.h"
#include "type.h"
#include "value.h"
#include "landingpad.h"
#include "switch.h"
#include "callinvoke.h"
#include "basicblock.h"

static llvm::IRBuilder<> _llvm_builder(llvm::getGlobalContext());

ejsval _ejs_llvm_IRBuilder_proto;
ejsval _ejs_llvm_IRBuilder;
static ejsval
_ejs_llvm_IRBuilder_impl (ejsval env, ejsval _this, int argc, ejsval *args)
{
    EJS_NOT_IMPLEMENTED();
}

ejsval
_ejs_llvm_IRBuilder_new(llvm::IRBuilder<>* llvm_fun)
{
    return _ejs_object_create (_ejs_null);
}


ejsval
_ejs_llvm_IRBuilder_setInsertPoint(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_BB_ARG(0, bb);
    if (bb != NULL)
        _llvm_builder.SetInsertPoint (bb);
    return _ejs_undefined;
}

ejsval
_ejs_llvm_IRBuilder_setInsertPointStartBB(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_BB_ARG(0, bb);
    if (bb != NULL)
        _llvm_builder.SetInsertPoint (bb, bb->getFirstInsertionPt());
    return _ejs_undefined;
}

ejsval
_ejs_llvm_IRBuilder_getInsertBlock(ejsval env, ejsval _this, int argc, ejsval *args)
{
    llvm::BasicBlock *llvm_bb = _llvm_builder.GetInsertBlock();
    if (llvm_bb)
        return _ejs_llvm_BasicBlock_new (llvm_bb);
    return _ejs_null;
}

ejsval
_ejs_llvm_IRBuilder_createRet(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0,val);
    return _ejs_llvm_Value_new(_llvm_builder.CreateRet(val));
}

ejsval
_ejs_llvm_IRBuilder_createPointerCast(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0,val);
    REQ_LLVM_TYPE_ARG(1,ty);
    REQ_UTF8_ARG(2,name);

    return _ejs_llvm_Value_new(_llvm_builder.CreatePointerCast(val, ty, name));
}

ejsval
_ejs_llvm_IRBuilder_createFPCast(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0,val);
    REQ_LLVM_TYPE_ARG(1,ty);
    REQ_UTF8_ARG(2,name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateFPCast(val, ty, name));
}

ejsval
_ejs_llvm_IRBuilder_createCall(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, callee);
    REQ_ARRAY_ARG(1, argv);
    REQ_UTF8_ARG(2, name);

    std::vector<llvm::Value*> ArgsV;
    for (unsigned i = 0, e = EJSARRAY_LEN(argv); i != e; ++i) {
        ArgsV.push_back (_ejs_llvm_Value_GetLLVMObj(EJSARRAY_ELEMENTS(argv)[i]));
        if (ArgsV.back() == 0) abort(); // XXX throw an exception here
    }

    return _ejs_llvm_Call_new (_llvm_builder.CreateCall(callee, ArgsV, name));
}

ejsval
_ejs_llvm_IRBuilder_createInvoke(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, callee);
    REQ_ARRAY_ARG(1, argv);
    REQ_LLVM_BB_ARG(2, normal_dest);
    REQ_LLVM_BB_ARG(3, unwind_dest);
    REQ_UTF8_ARG(4, name);

    std::vector<llvm::Value*> ArgsV;
    for (unsigned i = 0, e = EJSARRAY_LEN(argv); i != e; ++i) {
        ArgsV.push_back (_ejs_llvm_Value_GetLLVMObj(EJSARRAY_ELEMENTS(argv)[i]));
        if (ArgsV.back() == 0) abort(); // XXX throw an exception here
    }

    return _ejs_llvm_Invoke_new (_llvm_builder.CreateInvoke(callee, normal_dest, unwind_dest, ArgsV, name));
}

ejsval
_ejs_llvm_IRBuilder_createFAdd(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, left);
    REQ_LLVM_VAL_ARG(1, right);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateFAdd(left, right, name));
}

ejsval
_ejs_llvm_IRBuilder_createAlloca(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_TYPE_ARG(0, ty);
    REQ_UTF8_ARG(1, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateAlloca(ty, 0, name));
}

ejsval
_ejs_llvm_IRBuilder_createLoad(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);
    REQ_UTF8_ARG(1, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateLoad(val, name));
}

ejsval
_ejs_llvm_IRBuilder_createStore(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);
    REQ_LLVM_VAL_ARG(1, ptr);

    return _ejs_llvm_Value_new (_llvm_builder.CreateStore(val, ptr));
}

ejsval
_ejs_llvm_IRBuilder_createExtractElement(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);
    REQ_LLVM_VAL_ARG(1, idx);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateExtractElement(val, idx, name));
}

ejsval
_ejs_llvm_IRBuilder_createGetElementPointer(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);
    REQ_ARRAY_ARG(1, idxv);
    REQ_UTF8_ARG(2, name);

    std::vector<llvm::Value*> IdxV;
    for (unsigned i = 0, e = EJSARRAY_LEN(idxv); i != e; ++i) {
        IdxV.push_back (_ejs_llvm_Value_GetLLVMObj(EJSARRAY_ELEMENTS(idxv)[i]));
        if (IdxV.back() == 0) abort(); // XXX throw an exception here
    }

    return _ejs_llvm_Value_new (_llvm_builder.CreateGEP(val, IdxV, name));
}

ejsval
_ejs_llvm_IRBuilder_createInBoundsGetElementPointer(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);
    REQ_LLVM_VAL_ARG(1, idx);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateInBoundsGEP(val, idx, name));
}

ejsval
_ejs_llvm_IRBuilder_createStructGetElementPointer(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);
    REQ_INT_ARG(1, idx);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateStructGEP(val, idx, name));
}

ejsval
_ejs_llvm_IRBuilder_createICmpEq(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, left);
    REQ_LLVM_VAL_ARG(1, right);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateICmpEQ(left, right, name));
}

ejsval
_ejs_llvm_IRBuilder_createICmpSGt(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, left);
    REQ_LLVM_VAL_ARG(1, right);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateICmpSGT(left, right, name));
}

ejsval
_ejs_llvm_IRBuilder_createBr(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_BB_ARG(0, dest);

    return _ejs_llvm_Value_new (_llvm_builder.CreateBr(dest));
}

ejsval
_ejs_llvm_IRBuilder_createCondBr(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, cond);
    REQ_LLVM_BB_ARG(1, thenPart);
    REQ_LLVM_BB_ARG(2, elsePart);

    return _ejs_llvm_Value_new (_llvm_builder.CreateCondBr(cond, thenPart, elsePart));
}

ejsval
_ejs_llvm_IRBuilder_createPhi(ejsval env, ejsval _this, int argc, ejsval *args)
{
    abort();
#if notyet
    REQ_LLVM_TYPE_ARG(0, ty);
    REQ_INT_ARG(1, incoming_values);
    REQ_UTF8_ARG(2, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreatePHI(ty, incoming_values, name));
#endif
}

ejsval
_ejs_llvm_IRBuilder_createGlobalStringPtr(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_UTF8_ARG(0, val);
    REQ_UTF8_ARG(1, name);

    return _ejs_llvm_Value_new (_llvm_builder.CreateGlobalStringPtr(val, name));
}

ejsval
_ejs_llvm_IRBuilder_createUnreachable(ejsval env, ejsval _this, int argc, ejsval *args)
{
    return _ejs_llvm_Value_new (_llvm_builder.CreateUnreachable());
}

ejsval
_ejs_llvm_IRBuilder_createSwitch(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, V);
    REQ_LLVM_BB_ARG(1, Dest);
    REQ_INT_ARG(2, num_cases);

    return _ejs_llvm_Switch_new (_llvm_builder.CreateSwitch(V, Dest, num_cases));
}

ejsval
_ejs_llvm_IRBuilder_createLandingPad(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_TYPE_ARG(0, ty);
    REQ_LLVM_VAL_ARG(1, persFn);
    REQ_INT_ARG(2, num_clauses);
    REQ_UTF8_ARG(3, name);

    return _ejs_llvm_LandingPad_new (_llvm_builder.CreateLandingPad(ty, persFn, num_clauses, name));
}

ejsval
_ejs_llvm_IRBuilder_createResume(ejsval env, ejsval _this, int argc, ejsval *args)
{
    REQ_LLVM_VAL_ARG(0, val);

    return _ejs_llvm_Value_new (_llvm_builder.CreateResume(val));
}

void
_ejs_llvm_IRBuilder_init (ejsval exports)
{
    START_SHADOW_STACK_FRAME;

    _ejs_gc_add_named_root (_ejs_llvm_IRBuilder_proto);
    _ejs_llvm_IRBuilder_proto = _ejs_object_create(_ejs_Object_prototype);

    ADD_STACK_ROOT(ejsval, tmpobj, _ejs_function_new_utf8 (_ejs_null, "LLVMIRBuilder", (EJSClosureFunc)_ejs_llvm_IRBuilder_impl));
    _ejs_llvm_IRBuilder = tmpobj;


#define OBJ_METHOD(x) EJS_INSTALL_FUNCTION(_ejs_llvm_IRBuilder, EJS_STRINGIFY(x), _ejs_llvm_IRBuilder_##x)

    _ejs_object_setprop (_ejs_llvm_IRBuilder,       _ejs_atom_prototype,  _ejs_llvm_IRBuilder_proto);

    OBJ_METHOD(setInsertPoint);
    OBJ_METHOD(setInsertPointStartBB);
    OBJ_METHOD(getInsertBlock);
    OBJ_METHOD(createRet);
    OBJ_METHOD(createPointerCast);
    OBJ_METHOD(createFPCast);
    OBJ_METHOD(createCall);
    OBJ_METHOD(createInvoke);
    OBJ_METHOD(createFAdd);
    OBJ_METHOD(createAlloca);
    OBJ_METHOD(createLoad);
    OBJ_METHOD(createStore);
    OBJ_METHOD(createExtractElement);
    OBJ_METHOD(createGetElementPointer);
    OBJ_METHOD(createInBoundsGetElementPointer);
    OBJ_METHOD(createStructGetElementPointer);
    OBJ_METHOD(createICmpEq);
    OBJ_METHOD(createICmpSGt);
    OBJ_METHOD(createBr);
    OBJ_METHOD(createCondBr);
    OBJ_METHOD(createPhi);
    OBJ_METHOD(createGlobalStringPtr);
    OBJ_METHOD(createUnreachable);

    OBJ_METHOD(createSwitch);
    
    OBJ_METHOD(createLandingPad);
    OBJ_METHOD(createResume);

#undef OBJ_METHOD

    _ejs_object_setprop_utf8 (exports,              "IRBuilder", _ejs_llvm_IRBuilder);

    END_SHADOW_STACK_FRAME;
}