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
#ifndef _STMTWHILE_HPP_
#define _STMTWHILE_HPP_

#include "Statement.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * A standard while loop.  The loop has a condition and a loop body.
 * On entry, the condition is evaluated; if it is true, the body is
 * executed, and the condition evaluated again.  This repeats until
 * the condition is false.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtWhile.java,v 1.3 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtWhile : public Statement
{
private:
    Expression *cond;
    Statement *body;
    
public:
    /** Creates a new while loop. */
    StmtWhile(FEContext *context, Expression *cond, Statement *body)
	: Statement(context), cond(cond), body(body)
    {
    }
    
    /** Returns the loop condition. */
    Expression *getCond()
    {
        return cond;
    }
    
    /** Returns the loop body. */
    Statement *getBody()
    {
        return body;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtWhile(this);
    }
};

}

#endif
