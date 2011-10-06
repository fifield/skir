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

#ifndef _EXPRBINARY_HPP_
#define _EXPRBINARY_HPP_

#include "Expression.hpp"

namespace streamit {

/**
 * A binary expression.  This has two child expressions, which must be
 * non-null, and an operator.  The child expressions are ordered
 * (because a-b is different from b-a).
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: ExprBinary.java,v 1.7 2006/01/25 17:04:25 thies Exp $
 */
class ExprBinary : public Expression
{
public:
    // Operators:
    enum {
	BINOP_ADD = 1,
	BINOP_SUB = 2,
	BINOP_MUL = 3,
	BINOP_DIV = 4,
	BINOP_MOD = 5,
	BINOP_AND = 6,
	BINOP_OR = 7,
	BINOP_EQ = 8,
	BINOP_NEQ = 9,
	BINOP_LT = 10,
	BINOP_LE = 11,
	BINOP_GT = 12,
	BINOP_GE = 13,
	// These are bitwise AND/OR/XOR:
	BINOP_BAND = 14,
	BINOP_BOR = 15,
	BINOP_BXOR = 16,
	BINOP_LSHIFT = 17,
	BINOP_RSHIFT = 18
    };

private:
    int op;
    Expression *left, *right;

public:    
    /**
     * Create a new binary expression given the operation and the
     * left and right child nodes.  Requires that op is a valid
     * operator code and that left and right are non-null. 
     *
     * @param context  file and line number this expression corresponds to
     * @param op       BINOP_ operator combining the two expressions
     * @param left     expression on the left of the operator
     * @param right    expression on the right of the operator
     */
    ExprBinary(FEContext *context,
	       int op, Expression *left, Expression *right) : Expression(context)
    {
        this->op = op;
        this->left = left;
        this->right = right;
    }

    /**
     * Returns the operator of this.
     *
     * @return BINOP_ operator code for this expression
     */
    int getOp() { return op; }   

    /**
     * Returns the left child expression of this.
     *
     * @return expression on the left-hand side of the operator
     */
    Expression *getLeft() { return left; }

    /**
     * Returns the right child expression of this.
     *
     * @return expression on the right-hand side of the operator
     */
    Expression *getRight() { return right; }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitExprBinary(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof ExprBinary))
//             return false;
//         ExprBinary eb = (ExprBinary)other;
//         if (!(left.equals(eb.getLeft())))
//             return false;
//         if (!(right.equals(eb.getRight())))
//             return false;
//         if (op != eb.getOp())
//             return false;
//         return true;
//     }
    
//     public int hashCode()
//     {
//         return left.hashCode() ^ right.hashCode() ^ new Integer(op).hashCode();
//     }

    string toString()
    {
        string theOp;
        switch (op)
            {
            case ExprBinary::BINOP_ADD: theOp = "+"; break;
            case ExprBinary::BINOP_SUB: theOp = "-"; break;
            case ExprBinary::BINOP_MUL: theOp = "*"; break;
            case ExprBinary::BINOP_DIV: theOp = "/"; break;
            case ExprBinary::BINOP_AND: theOp = "&&"; break;
            case ExprBinary::BINOP_OR: theOp = "||"; break;
            case ExprBinary::BINOP_EQ: theOp = "=="; break;
            case ExprBinary::BINOP_NEQ: theOp = "!="; break;
            case ExprBinary::BINOP_LT: theOp = "<"; break;
            case ExprBinary::BINOP_LE: theOp = "<="; break;
            case ExprBinary::BINOP_GT: theOp = ">"; break;
            case ExprBinary::BINOP_GE: theOp = ">="; break;
            case ExprBinary::BINOP_BAND: theOp = "&"; break;
            case ExprBinary::BINOP_BOR: theOp = "|"; break;
            case ExprBinary::BINOP_BXOR: theOp = "^"; break;
            case ExprBinary::BINOP_LSHIFT: theOp = "<<"; break;
            case ExprBinary::BINOP_RSHIFT: theOp = ">>"; break;
            default: 
		std::stringstream ss;
		ss << op;
		theOp = "? (" + ss.str() + ")";
		break;
             }
         return "(" + left->toString() + ")" + theOp +
             "(" + right->toString() + ")";
     }
};

} // namespace

#endif
