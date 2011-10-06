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
#ifndef _STMTHELPERCALL_HPP_
#define _STMTHELPERCALL_HPP_

#include "Statement.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * A statement that causes a helper call to be executed.
 *
 * @author  Janis Sermulins
 */

class StmtHelperCall : public Statement
{
 private:
    string helper_package;
    string name;
    ExpressionList *params;
 public:
    /**
     * Creates a helper call statement.
     *
     */
    StmtHelperCall(FEContext *context, string helper_package, string name, ExpressionList *params)
	: Statement(context), helper_package(helper_package), name(name), params(params)
							  
    {
    }

    /**
     * Get the helper package.
     */
    string getHelperPackage()
    {
        return helper_package;
    }

    /**
     * Get the name of the helper function.
     */
    string getName()
    {
        return name;
    }
    
    /**
     * Get the parameter list of the helper call.
     */
    ExpressionList *getParams()
    {
        return params;
    }
    
    /**
     * Accepts a front-end visitor.  Calls
     * <code>streamit.frontend.nodes.FEVisitor.visitStmtHelperCall</code>
     * on the visitor.
     *
     * @param v  visitor to accept
     * @return   defined by the visitor object
     */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtHelperCall(this);
    }
};

}

#endif
