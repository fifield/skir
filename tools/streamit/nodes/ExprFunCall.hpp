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
#ifndef _EXPRFUNCALL_HPP_
#define _EXPRFUNCALL_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A call to a particular named function.  This contains the name of
 * the function and a <code>java.util.List</code> of parameters.  Like
 * other <code>Expression</code>s, this is immutable; an unmodifiable
 * copy of the passed-in <code>List</code> is saved.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprFunCall.java,v 1.5 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprFunCall : public Expression
{
private:
    string name;
    ExpressionList *params;
    
public:
    /** Creates a new function call with the specified name and
     * parameter list. */
    ExprFunCall(FEContext *context, string name, ExpressionList *params) : Expression(context)
    {
        this->name = name;
        this->params = params;
    }

    /** Creates a new function call with the specified name and
     * specified single parameter. */
    ExprFunCall(FEContext *context, string name, Expression *param) : Expression(context)
    {
        this->name = name;
        this->params = new ExpressionList;
        this->params->push_back(param);
    }

    /** Creates a new function call with the specified name and
     * two specified parameters. */
    ExprFunCall(FEContext *context, string name,
		Expression *p1, Expression *p2) : Expression(context)
    {
        this->name = name;
        this->params = new ExpressionList;
        this->params->push_back(p1);
        this->params->push_back(p2);
    }

    /** Returns the name of the function being called. */
    string getName()
    {
        return name;
    }
    
    /** Returns the parameters of the function call, as an unmodifiable
     * list. */
    ExpressionList *getParams()
    {
        return params;
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprFunCall(this);
    }
};

}

#endif
