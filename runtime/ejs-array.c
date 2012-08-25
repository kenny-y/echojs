#include <assert.h>

#include "ejs-value.h"
#include "ejs-array.h"

EJSValue* _ejs_Array;
static EJSValue*
_ejs_Array_impl (EJSValue* env, EJSValue* _this, int argc, EJSValue **args)
{
  if (EJSVAL_IS_UNDEFINED(_this)) {
    // called as a function
    printf ("called Array() as a function!\n");
    return NULL;
  }
  else {
    // called as a constructor
    printf ("called Array() as a constructor!\n");
    return NULL;
  }
}

#define ARRAY_LEN(obj) (obj->u.a.array_length)
#define ARRAY_ELEMENTS(obj) (obj->u.a.elements)

static EJSValue*
_ejs_Array_prototype_push (EJSValue* env, EJSValue* _this, int argc, EJSValue **args)
{
  int i;
  assert (EJSVAL_IS_ARRAY(_this));
  for (i = 0; i < argc; i ++)
    ARRAY_ELEMENTS(_this)[ARRAY_LEN(_this)++] = args[i];
  return _ejs_number_new (ARRAY_LEN(_this));
}

static EJSValue*
_ejs_Array_prototype_pop (EJSValue* env, EJSValue* _this, int argc, EJSValue **args)
{
  assert (EJSVAL_IS_ARRAY(_this));
  if (ARRAY_LEN(_this) == 0)
    return _ejs_undefined;
  return ARRAY_ELEMENTS(_this)[--ARRAY_LEN(_this)];
}

static EJSValue*
_ejs_Array_prototype_slice (EJSValue* env, EJSValue* _this, int argc, EJSValue **args)
{
  abort();
}

static EJSValue*
_ejs_Array_prototype_splice (EJSValue* env, EJSValue* _this, int argc, EJSValue **args)
{
  abort();
}

static EJSValue* _ejs_Array_proto;
EJSValue*
_ejs_array_get_prototype()
{
  return _ejs_Array_proto;
}

void
_ejs_array_init(EJSValue *global)
{
  _ejs_Array = _ejs_closure_new (NULL, (EJSClosureFunc)_ejs_Array_impl);
  _ejs_Array_proto = _ejs_object_new(NULL);

  _ejs_object_setprop_utf8 (_ejs_Array,       "prototype",  _ejs_Array_proto);
  _ejs_object_setprop_utf8 (_ejs_Array_proto, "prototype",  _ejs_object_get_prototype());
  _ejs_object_setprop_utf8 (_ejs_Array_proto, "push",       _ejs_closure_new (NULL, (EJSClosureFunc)_ejs_Array_prototype_push));
  _ejs_object_setprop_utf8 (_ejs_Array_proto, "pop",        _ejs_closure_new (NULL, (EJSClosureFunc)_ejs_Array_prototype_pop));
  _ejs_object_setprop_utf8 (_ejs_Array_proto, "slice",      _ejs_closure_new (NULL, (EJSClosureFunc)_ejs_Array_prototype_slice));
  _ejs_object_setprop_utf8 (_ejs_Array_proto, "splice",     _ejs_closure_new (NULL, (EJSClosureFunc)_ejs_Array_prototype_splice));

  _ejs_object_setprop (global, _ejs_string_new_utf8("Array"), _ejs_Array);
}
