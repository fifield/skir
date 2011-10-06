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
#ifndef _EXPRCONSTANT_HPP_
#define _EXPRCONSTANT_HPP_

#include "Expression.hpp"
#include "ExprConstInt.hpp"
#include "ExprConstFloat.hpp"

namespace streamit {

/**
 * A constant-valued expression.  This class only serves to hold
 * static methods for creating constants.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprConstant.java,v 1.4 2006/01/25 17:04:25 thies Exp $
 */
class ExprConstant : public Expression
{
public:    
    /**
     * Create a new constant-valued expression corresponding to a
     * String value.  val must be a valid real number, according to
     * java.lang.Double.valueOf(), excepting that it may end in "i" to
     * indicate an imaginary value.  This attempts to create an
     * integer if possible, and a real-valued expression if possible;
     * however, it may also create an ExprComplex with a zero (null)
     * real part.  Eexpressions like "3+4i" need to be parsed into
     * separate expressions. 
     *
     * @param context  file and line number for the string
     * @param val      string containing the constant to create
     * @return         an expression corresponding to the string value
     */
    static Expression *createConstant(FEContext *context, string val)
    {
        // Either val ends in "i", or it doesn't.
        if (val[val.size()-1] == 'i') {
	    val = string(val.begin(), val.end()-1);
	    return new ExprComplex(context, NULL,
				   createConstant(context, val));
	}

        // Maybe it's integral.
	if (val.find('.') == string::npos)
	    return new ExprConstInt(context, val);
	else
	    // No; create a float (and lose if this is wrong too).
	    return new ExprConstFloat(context, val);
    }
};

}

#endif
