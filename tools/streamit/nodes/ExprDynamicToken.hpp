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
#ifndef _EXPRDYNAMICTOKEN_HPP_
#define _EXPRDYNAMICTOKEN_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * Indicates the '*' token as used in a dynamic rate declaration.  Its
 * type is an integer.
 *
 */
class ExprDynamicToken : public Expression
{
public:
    /** Create a new ExprDynamicToken. */
    ExprDynamicToken(FEContext *context) : Expression(context)
    {
    }

    /** Create a new ExprDynamicToken with no context. */
    ExprDynamicToken() : Expression(NULL)
    {
    }
    
    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprDynamicToken(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof ExprDynamicToken))
//             return false;
//         return true;
//     }
    
//     public int hashCode()
//     {
//         // following the pattern in the integer constants -- constants
//         // of the same value have the same hashcode
//         return new Character('*').hashCode();
//     }
    
    string toString()
    {
        return string("*");
    }
};

}

#endif
