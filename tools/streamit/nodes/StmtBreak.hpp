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
#ifndef _STMTBREAK_HPP_
#define _STMTBREAK_HPP_

#include "Statement.hpp"

namespace streamit {

/**
 * A simple break statement.  This statement is used to exit the
 * innermost section of control flow, such as a for or while loop.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtBreak.java,v 1.4 2004/01/16 21:18:39 dmaze Exp $
 */
class StmtBreak : public Statement
{
public:
    /** Creates a new break statement. */
    StmtBreak(FEContext *context) : Statement(context)
    {
    }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitStmtBreak(this);
    }

//     public boolean equals(Object other)
//     {
//         // No state; any two break statements are equal.
//         if (other instanceof StmtBreak)
//             return true;
//         return false;
//     }

//     public int hashCode()
//     {
//         // No state, so...
//         return 17;
//     }
    
    string toString()
    {
        return string("break");
    }
};

}

#endif

