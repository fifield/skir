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
#ifndef _EXPRCONSTINT_HPP_
#define _EXPRCONSTINT_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * An integer-valued constant.  This can be freely promoted to an
 * ExprConstFloat.  This is always real-valued, though it can appear
 * in an ExprComplex to form a complex integer expression.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprConstInt.java,v 1.4 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprConstInt : public Expression
{
private:
    int val;
    
public:
    /** Create a new ExprConstInt with a specified value. */
    ExprConstInt(FEContext *context, int val) : Expression(context), val(val)
    {
    }

    /** Create a new ExprConstInt with a specified value but no context. */
    ExprConstInt(int val) : Expression(NULL), val(val)
    {
    }
    
    /** Parse a string as an integer, and create a new ExprConstInt
     * from the result. */
    ExprConstInt(FEContext *context, string str) : Expression(context)
    {
	std::stringstream ss;
	if (str.find("0x") != string::npos) {
	    ss << std::hex << str;
	} else {
	    ss << str;
	}
	unsigned int v;
	ss >> v;
	val = (int)v;
    }
    
    /** Returns the value of this. */
    int getVal() { return val; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprConstInt(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof ExprConstInt))
//             return false;
//         return val == ((ExprConstInt)other).getVal();
//     }
    
//     public int hashCode()
//     {
//         return new Integer(val).hashCode();
//     }
    
    string toString()
    {
	std::stringstream ss;
	ss << val;
	return ss.str();
    }
};

}

#endif
