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
#ifndef _FUNCTION_HPP_
#define _FUNCTION_HPP_

#include "FENode.hpp"
#include "Type.hpp"
#include "Parameter.hpp"
#include "Expression.hpp"
#include "Statement.hpp"
#include "TypePrimitive.hpp"

namespace streamit {

/**
 * A function declaration in a StreamIt program.  This may be an init
 * function, work function, helper function, or message handler.  A
 * function has a class (one of the above), an optional name, an
 * optional parameter list, a return type (void for anything other
 * than helper functions), and a body.  It may also have rate
 * declarations; there are expressions corresponding to the number of
 * items peeked at, popped, and pushed per steady-state execution.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: Function.java,v 1.9 2006/08/23 23:01:08 thies Exp $
 */
class Function : public FENode
{
public:
    // Classes:
    enum {
	FUNC_INIT = 1,
	FUNC_WORK = 2,
	FUNC_HANDLER = 3,
	FUNC_HELPER = 4,
	FUNC_CONST_HELPER = 5,
	FUNC_BUILTIN_HELPER = 6,
	FUNC_PREWORK = 7,
	FUNC_NATIVE = 8
    };

private:
    int cls;
    string name; // or null
    Type *returnType;
    ParameterList *params;
    Statement *body;
    Expression *peekRate, *popRate, *pushRate;
    
public:
    /** Internal constructor to create a new Function from all parts.
     * This is public so that visitors that want to create new objects
     * can, but you probably want one of the other creator functions.
     *
     * The I/O rates may be null if declarations are omitted from the
     * original source. */
    Function(FEContext *context, int cls, string name,
	     Type *returnType, ParameterList *params, Statement *body,
	     Expression *peek, Expression *pop, Expression *push) : FENode(context)
    {
        this->cls = cls;
        this->name = name;
        this->returnType = returnType;
        this->params = params;
        this->body = body;
        this->peekRate = peek;
        this->popRate = pop;
        this->pushRate = push;
    }
    
    /** Create a new init function given its body.  An init function
     * may not do I/O on the tapes. */
    static Function *newInit(FEContext *context, Statement *body)
    {
        return new Function(context, FUNC_INIT, string(),
                            new TypePrimitive(TypePrimitive::TYPE_VOID),
                            new ParameterList, body,
                            NULL, NULL, NULL);
    }

    /** Create a new message handler given its name (not NULL), parameters,
     * and body.  A message handler may not do I/O on the tapes.  */
    static Function *newHandler(FEContext *context, string name,
				ParameterList *params, Statement *body)
    {
        return new Function(context, FUNC_HANDLER, name,
                            new TypePrimitive(TypePrimitive::TYPE_VOID),
                            params, body,
                            NULL, NULL, NULL);
    }
    
    /** Create a new helper function given its parts. */
    static Function *newHelper(FEContext *context, string name,
			       Type *returnType, ParameterList *params,
			       Statement *body, Expression *peek,
			       Expression *pop, Expression *push)
    {
        return new Function(context, FUNC_HELPER, name, returnType,
                            params, body, peek, pop, push);
    }

    /** Returns the class of this function as an integer. */
    int getCls() 
    {
        return cls;
    }
    
    /** Returns the name of this function, or NULL if it is anonymous. */
    string getName()
    {
        return name;
    }
    
    /** Returns the parameters of this function, as a List of Parameter
     * objects. */
    ParameterList *getParams()
    {
        return params;
    }
    
    /** Returns the return type of this function. */
    Type *getReturnType()
    {
        return returnType;
    }
    
    /** Returns the body of this function, as a single statement
     * (likely a StmtBlock). */
    Statement *getBody()
    {
        return body;
    }
    
    /** Gets the peek rate of this. */
    Expression *getPeekRate() 
    {
        return peekRate;
    }
    
    /** Gets the pop rate of this. */
    Expression *getPopRate()
    {
        return popRate;
    }
    
    /** Gets the push rate of this. */
    Expression *getPushRate()
    {
        return pushRate;
    }
    
    /** Returns whether this filter might do I/O. */
//     bool doesIO() {
//         // for now, detect I/O rates as 0 if they are NULL or equal to
//         // the constant int of 0.  This might miss a few parameterized
//         // cases where the I/O rate is a parameter to the filter that
//         // happens to be zero.
//         //ExprConstInt zero = new ExprConstInt(0);
//         bool noPeek = peekRate==NULL || peekRate->equals(0);
//         bool noPop = popRate==NULL || popRate->equals(0);
//         bool noPush = pushRate==NULL || pushRate->equals(0);
//         return !noPeek || !noPop || !noPush;
//    }

    /** Accepts a front-end visitor. */
    void *accept(FEVisitor *v)
    {
        return v->visitFunction(this);
    }

    bool isFunction() { return true; }
};

typedef vector<Function*> FunctionList;

}

#endif
