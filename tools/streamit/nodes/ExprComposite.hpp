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
#ifndef _EXPRCOMPOSITE_HPP_
#define _EXPRCOMPOSITE_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A composite valued expression. Such as float2, float3, float4
 *
 * @author  Janis Sermulins &lt;janiss@mit.edu&gt;
 */
class ExprComposite : public Expression
{
private:
    Expression *x, *y, *z, *w;
    
public:
    /**
     * Create a new ExprComposite with the specified parts.
     * Either of parts may be null, to denote float2 or float3.
     *
     * @param context  file and line number this expression corresponds to
     */
    ExprComposite(FEContext *context, Expression *x, Expression *y,
		  Expression *z, Expression *w) : Expression(context)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
    
    Expression *getX() { return x; }  

    Expression *getY() { return y; }  

    Expression *getZ() { return z; }  

    Expression *getW() { return w; }  

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
	return v->visitExprComposite(this);
    }

    int getDim() {
        int dim = 2;
        if (z != NULL) dim++;
        if (w != NULL) dim++;
        return dim;
    }

    string toString()
    {
	std::stringstream ss;
	ss << "[" << x << "," << y;
        if (z != NULL) ss << "," << z;
        if (w != NULL) ss << "," << w;
	ss << "]";
        return ss.str();
    }
    
//     public boolean equals(Object other)
//     {
//         if (!(other instanceof ExprComposite))
//             return false;
//         ExprComposite that = (ExprComposite)other;

//         if (!(this.x.equals(that.x)))
//             return false;
//         if (!(this.y.equals(that.y)))
//             return false;

//         if (this.z != null && !(this.z.equals(that.z)))
//             return false;
//         if (this.z == null && that.z != null) 
//             return false;

//         if (this.w != null && !(this.w.equals(that.w)))
//             return false;
//         if (this.w == null && that.w != null) 
//             return false;

//         return true;
//     }
    
//     public int hashCode()
//     {
//         return x.hashCode() ^ y.hashCode();
//     }
};

}

#endif
