/*
 * Copyright 2003 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
#ifndef _EXRHELPERCALL_HPP_
#define _EXRHELPERCALL_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A call to a particular helper function.  
 *
 * @author  Janis Sermulins
 */

class ExprHelperCall : public Expression
{
private:
    string helper_package;
    string name;
    ExpressionList *params;
    
public:
    /** Creates a new helper call with the specified name and
     * parameter list. */
    ExprHelperCall(FEContext *context, string helper_package, string name, ExpressionList *params)
	: Expression(context), helper_package(helper_package), name(name), params(params)
    {
    }

    /** Returns the name of the helper package. */
    string getHelperPackage()
    {
        return helper_package;
    }
   
    /** Returns the name of the function being called. */
    string getName()
    {
        return name;
    }
    
    /** Returns the parameters of the helper call, as an unmodifiable
     * list. */
    ExpressionList *getParams()
    {
        return params;
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprHelperCall(this);
    }
};

}

#endif
