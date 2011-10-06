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
#ifndef _STREAMSPEC_HPP_
#define _STREAMSPEC_HPP_

#include "FENode.hpp"
#include "StreamType.hpp"
#include "Parameter.hpp"
#include "FieldDecl.hpp"
#include "Function.hpp"
#include "FuncWork.hpp"

namespace streamit {

/**
 * Container class containing all of the state for a StreamIt stream
 * type.  A StreamSpec may or may not have a name; if there is no
 * name, this is an anonymous stream.  It also has a type (as an
 * integer), a stream type (with I/O data types), a parameter list, a
 * list of variable declarations (as <code>Statement</code>s; they
 * should all actually be <code>StmtVarDecl</code>s), and a list of
 * function declarations (as <code>Function</code> objects).  The
 * stream type may be <code>null</code>, in which case the compiler
 * will need to determine the stream type on its own.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: StreamSpec.java,v 1.18 2006/08/23 23:01:08 thies Exp $
 */
class StreamSpec : public FENode
{
private:
    int type;
    StreamType *st;
    string name;
    ParameterList *params;
    FieldDeclList *fields;
    FieldDeclList *locals;
    FunctionList *funcs;
    StreamSpec *parent;

public:
    enum {
	/** Stream type constant for a filter. */
	STREAM_FILTER = 1,
	/** Stream type constant for a pipeline. */
	STREAM_PIPELINE = 2,
	/** Stream type constant for a split-join. */
	STREAM_SPLITJOIN = 3,
	/** Stream type constant for a feedback loop. */
	STREAM_FEEDBACKLOOP = 4,
	/** Stream type constant for globals. */
	STREAM_GLOBAL = 5
    };
    
    /**
     * Creates a new stream specification given its name, a list of
     * variables, and a list of functions.
     *
     * @param context  front-end context indicating file and line
     *                 number for the specification
     * @param type     STREAM_* constant indicating the type of
     *                 stream object
     * @param st       stream type giving input and output types of
     *                 the stream object
     * @param name     string name of the object
     * @param params   list of <code>Parameter</code> that are formal
     *                 parameters to the stream object
     * @param vars     list of <code>StmtVarDecl</code> that are
     *                 fields of a filter stream
     * @param funcs    list of <code>Function</code> that are member
     *                 functions of the stream object
     */
    StreamSpec(FEContext *context, int type, StreamType *st,
	       string name, ParameterList *params, 
	       FieldDeclList *vars, FunctionList *funcs) : FENode(context)
    {
        this->type = type;
        this->st = st;
        this->name = name;
        this->params = params;
        this->fields = vars;
        this->funcs = funcs;
	this->locals = new FieldDeclList;
	this->parent = NULL;
    }
    
    /**
     * Creates a new stream specification given its name and the text
     * of its init function.  Useful for composite streams that have
     * no other functions.
     *
     * @param context  front-end context indicating file and line
     *                 number for the specification
     * @param type     STREAM_* constant indicating the type of
     *                 stream object
     * @param st       stream type giving input and output types of
     *                 the stream object
     * @param name     string name of the object
     * @param params   list of <code>Parameter</code> that are formal
     *                 parameters to the stream object
     * @param init     statement containing initialization code for
     *                 the object
     */
    StreamSpec(FEContext *context, int type, StreamType *st,
	       string name, ParameterList *params, Statement *init) : FENode(context)
    {
        this->type = type;
        this->st = st;
        this->name = name;
        this->params = params;
        this->fields = new FieldDeclList;
	this->locals = new FieldDeclList;
        this->funcs = new FunctionList;
	this->funcs->push_back(Function::newInit(init->getContext(), init));
        //this(context, type, st, name, params, Collections.EMPTY_LIST,
	//Collections.singletonList(Function.newInit(init.getContext(),
	//init)));
	this->parent = NULL;
    }

    StreamSpec *getParent()
    {
	return parent;
    }

    void setParent(StreamSpec *p)
    {
	parent = p;
    }

    /**
     * Returns the type of this, as one of the integer constants above.
     *
     * @return  integer type of the stream object
     */
    int getType()
    {
        return type;
    }

    /**
     * Returns the type of this, as a String (Pipeline, SplitJoin, FeedbackLoop, Filter).
     *
     * @return  String type of the stream object
     */
    string getTypeString() {
        switch(type) {
        case STREAM_FILTER:
            return string("Filter");
        case STREAM_PIPELINE:
            return string("Pipeline");
        case STREAM_SPLITJOIN:
            return string("SplitJoin");
        case STREAM_FEEDBACKLOOP:
            return string("FeedbackLoop");
        }
        return string();
    }

    /**
     * Returns the stream type (I/O data types) of this.
     *
     * @return  stream type containing input and output types of the
     *          stream, or null if a stream type was not explicitly
     *          included in the code
     */
    StreamType *getStreamType()
    {
        return st;
    }

    /**
     * Returns the name of this, or null if this is an anonymous stream.
     *
     * @return  string name of the object, or null for an anonymous stream
     */
    string getName()
    {
        return name;
    }
    void setName(string s)
    {
        name = s;
    }

    /**
     * Returns the formal parameters of the stream object.
     *
     * @return  list of {@link Parameter}
     */
     ParameterList *getParams()
     {
         return params;
     }
    
    /**
     * Returns the field variables declared in this, as a list of
     * Statements.  Each of the statements will probably be a
     * {@link StmtVarDecl}.
     *
     * @return  list of {@link Statement}
     */
     FieldDeclList *getFields()
     {
         return fields;
     }
    
    
    /**
     * Returns the local variables declared in this, as a list of
     * Statements.  Each of the statements will probably be a
     * {@link StmtVarDecl}.
     *
     * @return  list of {@link Statement}
     */
    FieldDeclList *getLocals()
    {
	return locals;
    }

    /**
     * Returns the functions declared in this, as a list of Functions.
     *
     * @return  list of {@link Function}
     */
     FunctionList *getFuncs()
     {
         return funcs;
     }

    /**
     * Returns the init function declared in this, or null.  If multiple
     * init functions are declared (probably an error), returns one
     * arbitrarily.
     *
     * @return  function containing the initialiation code
     */
    Function *getInitFunc()
    {
	FunctionList::iterator iter;
	for (iter = funcs->begin(); iter != funcs->end(); iter++) {
	    Function *f = *iter;
	    if (f->getCls() == Function::FUNC_INIT)
		return f;
	}
	return NULL;
    }

    /**
     * Returns the work function declared in this, or null.  If multiple
     * work functions are declared (probably an error), returns one
     * arbitrarily.
     *
     * @return  function containing steady-state work code, or null for
     *          non-filters
     */
    FuncWork *getWorkFunc()
    {
	FunctionList::iterator iter;
	for (iter = funcs->begin(); iter != funcs->end(); iter++) {
	    Function *f = *iter;
	    if (f->getCls() == Function::FUNC_WORK)
		return (FuncWork *)f;
	}
	return NULL;
    }

    /** 
     * Returns a list of the helper functions in this that do I/O.
     *
     * @return list of helper functions performing I/O
     */
//     FunctionList *getHelperIOFuncs() {
//         FunctionList *ioList = new FunctionList();
// 	FunctionList::iterator iter;
//         for (iter = funcs->begin(); iter != funcs->end(); iter++) {
//             Function *aFunction = *iter;
// 	    if (aFunction->getCls() == Function::FUNC_HELPER &&
//                 aFunction->doesIO()) {
//                 ioList->push_back(aFunction);
//             }
//         }
//         return ioList;
//     }
    
    /**
     * Returns the function with a given name contained in this, or
     * null.  name should not be null.  If multiple functions are
     * declared with the same name (probably an error), returns one
     * arbitrarily.
     *
     * @return  function named name, or null
     */
    Function *getFuncNamed(string name)
    {
	FunctionList::iterator iter;
        for (iter = funcs->begin(); iter != funcs->end(); iter++)
            {
                Function *func = *iter;
                string fname = func->getName();
                if (fname == name)
                    return func;
            }
        return NULL;
    }

    /**
     * Accept a front-end visitor.
     *
     * @param v  front-end visitor to accept
     * @return   object returned from the visitor
     * @see      FEVisitor#visitStreamSpec
     */
    void *accept(FEVisitor *v)
    {
        return v->visitStreamSpec(this);
    }
};

typedef vector<StreamSpec*> StreamSpecList;

}

#endif
