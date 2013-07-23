#include "node-llvm.h"
#include "dibuilder.h"
#include "function.h"
#include "module.h"
#include "type.h"
#include "value.h"

using namespace node;
using namespace v8;


namespace jsllvm {

  // DIBuilder

  void DIBuilder::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DIBuilder"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "createCompileUnit", DIBuilder::CreateCompileUnit);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "createFile", DIBuilder::CreateFile);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "createFunction", DIBuilder::CreateFunction);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "finalize", DIBuilder::Finalize);

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DIBuilder"),
		s_func);
  }

  v8::Handle<v8::Value> DIBuilder::New(llvm::DIBuilder *llvm_dibuilder)
  {
    HandleScope scope;
    Local<Object> new_instance = DIBuilder::s_func->NewInstance();
    DIBuilder* new_dibuilder = new DIBuilder(llvm_dibuilder);
    new_dibuilder->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DIBuilder::New(const Arguments& args)
  {
    HandleScope scope;

    if (args.Length()) {
      REQ_LLVM_MODULE_ARG(0, module);

      DIBuilder* dib = new DIBuilder(new llvm::DIBuilder (*module));
      dib->Wrap(args.This());
    }
    return scope.Close(args.This());
  }

  DIBuilder::DIBuilder (llvm::DIBuilder *llvm_dibuilder)
    : llvm_dibuilder (llvm_dibuilder)
  {
    Initialize();
  }

  DIBuilder::DIBuilder()
    : llvm_dibuilder (NULL)
  {
  }


  DIBuilder::~DIBuilder()
  {
  }

  void
  DIBuilder::Initialize ()
  {
    llvm::DIType int32Type = llvm_dibuilder->createBasicType ("EJSValue", 4, 4, llvm::dwarf::DW_ATE_signed);
    ejsValueType = llvm_dibuilder->createPointerType (int32Type, 4);
  }

  Handle<v8::Value> DIBuilder::CreateCompileUnit(const Arguments& args)
  {
    HandleScope scope;
    DIBuilder* dib = ObjectWrap::Unwrap<DIBuilder>(args.This());
  
    REQ_UTF8_ARG(0, file);
    REQ_UTF8_ARG(1, dir);
    REQ_UTF8_ARG(2, producer);
    REQ_BOOL_ARG(3, isOptimized);
    REQ_UTF8_ARG(4, flags);
    REQ_INT_ARG(5, runtimeVersion);

    dib->llvm_dibuilder->createCompileUnit(llvm::dwarf::DW_LANG_C99,
					   *file, *dir,
					   *producer,
					   isOptimized,
					   *flags,
					   runtimeVersion);
    return scope.Close(Undefined());
  }

  Handle<v8::Value> DIBuilder::CreateFile(const Arguments& args)
  {
    HandleScope scope;
    DIBuilder* dib = ObjectWrap::Unwrap<DIBuilder>(args.This());
  
    REQ_UTF8_ARG(0, file);
    REQ_UTF8_ARG(1, dir);

    Handle<v8::Value> result = DIFile::New(dib->llvm_dibuilder->createFile(*file, *dir));
    return scope.Close(result);
  }

  llvm::DIType DIBuilder::CreateDIFunctionType(llvm::DIFile file, llvm::FunctionType *fty)
  {
    std::vector<llvm::Value*> args;
    args.push_back (ejsValueType);

#if false
    const ParameterList & params = type->params();
    for (ParameterList::const_iterator it = params.begin(); it != params.end(); ++it) {
      const ParameterDefn * param = *it;
      DIType ptype = CreateDIParameterType(param->type());
      args.push_back(ptype);
    }
#endif
    return llvm_dibuilder->createSubroutineType(file, llvm_dibuilder->getOrCreateArray(args));
  }

  Handle<v8::Value> DIBuilder::CreateFunction(const Arguments& args)
  {
    HandleScope scope;
    DIBuilder* dib = ObjectWrap::Unwrap<DIBuilder>(args.This());
  
    REQ_UTF8_ARG(0, name);
    REQ_UTF8_ARG(1, linkageName);
    REQ_LLVM_DIFILE_ARG(2, file);
    REQ_INT_ARG(3, line_no);
    REQ_BOOL_ARG(4, isLocalToUnit);
    REQ_BOOL_ARG(5, isDefinition);
    REQ_INT_ARG(6, scopeLine);
    REQ_INT_ARG(7, flags);
    REQ_BOOL_ARG(8, isOptimized);
    REQ_LLVM_FUN_ARG(9, fn);

    
    Handle<v8::Value> result = DISubprogram::New(dib->llvm_dibuilder->createFunction (file,
										      *name,
										      *linkageName,
										      file,
										      line_no,
										      dib->CreateDIFunctionType(file, fn->getFunctionType()),
										      isLocalToUnit,
										      isDefinition,
										      scopeLine,
										      flags,
										      isOptimized,
										      fn));
    return scope.Close(result);
  }

  Handle<v8::Value> DIBuilder::Finalize(const Arguments& args)
  {
    HandleScope scope;
    DIBuilder* dib = ObjectWrap::Unwrap<DIBuilder>(args.This());
  
    dib->llvm_dibuilder->finalize();
    return scope.Close(Undefined());
  }

  Persistent<FunctionTemplate> DIBuilder::s_ct;
  Persistent<v8::Function> DIBuilder::s_func;


  // DIDescriptor


  void DIDescriptor::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DIDescriptor"));

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DIDescriptor"),
		s_func);
  }

  v8::Handle<v8::Value> DIDescriptor::New(llvm::DIDescriptor llvm_didescriptor)
  {
    HandleScope scope;
    Local<Object> new_instance = DIDescriptor::s_func->NewInstance();
    DIDescriptor* new_didescriptor = new DIDescriptor(llvm_didescriptor);
    new_didescriptor->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DIDescriptor::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DIDescriptor::DIDescriptor(llvm::DIDescriptor llvm_didescriptor)
    : llvm_didescriptor(llvm_didescriptor)
  {
  }

  DIDescriptor::DIDescriptor()
  {
  }

  DIDescriptor::~DIDescriptor()
  {
  }

  Persistent<FunctionTemplate> DIDescriptor::s_ct;
  Persistent<v8::Function> DIDescriptor::s_func;




  // DIType


  void DIType::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DIType"));

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DIType"),
		s_func);
  }

  v8::Handle<v8::Value> DIType::New(llvm::DIType llvm_ditype)
  {
    HandleScope scope;
    Local<Object> new_instance = DIType::s_func->NewInstance();
    DIType* new_ditype = new DIType(llvm_ditype);
    new_ditype->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DIType::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DIType::DIType(llvm::DIType llvm_ditype)
    : llvm_ditype(llvm_ditype)
  {
  }

  DIType::DIType()
  {
  }

  DIType::~DIType()
  {
  }

  Persistent<FunctionTemplate> DIType::s_ct;
  Persistent<v8::Function> DIType::s_func;

  // DIScope


  void DIScope::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->Inherit(DIDescriptor::s_ct);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DIScope"));

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DIScope"),
		s_func);
  }

  v8::Handle<v8::Value> DIScope::New(llvm::DIScope llvm_discope)
  {
    HandleScope scope;
    Local<Object> new_instance = DIScope::s_func->NewInstance();
    DIScope* new_discope = new DIScope(llvm_discope);
    new_discope->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DIScope::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DIScope::DIScope(llvm::DIScope llvm_discope)
    : llvm_discope(llvm_discope)
  {
  }

  DIScope::DIScope()
  {
  }

  DIScope::~DIScope()
  {
  }

  Persistent<FunctionTemplate> DIScope::s_ct;
  Persistent<v8::Function> DIScope::s_func;


  // DISubprogram


  void DISubprogram::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->Inherit(DIScope::s_ct);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DISubprogram"));

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DISubprogram"),
		s_func);
  }

  v8::Handle<v8::Value> DISubprogram::New(llvm::DISubprogram llvm_disubprogram)
  {
    HandleScope scope;
    Local<Object> new_instance = DISubprogram::s_func->NewInstance();
    DISubprogram* new_disubprogram = new DISubprogram(llvm_disubprogram);
    new_disubprogram->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DISubprogram::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DISubprogram::DISubprogram(llvm::DISubprogram llvm_disubprogram)
    : llvm_disubprogram(llvm_disubprogram)
  {
  }

  DISubprogram::DISubprogram()
  {
  }

  DISubprogram::~DISubprogram()
  {
  }

  Persistent<FunctionTemplate> DISubprogram::s_ct;
  Persistent<v8::Function> DISubprogram::s_func;




  // DIFile


  void DIFile::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DIFile"));

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DIFile"),
		s_func);
  }

  v8::Handle<v8::Value> DIFile::New(llvm::DIFile llvm_difile)
  {
    HandleScope scope;
    Local<Object> new_instance = DIFile::s_func->NewInstance();
    DIFile* new_difile = new DIFile(llvm_difile);
    new_difile->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DIFile::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DIFile::DIFile(llvm::DIFile llvm_difile)
    : llvm_difile(llvm_difile)
  {
  }

  DIFile::DIFile()
  {
  }

  DIFile::~DIFile()
  {
  }

  Persistent<FunctionTemplate> DIFile::s_ct;
  Persistent<v8::Function> DIFile::s_func;



  // DILexicalBlock


  void DILexicalBlock::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->Inherit(DIScope::s_ct);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DILexicalBlock"));

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DILexicalBlock"),
		s_func);
  }

  v8::Handle<v8::Value> DILexicalBlock::New(llvm::DILexicalBlock llvm_dilexicalblock)
  {
    HandleScope scope;
    Local<Object> new_instance = DILexicalBlock::s_func->NewInstance();
    DILexicalBlock* new_dilexicalblock = new DILexicalBlock(llvm_dilexicalblock);
    new_dilexicalblock->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DILexicalBlock::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DILexicalBlock::DILexicalBlock(llvm::DILexicalBlock llvm_dilexicalblock)
    : llvm_dilexicalblock(llvm_dilexicalblock)
  {
  }

  DILexicalBlock::DILexicalBlock()
  {
  }

  DILexicalBlock::~DILexicalBlock()
  {
  }

  Persistent<FunctionTemplate> DILexicalBlock::s_ct;
  Persistent<v8::Function> DILexicalBlock::s_func;

  // DebugLoc


  void DebugLoc::Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("DebugLoc"));

    NODE_SET_METHOD(s_ct, "get", DebugLoc::Get);

    s_func = Persistent<v8::Function>::New(s_ct->GetFunction());
    target->Set(String::NewSymbol("DebugLoc"),
		s_func);
  }

  v8::Handle<v8::Value> DebugLoc::New(llvm::DebugLoc llvm_debugloc)
  {
    HandleScope scope;
    Local<Object> new_instance = DebugLoc::s_func->NewInstance();
    DebugLoc* new_debugloc = new DebugLoc(llvm_debugloc);
    new_debugloc->Wrap(new_instance);
    return scope.Close(new_instance);
  }

  Handle<v8::Value> DebugLoc::New(const Arguments& args)
  {
    HandleScope scope;
    return scope.Close(args.This());
  }

  DebugLoc::DebugLoc(llvm::DebugLoc llvm_debugloc)
    : llvm_debugloc(llvm_debugloc)
  {
  }

  DebugLoc::DebugLoc()
  {
  }

  DebugLoc::~DebugLoc()
  {
  }

  Handle<v8::Value> DebugLoc::Get(const Arguments& args)
  {
    HandleScope scope;

    REQ_INT_ARG(0, line);
    REQ_INT_ARG(1, column);
    REQ_LLVM_DISCOPE_ARG(2, discope);

    return scope.Close(DebugLoc::New(llvm::DebugLoc::get(line, column, discope, NULL)));
  }

  Persistent<FunctionTemplate> DebugLoc::s_ct;
  Persistent<v8::Function> DebugLoc::s_func;

};