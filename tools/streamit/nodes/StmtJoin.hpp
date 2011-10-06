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
#ifndef _STMTJOIN_HPP_
#define _STMTJOIN_HPP_

#include "Statement.hpp"
#include "SplitterJoiner.hpp"

namespace streamit {

/**
 * Declare the joiner type for a split-join or feedback loop.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtJoin.java,v 1.7 2006/03/27 21:42:55 dimock Exp $
 */
class StmtJoin : public Statement
{
private:
    SplitterJoiner *sj;
    
public:
    /**
     * Creates a new join statement with the specified joiner type.
     *
     * @param context  file and line number this statement corresponds to
     * @param joiner type of splitter for this stream
     */
    StmtJoin(FEContext *context, SplitterJoiner *joiner) : Statement(context), sj(joiner)
    {
    }
    
    /**
     * Returns the joiner type for this.
     *
     * @return the joiner object
     */
    SplitterJoiner *getJoiner()
    {
        return sj;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtJoin(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof StmtJoin))
//             return false;
//         return ((StmtJoin)other).sj.equals(sj);
//     }
    
//     public int hashCode()
//     {
//         return sj.hashCode();
//     }
    
    string toString()
    {
        return "join " + sj->toString();
    }
};

}

#endif
