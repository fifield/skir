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
#ifndef _STMTPUSH_HPP_
#define _STMTPUSH_HPP_

#include "Statement.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * Push a single value on to the current filter's output tape.  This
 * statement has an expression, which is the value to be pushed.  The
 * type of the expression must match the output type of the filter
 * exactly.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtPush.java,v 1.4 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtPush : public Statement
{
private:
    Expression *value;
    
public:
    /** Creates a new push statement with the specified value. */
    StmtPush(FEContext *context, Expression *value) : Statement(context)
    {
        this->value = value;
    }

    /** Returns the value this pushes. */
    Expression *getValue()
    {
        return value;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtPush(this);
    }
    
//     public boolean equals(Object other)
//     {
//         if (!(other instanceof StmtPush))
//             return false;
//         return value.equals(((StmtPush)other).getValue());
//     }
    
//     public int hashCode()
//     {
//         return value.hashCode();
//     }

    string toString()
    {
        return "push(" + value->toString() + ")";
    }
};

}

#endif
