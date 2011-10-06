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
#ifndef _EXPRTYPECAST_HPP_
#define _EXPRTYPECAST_HPP_

#include "Expression.hpp"
#include "Type.hpp"

namespace streamit {

/**
 * An expression directing one expression to be interpreted as a different
 * (primitive) type.  This has a child instruction and the type that is
 * being cast to.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprTypeCast.java,v 1.3 2004/01/16 20:28:24 dmaze Exp $
 */
class ExprTypeCast : public Expression
{
private:
    Type *type;
    Expression *expr;
    
public:
    /**
     * Create a new ExprTypeCast with a specified type and child
     * expression.
     *
     * @param context  Context indicating file and line number
     *                 this expression was created in
     * @param type     Type the expression is being cast to
     * @param expr     Expression being cast
     */
    ExprTypeCast(FEContext *context, Type *type, Expression *expr) : Expression(context)
    {
        this->type = type;
        this->expr = expr;
    }
    
    /**
     * Get the type the expression is being cast to.
     *
     * @return  Type the expression is cast to
     */
    Type *getType()
    {
        return type;
    }
    
    /**
     * Get the expression being cast.
     *
     * @return  The expression being cast
     */
    Expression *getExpr()
    {
        return expr;
    }
    
    /**
     * Accept a front-end visitor.
     */
    void *accept(FEVisitor *v)
    {
        return v->visitExprTypeCast(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof ExprTypeCast))
//             return false;
//         ExprTypeCast etc = (ExprTypeCast)other;
//         if (!(type.equals(etc.type)))
//             return false;
//         if (!(expr.equals(etc.expr)))
//             return false;
//         return true;
//     }
    
//     public int hashCode()
//     {
//         return type.hashCode() ^ expr.hashCode();
//     }
    
    string toString()
    {
	std::stringstream ss;
	ss << "((" << type << ")" << expr << ")";
	return ss.str();
    }
};

} //namespace

#endif
