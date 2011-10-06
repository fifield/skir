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
#ifndef _EXPRRANGE_HPP_
#define _EXPRRANGE_HPP_

#include "Expression.hpp"
#include "ExprDynamicToken.hpp"

namespace streamit {

/**
 * Indicates a range such as [min,ave,max] as used in a dynamic rate.
 * Ranges are over integers.
 *
 */
class ExprRange : public Expression
{
private:
    Expression *min,*ave,*max;

public:
    /** Create a new ExprDynamicToken. */
    ExprRange(FEContext *context, Expression *min, Expression *ave, Expression *max)
	: Expression(context)
    {
        this->min = min;
        // a null average turns into a dynamic average
        if (ave==NULL) {
            this->ave = new ExprDynamicToken(context);
        } else {
            this->ave = ave;
        }
        this->max = max;
    }
    
    ExprRange(FEContext *context, Expression *min, Expression *max) 
	: Expression(context)
    {
        //this(context, min, null, max);
	this->min = min;
	this->ave = new ExprDynamicToken(context);
	this->max = max;
    }

    /** Create a new ExprRange with no context. */
    ExprRange(Expression *min, Expression *ave, Expression *max)
	: Expression(NULL)
    {
	//this(null, min, ave, max);
        this->min = min;
	this->ave = ave;
	this->max = max;
    }

    /** Create a new ExprRange with no context. */
    ExprRange(Expression *min, Expression *max) 
	: Expression(NULL)
    {
        //this((FEContext)null, min, max);
        this->min = min;
	this->ave = new ExprDynamicToken(NULL);
	this->max = max;
    }
    
    /** Return minimum of range. */
    Expression *getMin() {
        return min;
    }

    /** Return average of range. */
    Expression *getAve() {
        return ave;
    }

    /** Return maximum of range. */
    Expression *getMax() {
        return max;
    }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprRange(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof ExprRange))
//             return false;
//         ExprRange range = (ExprRange)other;
//         if (!(min.equals(range.min)))
//             return false;
//         if (!(ave.equals(range.ave)))
//             return false;
//         if (!(max.equals(range.max)))
//             return false;
//         return true;
//     }
    
//     public int hashCode()
//     {
//         // following the pattern in the integer constants -- constants
//         // of the same value have the same hashcode
//         return min.hashCode() * ave.hashCode() * max.hashCode();
//     }
    
    string toString()
    {
	std::stringstream ss;
        ss << "[" << min << "," << ave << "," << max << "]";
	return ss.str();
    }
};

}

#endif
