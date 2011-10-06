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
#ifndef _STMTVARDECL_HPP_
#define _STMTVARDECL_HPP_

#include "Statement.hpp"
#include "Expression.hpp"

namespace streamit {

/**
 * A variable-declaration statement.  This statement declares a
 * sequence of variables, each of which has a name, a type, and an
 * optional initialization value.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StmtVarDecl.java,v 1.7 2006/01/25 17:04:25 thies Exp $
 */
class StmtVarDecl : public Statement
{
private:
    TypeList *types;
    NameList *names;
    ExpressionList *inits;

    /**
     * Create a new variable declaration with corresponding lists of
     * types, names, and initialization values.  The three lists
     * passed in are duplicated, and may be mutated after calling this
     * constructor without changing the value of this object.  The
     * types and names must all be non-null; if a particular variable
     * is uninitialized, the corresponding initializer value may be
     * null.
     *
     * @param  context  Context indicating what line and file this
     *                  statement is created at
     * @param  types    List of <code>Type</code> of the variables
     *                  declared here
     * @param  names    List of <code>String</code> of the names of the
     *                  variables declared here
     * @param  inits    List of <code>Expression</code> (or
     *                  <code>null</code>) containing initializers of
     *                  the variables declared here
     */
public:
    StmtVarDecl(FEContext *context, TypeList *types, NameList *names,
		ExpressionList *inits) : Statement(context)
    {
        // TODO: check for validity, including types of object
        // in the lists and that all three are the same length.
        this->types = types;
        this->names = names;
        this->inits = inits;
    }

    /**
     * Create a new variable declaration with exactly one variable
     * in it.  If the variable is uninitialized, the initializer may
     * be <code>null</code>.
     *
     * @param  context  Context indicating what line and file this
     *                  statement is created at
     * @param  type     Type of the variable
     * @param  name     Name of the variable
     * @param  init     Expression initializing the variable, or
     *                  <code>null</code> if the variable is uninitialized
     */
    StmtVarDecl(FEContext *context, Type *type, string name,
		Expression *init) : Statement(context)
    {
        this->types = new TypeList;
        this->names = new NameList;
        this->inits = new ExpressionList;

	this->types->push_back(type);
	this->names->push_back(name);
	this->inits->push_back(init);
    }
    
    /**
     * Get the type of the nth variable declared by this.
     *
     * @param  n  Number of variable to retrieve (zero-indexed).
     * @return    Type of the nth variable.
     */
    Type *getType(int n)
    {
        return (*types)[n];
    }

    /**
     * Get an immutable list of the types of all of the variables
     * declared by this.
     *
     * @return  Unmodifiable list of <code>Type</code> of the
     *          variables in this
     */
    TypeList *getTypes()
    {
        return types;
    }
    
    /**
     * Get the name of the nth variable declared by this.
     *
     * @param  n  Number of variable to retrieve (zero-indexed).
     * @return    Name of the nth variable.
     */
    string getName(int n)
    {
        return (*names)[n];
    }
    
    /**
     * Get an immutable list of the names of all of the variables
     * declared by this.
     *
     * @return  Unmodifiable list of <code>String</code> of the
     *          names of the variables in this
     */
    NameList *getNames()
    {
        return names;
    }
    
    /**
     * Get the initializer of the nth variable declared by this.
     *
     * @param  n  Number of variable to retrieve (zero-indexed).
     * @return    Expression initializing the nth variable, or
     *            <code>null</code> if the variable is
     *            uninitialized.
     */
    Expression *getInit(int n)
    {
        return (*inits)[n];
    }
    
    /**
     * Get an immutable list of the initializers of all of the
     * variables declared by this.  Members of the list may be
     * <code>null</code> if a particular variable is uninitialized.
     *
     * @return  Unmodifiable list of <code>Expression</code> (or
     *          <code>null</code>) of the initializers of the
     *          variables in this
     */
    ExpressionList *getInits()
    {
        return inits;
    }
    
    /**
     * Get the number of variables declared by this.  This value should
     * always be at least 1.
     *
     * @return  Number of variables declared
     */
    int getNumVars()
    {
        // CLAIM: the three lists have the same length.
        return types->size();
    }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
	return v->visitStmtVarDecl(this);
    }

//     public boolean equals(Object other)
//     {
//         if (!(other instanceof StmtVarDecl))
//             return false;
//         StmtVarDecl svd = (StmtVarDecl)other;
//         if (svd.types.size() != types.size())
//             return false;
//         for (int i = 0; i < types.size(); i++)
//             {
//                 if (!(types.get(i).equals(svd.types.get(i))))
//                     return false;
//                 if (!(names.get(i).equals(svd.names.get(i))))
//                     return false;
//                 if (inits.get(i) == null && svd.inits.get(i) != null)
//                     return false;
//                 if (inits.get(i) != null &&
//                     !(inits.get(i).equals(svd.inits.get(i))))
//                     return false;
//             }
//         return true;
//     }
    
//     public int hashCode()
//     {
//         // just use the first type and name.
//         return types.get(0).hashCode() ^ names.get(0).hashCode();
//     }

//     public String toString()
//     {
//         StringBuffer result = new StringBuffer();
//         for (int i = 0; i < types.size(); i++)
//             {
//                 if (i != 0)
//                     result.append("; ");
//                 result.append(types.get(i) + " " + names.get(i));
//                 if (inits.get(i) != null)
//                     result.append("=" + inits.get(i));
//             }
//         return result.toString();
//     }
};

}

#endif
