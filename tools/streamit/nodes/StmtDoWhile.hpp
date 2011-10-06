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
#ifndef _STMTDOWHILE_HPP_
#define _STMTDOWHILE_HPP_

#include "Statement.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * A standard do-while loop.  This statement has a loop body and a
 * condition.  On entry, the body is executed, and then the condition
 * is evaluated; if it is true, the body is executed again.  This continues
 * until the condition evaluates to false.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtDoWhile.java,v 1.3 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtDoWhile : public Statement
{
private:
    Statement *body;
    Expression *cond;

public:
    /** Creates a new while loop. */
    StmtDoWhile(FEContext *context, Statement *body, Expression *cond)
	: Statement(context), body(body), cond(cond)
    {
    }
    
    /** Returns the loop body. */
    Statement *getBody()
    {
        return body;
    }
    
    /** Returns the loop condition. */
    Expression *getCond()
    {
        return cond;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtDoWhile(this);
    }
};

}

#endif
