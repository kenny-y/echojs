
#ifndef _ejs_ops_h
#define _ejs_ops_h

#include "ejs.h"
#include "ejs-object.h"

/* returns an EJSPrimString* */
EJSValue* NumberToString(double d);
EJSValue* ToString(EJSValue *exp);
double ToDouble(EJSValue *exp);
EJSValue* ToObject(EJSValue *exp);
EJSValue* ToBoolean(EJSValue *exp);

EJSValue* _ejs_op_neg (EJSValue* exp);
EJSValue* _ejs_op_not (EJSValue* exp);
EJSValue* _ejs_op_void (EJSValue* exp);
EJSValue* _ejs_op_typeof (EJSValue* exp);
EJSValue* _ejs_op_delete (EJSValue* obj, EJSValue* prop);
EJSValue* _ejs_op_bitwise_and (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_bitwise_or (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_rsh (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_ursh (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_mod (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_add (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_mult (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_lt (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_le (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_gt (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_ge (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_sub (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_strict_eq (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_strict_neq (EJSValue* lhs, EJSValue* rhs);

EJSValue* _ejs_op_eq (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_neq (EJSValue* lhs, EJSValue* rhs);

EJSValue* _ejs_op_instanceof (EJSValue* lhs, EJSValue* rhs);
EJSValue* _ejs_op_in (EJSValue* lhs, EJSValue* rhs);

EJSBool _ejs_truthy (EJSValue* val);

#endif // _ejs_ops_h