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
#ifndef _STMTSPLIT_HPP_
#define _STMTSPLIT_HPP_

#include "Statement.hpp"
#include "SplitterJoiner.hpp"

namespace streamit {

/**
 * Declare the splitter type for a split-join or feedback loop.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtSplit.java,v 1.5 2003/10/09 19:51:00 dmaze Exp $
 */
class StmtSplit : public Statement
{
private:
    SplitterJoiner *sj;

public:    
    /**
     * Creates a new split statement with the specified splitter type.
     *
     * @param context  file and line number this statement corresponds to
     * @param splitter type of splitter for this stream
     */
    StmtSplit(FEContext *context, SplitterJoiner *splitter) : Statement(context), sj(splitter)
    {
    }
    
    /**
     * Returns the splitter type for this.
     *
     * @return the splitter object
     */
    SplitterJoiner *getSplitter()
    {
        return sj;
    }
    
    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtSplit(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof StmtSplit))
//             return false;
//         return ((StmtSplit)other).sj.equals(sj);
//     }
    
//     public int hashCode()
//     {
//         return sj.hashCode();
//     }
    
//     public String toString()
//     {
//         return "split " + sj;
//     }
};

}

#endif
