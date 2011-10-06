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
#ifndef _STMTEMPTY_HPP_
#define _STMTEMPTY_HPP_

#include "Statement.hpp"

namespace streamit {

/**
 * An empty statement.  This does nothing; it is mostly useful as a
 * placeholder in things like for loops.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtEmpty.java,v 1.1 2003/12/01 21:45:26 dmaze Exp $
 */
class StmtEmpty : public Statement
{
public:
    /** 
     * Creates a new empty statement.
     *
     * @param context   File and line number the statement corresponds to
     */
    StmtEmpty(FEContext *context) : Statement(context)
    {
    }
    
    void *accept(FEVisitor *v)
    {
	return v->visitStmtEmpty(this);
    }
};

}

#endif
