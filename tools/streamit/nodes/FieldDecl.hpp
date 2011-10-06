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
#ifndef _FIELDDECL_HPP_
#define _FIELDDECL_HPP_

#include "FENode.hpp"
#include "Type.hpp"
#include "Expression.hpp"

namespace streamit {

typedef vector<string> NameList;

/**
 * Declaration of a set of fields in a filter or structure.  This
 * describes the declaration of a list of variables, each of which has
 * a type, a name, and an optional initialization value.  This is
 * explicitly not a <code>Statement</code>; declarations that occur
 * inside functions are local variable declarations, not field
 * declarations.  Similarly, this is not a stream parameter (in
 * StreamIt code; it may be in Java code).
 *
 * @see     StmtVarDecl
 * @see     Parameter
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: FieldDecl.java,v 1.5 2003/10/09 19:50:59 dmaze Exp $
 */
class FieldDecl : public FENode
{
 private:
    TypeList *types;
    NameList *names;
    ExpressionList *inits;

public:
    /**
     * Create a new field declaration with corresponding lists of
     * types, names, and initialization values.  The three lists
     * passed in are duplicated, and may be mutated after calling this
     * constructor without changing the value of this object.  The
     * types and names must all be non-null; if a particular field is
     * uninitialized, the corresponding initializer value may be null.
     *
     * @param  context  Context indicating what line and file this
     *                  field is created at
     * @param  types    List of <code>Type</code> of the fields
     *                  declared here
     * @param  names    List of <code>String</code> of the names of the
     *                  fields declared here
     * @param  inits    List of <code>Expression</code> (or
     *                  <code>null</code>) containing initializers of
     *                  the fields declared here
     */
    FieldDecl(FEContext *context, TypeList *types, 
	      NameList* names, ExpressionList *inits) 
	: FENode(context), types(types), names(names), inits(inits)
    {
    }

    /**
     * Create a new field declaration with exactly one variable in it.
     * If the field is uninitialized, the initializer may be
     * <code>null</code>.
     *
     * @param  context  Context indicating what line and file this
     *                  field is created at
     * @param  type     Type of the field
     * @param  name     Name of the field
     * @param  init     Expression initializing the field, or
     *                  <code>null</code> if the field is uninitialized
     */
    FieldDecl(FEContext *context, Type *type, string name,
	      Expression *init) : FENode(context)
    {
	types = new TypeList;
	names = new NameList;
	inits = new ExpressionList;

	types->push_back(type);
	names->push_back(name);
	inits->push_back(init);
    }
    
    /**
     * Get the type of the nth field declared by this.
     *
     * @param  n  Number of field to retrieve (zero-indexed)
     * @return    Type of the nth field
     */
    Type *getType(int n)
    {
        return (*types)[n];
    }

    /**
     * Get an immutable list of the types of all of the fields
     * declared by this.
     *
     * @return  Unmodifiable list of <code>Type</code> of the
     *          fields in this
     */
    TypeList *getTypes()
    {
        return types;
    }
    
    /**
     * Get the name of the nth field declared by this.
     *
     * @param  n  Number of field to retrieve (zero-indexed)
     * @return    Name of the nth field
     */
    string getName(int n)
    {
        return (*names)[n];
    }
    
    /**
     * Get an immutable list of the names of all of the fields
     * declared by this.
     *
     * @return  Unmodifiable list of <code>String</code> of the
     *          names of the fields in this
     */
    NameList *getNames()
    {
        return names;
    }
    
    /**
     * Get the initializer of the nth field declared by this.
     *
     * @param  n  Number of field to retrieve (zero-indexed)
     * @return    Expression initializing the nth field, or
     *            <code>null</code> if the field is
     *            uninitialized
     */
    Expression *getInit(int n)
    {
        return (*inits)[n];
    }
    
    /**
     * Get an immutable list of the initializers of all of the field
     * declared by this.  Members of the list may be <code>null</code>
     * if a particular field is uninitialized.
     *
     * @return  Unmodifiable list of <code>Expression</code> (or
     *          <code>null</code>) of the initializers of the
     *          fields in this
     */
    ExpressionList *getInits()
    {
        return inits;
    }
    
    /**
     * Get the number of fields declared by this.  This value should
     * always be at least 1.
     *
     * @return  Number of fields declared
     */
    int getNumFields()
    {
        // CLAIM: the three lists have the same length.
        return types->size();
    }

    /** Accept a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitFieldDecl(this);
    }
};

typedef vector<FieldDecl*> FieldDeclList;

}

#endif
