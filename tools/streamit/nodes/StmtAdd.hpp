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
#ifndef _STMTADD_HPP_
#define _STMTADD_HPP_

#include "Statement.hpp"
#include "StreamCreator.hpp"

namespace streamit {

/**
 * Add a child stream to a pipeline or split-join.  This statement has
 * a single {@link streamit.frontend.nodes.StreamCreator} object that
 * specifies what child is being added.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtAdd.java,v 1.3 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtAdd : public  Statement
{
public:
    StreamCreator *sc;
    
    /** Creates a new add statement for a specified child. */
    StmtAdd(FEContext *context, StreamCreator *sc) : Statement(context)
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
        return v->visitStmtAdd(this);
    }
};

}

#endif
