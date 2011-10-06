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
#ifndef _EXPRCONSTSTR_HPP_
#define _EXPRCONSTSTR_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A string literal.  The only place these currently appear in StreamIt
 * is as the filename argument to file reader and writer filters.
 * For convenience, these are stored in their original program-source
 * representation.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprConstStr.java,v 1.4 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprConstStr : public Expression
{
private:
    string val;
    
public:
    /**
     * Create a new ExprConstStr.
     *
     * @param context  file and line number in the source file
     * @param val      source-format representation of the string
     */
    ExprConstStr(FEContext *context, string val) : Expression(context), val(val)
    {
    }
    
    /**
     * Returns the value of this.  The returned string will be in
     * the format it appeared in in the original source file,
     * including leading and trailing double quotes.
     *
     * @return  source-format representation of the string
     */
    string getVal() { return val; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprConstStr(this);
    }

    string toString() { return val; }
};

}

#endif

