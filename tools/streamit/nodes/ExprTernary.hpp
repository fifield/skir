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
#ifndef _EXPRTERNARY_HPP_
#define _EXPRTERNARY_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A ternary expression; that is, one with three children.  C and Java
 * have exactly one of these, which is the conditional expression
 * <code>(a ? b : c)</code>, which is an expression equivalent of "if
 * (a) b else c".
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprTernary.java,v 1.3 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprTernary : public Expression
{
public:
    // Operators: (for consistency, really)
    enum { TEROP_COND = 1 };

private:			   
    int op;
    Expression *a, *b, *c;
    
public:
    /** Creates a new ExprTernary with the specified operation and
     * child expressions. */
    ExprTernary(FEContext *context,
		int op, Expression *a, Expression *b, Expression *c) : Expression(context)
    {
        this->op = op;
        this->a = a;
        this->b = b;
        this->c = c;
    }

    /** Returns the operation of this. */
    int getOp() { return op; }

    /** Returns the first child of this. */
    Expression *getA() { return a; }

    /** Returns the second child of this. */
    Expression *getB() { return b; }

    /** Returns the third child of this. */
    Expression *getC() { return c; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprTernary(this);
    }
};

}

#endif
