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
#ifndef _EXPRCONSTCHAR_HPP_
#define _EXPRCONSTCHAR_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A single-character literal, as appears inside single quotes in
 * Java.  This generally doesn't actually appear in StreamIt code.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprConstChar.java,v 1.3 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprConstChar : public Expression
{
private:
    char val;

public:
    /** Create a new ExprConstChar for a particular character. */
    ExprConstChar(FEContext *context, char val) : Expression(context), val(val)
    {
    }
    
    /** Create a new ExprConstChar containing the first character of a
     * String. */
    ExprConstChar(FEContext *context, string str) : Expression(context)
    {
	const char *c = str.c_str();
	val = c[0];
    }
    
    /** Returns the value of this. */
    char getVal() { return val; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprConstChar(this);
    }

    string toString() { return string(&val); }

};

}
#endif
