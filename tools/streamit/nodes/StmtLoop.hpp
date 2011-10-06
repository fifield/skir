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
#ifndef _STMTLOOP_HPP_
#define _STMTLOOP_HPP_

#include "Statement.hpp"

namespace streamit {

/**
 * Add the loop stream to a feedback loop.  This statement has a
 * single {@link streamit.frontend.nodes.StreamCreator} object that
 * specifies what child is being added.
 * 
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtLoop.java,v 1.3 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtLoop : public Statement
{
private:
    StreamCreator *sc;

public:
    /** Creates a new loop statement for a specified child. */
    StmtLoop(FEContext *context, StreamCreator *sc) : Statement(context)
    {
        this->sc = sc;
    }
    
    /** Returns the child stream creator. */
    StreamCreator *getCreator()
    {
        return sc;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
	return v->visitStmtLoop(this);
    }
};

}    

#endif
