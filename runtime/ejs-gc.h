
#ifndef _ejs_gc_h_
#define _ejs_gc_h_

#include "ejs.h"

typedef struct GCObjectHeader* GCObjectPtr;

typedef struct GCObjectHeader {
  EJSValueTag tag;

  GCObjectPtr prev_link;
  GCObjectPtr next_link;
} GCObjectHeader;

extern void _ejs_gc_init();
extern void _ejs_gc_shutdown();
extern void _ejs_gc_collect();
extern GCObjectPtr _ejs_gc_alloc(size_t size);

#define _ejs_gc_new(T) (T*)_ejs_gc_alloc(sizeof(T))

#define _ejs_gc_add_named_root(v) __ejs_gc_add_named_root(&v, #v)

extern void __ejs_gc_add_named_root(EJSValue** val, const char *name);

extern void _ejs_gc_remove_root(EJSValue** root);

typedef struct FrameMap {
  int32_t NumRoots;    //< Number of roots in stack frame.
  int32_t NumMeta;     //< Number of metadata entries.  May be < NumRoots.
  const void *Meta[0]; //< Metadata for each root.
} FrameMap;

typedef struct StackEntry {
  struct StackEntry *Next;    //< Link to next stack entry (the caller's).
  FrameMap *Map; //< Pointer to constant FrameMap.
  void *Roots[0];      //< Stack roots (in-place array).
} StackEntry;

extern StackEntry *llvm_gc_root_chain;

#define NUM_NATIVE_ROOTS 50

typedef struct {
  struct StackEntry *Next;       //< Link to next stack entry (the caller's).
  struct FrameMap *Map;    //< Pointer to constant FrameMap.
  void *Roots[NUM_NATIVE_ROOTS]; //< Stack roots (in-place array).
} NativeStackEntry;


#define START_SHADOW_STACK_FRAME					\
  FrameMap fmap;							\
  fmap.NumRoots = fmap.NumMeta = 0;					\
  NativeStackEntry stack;						\
  memset (stack.Roots, 0, sizeof(void*) * NUM_NATIVE_ROOTS);		\
  stack.Map = &fmap;							\
  stack.Next = llvm_gc_root_chain;					\
  llvm_gc_root_chain = (StackEntry*)&stack;

#define ADD_STACK_ROOT2(name)			    \
  if (stack.Map->NumRoots == NUM_NATIVE_ROOTS-1) {  \
    printf ("out of native stack root entries\n");  \
    abort();					    \
  }						    \
  stack.Roots[stack.Map->NumRoots++] = name

#define ADD_STACK_ROOT(t, name, init)		    \
  t name = init;				    \
  ADD_STACK_ROOT2(name)

#define END_SHADOW_STACK_FRAME			\
  llvm_gc_root_chain = llvm_gc_root_chain->Next

#endif /* _ejs_gc_h */