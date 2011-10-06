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
#ifndef _STMTBLOCK_HPP_
#define _STMTBLOCK_HPP_

#include "Statement.hpp"

namespace streamit {

/**
 * A block of statements executed in sequence.  This introduces a
 * lexical scope for variable declarations, and is a way for multiple
 * statements to be used in loops or conditionals.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtBlock.java,v 1.3 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtBlock : public Statement
{
private:
    StatementList *stmts;

    // Should this also have a symbol table?  --dzm
    
public:
    /** Create a new StmtBlock with the specified ordered list of
     * statements. */
    StmtBlock(FEContext *context, StatementList *stmts) : Statement(context)
    {
        this->stmts = stmts;
    }
    
    /** Returns the list of statements of this. */
    StatementList *getStmts()
    {
        return stmts;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtBlock(this);
    }
};

}

#endif

