#ifndef NODE_LLVM_H
#define NODE_LLVM_H

#include "nan.h"

#include <sstream>
#include <string>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Bitcode/ReaderWriter.h"

#define REQ_FUN_ARG(I, VAR)                                             \
  if (info.Length() <= (I) || !info[I]->IsFunction()) {			\
    char buf[256];							\
    snprintf(buf, 256, "Argument " #I " must be a function at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
  }									\
  Local<Function> VAR = Local<Function>::Cast(info[I]);

#define REQ_ARRAY_ARG(I, VAR)                                           \
  if (info.Length() <= (I) || !info[I]->IsArray()) {			\
    char buf[256];							\
    snprintf(buf, 256, "Argument " #I " must be an array at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
  }									\
  Local<Array> VAR = Local<Array>::Cast(info[I]);

#define REQ_INT_ARG(I, VAR)						\
  if (info.Length() <= (I) || !info[I]->IsNumber()) {			\
    char buf[256];							\
    snprintf(buf, 256, "Argument " #I " must be an integer at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
  }									\
  int64_t VAR = (int64_t)info[I]->NumberValue();


#define REQ_DOUBLE_ARG(I, VAR)                                           \
  if (info.Length() <= (I) || !info[I]->IsNumber()) {			\
    char buf[256];							\
    snprintf(buf, 256, "Argument " #I " must be a number at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
  }									\
  double VAR = info[I]->NumberValue();

#define REQ_BOOL_ARG(I, VAR)                                           \
  if (info.Length() <= (I) || !info[I]->IsBoolean()) {			\
    char buf[256];							\
    snprintf(buf, 256, "Argument " #I " must be a bool at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
  }									\
  bool VAR = info[I]->BooleanValue();

#define REQ_UTF8_ARG(I, VAR)						\
  if (info.Length() <= (I) || !info[I]->IsString()) {			\
    char buf[256];							\
    snprintf(buf, 256, "Argument " #I " must be a string at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
  }									\
  Local<String> VAR##_str = Local<String>::Cast(info[I]);		\
  Nan::Utf8String VAR(VAR##_str);

#define FALLBACK_UTF8_ARG(I, VAR, FALLBACK)				\
  Local<String> VAR##_str;						\
  if (info.Length() <= (I)) {						\
    VAR##_str = Nan::New("").ToLocalChecked();				\
  }									\
  else {								\
    if (!info[I]->IsString()) {						\
      char buf[256];							\
      snprintf(buf, 256, "Argument " #I " must be a string at %s:%d\n", __FILE__, __LINE__); \
    Nan::ThrowTypeError(buf);						\
    }									\
    VAR##_str = Local<String>::Cast(info[I]);				\
  }									\
  Nan::Utf8String VAR(VAR##_str);

#define FALLBACK_EMPTY_UTF8_ARG(I, VAR) FALLBACK_UTF8_ARG(I, VAR, "")

#define REQ_LLVM_VAL_ARG(I, VAR)					\
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Value::HasInstance(info[I]) */) {	\
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm Value");	\
  }									\
  ::llvm::Value* VAR = jsllvm::Value::GetLLVMObj(info[I]);

#define REQ_LLVM_CONST_ARG(I, VAR)					\
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Constant::HasInstance(info[I]) */) {	\
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm Constant");	\
  }									\
  ::llvm::Constant* VAR = static_cast< ::llvm::Constant*>(jsllvm::Value::GetLLVMObj(info[I]));

#define REQ_NULLABLE_LLVM_CONST_ARG(I, VAR)					\
  ::llvm::Constant* VAR;						\
  if (info.Length() > (I) && info[I]->IsNull())				\
    VAR = NULL;								\
  else if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Constant::HasInstance(info[I]) */) { \
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm Constant");	\
  }									\
  else									\
    VAR = static_cast< ::llvm::Constant*>(jsllvm::Value::GetLLVMObj(info[I]));

#define REQ_LLVM_CONST_INT_ARG(I, VAR)					\
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Constant::HasInstance(info[I]) */) {	\
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm ConstantInt");	\
  }									\
  ::llvm::ConstantInt* VAR = static_cast< ::llvm::ConstantInt*>(jsllvm::Value::GetLLVMObj(info[I]));

#define REQ_LLVM_MODULE_ARG(I, VAR)					\
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Constant::HasInstance(info[I]) */) {	\
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm Module");	\
  }									\
  ::llvm::Module* VAR = jsllvm::Module::GetLLVMObj(info[I]);

#define REQ_LLVM_TYPE_ARG(I, VAR)					\
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Type::HasInstance(info[I]) */) \
    Nan::ThrowTypeError("Argument " #I " must be an llvm Type"); \
  ::llvm::Type* VAR = jsllvm::Type::GetLLVMObj(info[I]);

#define REQ_LLVM_BB_ARG(I, VAR)						\
  if (info.Length() <= (I) || (!info[I]->IsNull() && !info[I]->IsObject() /* XXX && !jsllvm::BasicBlock::HasInstance(info[I]) */)) { \
    Nan::ThrowTypeError("Argument " #I " must be an llvm BasieBlock");	\
  }									\
  ::llvm::BasicBlock* VAR = jsllvm::BasicBlock::GetLLVMObj(info[I]);

#define REQ_LLVM_FUN_ARG(I, VAR) \
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::Function::HasInstance(info[I]) */) {  \
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm Function");	\
  }									\
  ::llvm::Function* VAR = jsllvm::Function::GetLLVMObj(info[I]);

#define REQ_LLVM_DIFILE_ARG(I, VAR) \
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::DIFile::HasInstance(info[I]) */) {  \
    Nan::ThrowTypeError("Argument " #I " must be an llvm DIFile");	\
  }									\
  ::llvm::DIFile VAR = jsllvm::DIFile::GetLLVMObj(info[I]);

#define REQ_LLVM_DISCOPE_ARG(I, VAR) \
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::DIScope::HasInstance(info[I]) */) {  \
    Nan::ThrowTypeError("Argument " #I " must be an llvm DIScope");	\
  }									\
  ::llvm::DIScope VAR = jsllvm::DIScope::GetLLVMObj(info[I]);

#define REQ_LLVM_DEBUGLOC_ARG(I, VAR) \
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::DIScope::HasInstance(info[I]) */) {  \
    Nan::ThrowTypeError("Argument " #I " must be an llvm DebugLoc");	\
  }									\
  ::llvm::DebugLoc VAR = jsllvm::DebugLoc::GetLLVMObj(info[I]);

#define REQ_LLVM_MDNODE_ARG(I, VAR)					\
  if (info.Length() <= (I) || !info[I]->IsObject() /* XXX || !jsllvm::MDNode::HasInstance(info[I]) */) {	\
    printf ("in function %s\n", __PRETTY_FUNCTION__);			\
    Nan::ThrowTypeError("Argument " #I " must be an llvm MDNode");	\
  }									\
  ::llvm::MDNode* VAR = jsllvm::MDNode::GetLLVMObj(info[I]);

extern std::string& trim(std::string& str);


namespace jsllvm {
#if NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION
#define PERSISTENT_TO_LOCAL(T, v) ::v8::Local<T>::New(::v8::Isolate::GetCurrent(), v);
#else
#define PERSISTENT_TO_LOCAL(T, v) ::v8::Local<T>::New(v)
#endif

// a handle little template to save a lot of typing
template<class LLVMTy, class JSLLVMTy>
class LLVMObjectWrap : public Nan::ObjectWrap {
public:
  LLVMObjectWrap(LLVMTy* llvm_obj) : llvm_obj(llvm_obj) { }
  static v8::Local<v8::Value> Create(LLVMTy* llvm_obj) {
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Object> new_instance = Nan::New(JSLLVMTy::constructor_func)->NewInstance();
    JSLLVMTy* new_a = new JSLLVMTy(llvm_obj);
    new_a->Wrap(new_instance);
    return scope.Escape(new_instance);
  }

  static LLVMTy* GetLLVMObj(v8::Local<v8::Value> value) {
    if (value->IsNull())
      return nullptr;
    return Nan::ObjectWrap::Unwrap<LLVMObjectWrap<LLVMTy, JSLLVMTy>>(value->ToObject())->llvm_obj;
  }

  static JSLLVMTy* Unwrap(v8::Local<v8::Object> handle) {
    return ObjectWrap::Unwrap<JSLLVMTy>(handle);
  }

 protected:
  LLVMTy* llvm_obj;
};

};

#endif /* NODE_LLVM_H */
