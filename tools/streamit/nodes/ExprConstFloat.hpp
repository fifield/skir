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
#ifndef _EXPRCONSTFLOAT_HPP_
#define _EXPRCONSTFLOAT_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A real-valued constant.  This can appear in an ExprComplex to form
 * a complex real expression.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprConstFloat.java,v 1.3 2003/10/09 19:50:59 dmaze Exp $
 */
class ExprConstFloat : public Expression
{
private:
    double val;
    
public:
    /**
     * Create a new ExprConstFloat with a specified value.
     *
     * @param context  file and line number for the expression
     * @param val      value of the constant
     */
    ExprConstFloat(FEContext *context, double val) : Expression(context), val(val)
    {
    }

    /**
     * Create a new ExprConstFloat with a specified value but no context.
     *
     * @param val  value of the constant
     */
    ExprConstFloat(double val) : Expression(NULL), val(val)
    {
    }
    
    /**
     * Parse a string as a double, and create a new ExprConstFloat
     * from the result.
     *
     * @param context  file and line number for the expression
     * @param str      string representing the value of the constant
     */
    ExprConstFloat(FEContext *context, string str) : Expression(context)
    {
	std::istringstream ss(str);
	ss >> val;
    }
    
    /**
     * Returns the value of this.
     *
     * @return float value of the constant
     */
    double getVal() { return val; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprConstFloat(this);
    }

    string toString()
    {
	std::stringstream ss;
	ss << val;
	return ss.str();
    }
};

}

#endif
