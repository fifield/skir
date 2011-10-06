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

/*
 * StreamItLex.g: Lexical tokens for StreamIt
 * $Id: StreamItLex.g,v 1.24 2006/08/23 23:01:03 thies Exp $
 */

header {

}

options {
	mangleLiteralPrefix = "TK_";
	language="Cpp";
}

class StreamItLex extends Lexer;
options {
	exportVocab=StreamItLex;
	charVocabulary = '\3'..'\377';
	k=3;
}

tokens {
	// Stream types:
	"filter"; "pipeline"; "splitjoin"; "feedbackloop";
	// Messaging:
	"portal"; "to"; "handler";
	// Composite streams:
	"add";
	// Splitters and joiners:
	"split"; "join";
	"duplicate"; "roundrobin";
	// Feedback loops:
	"body"; "loop"; "enqueue";
	// Special functions:
	"init"; "prework"; "work";
	// Manipulating tapes:
	"peek"; "pop"; "push";
	// Basic types:
	"boolean"; "float"; "bit"; "int"; "void"; "double"; "complex";
	TK_float2 = "float2"; TK_float3 = "float3"; TK_float4 = "float4"; 
	// Complicated types:
	"struct"; "template"; "native"; "static"; "helper";
	// Control flow:
	"if"; "else"; "while"; "do"; "for"; "switch"; "case"; "default"; "break";
	"continue"; "return";
	// Intrinsic values:
	"pi"; "true"; "false";
}

ARROW :	"->" ;

WS	:	(' '
	|	'\t'
	|	'\n'	{newline();}
	|	'\r')
		{ _ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP; }
	;


SL_COMMENT : 
	"//" 
	(~'\n')* '\n'
	{ _ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP; newline(); }
	;

ML_COMMENT
	:	"/*"
		(	{ LA(2)!='/' }? '*'
		|	'\n' { newline(); }
		|	~('*'|'\n')
		)*
		"*/"
			{ $setType(ANTLR_USE_NAMESPACE(antlr)Token::SKIP); }
	;


LPAREN
//options {
//	paraphrase="'('";
//}
	:	'('
	;

RPAREN
//options {
//	paraphrase="')'";
//}
	:	')'
	;

LCURLY:	'{' ;
RCURLY:	'}'	;
LSQUARE: '[' ;
RSQUARE: ']' ;
PLUS: '+' ;
PLUS_EQUALS: "+=" ;
INCREMENT: "++" ;
MINUS: '-' ;
MINUS_EQUALS: "-=" ;
DECREMENT: "--" ;
STAR: '*';
STAR_EQUALS: "*=" ;
DIV: '/';
DIV_EQUALS: "/=" ;
MOD: '%';
LOGIC_AND: "&&";
LOGIC_OR: "||";
BITWISE_AND: "&";
BITWISE_OR: "|";
BITWISE_XOR: "^";
BITWISE_COMPLEMENT: "~";
LSHIFT: "<<";
RSHIFT: ">>";
LSHIFT_EQUALS: "<<=";
RSHIFT_EQUALS: ">>=";
ASSIGN: '=';
EQUAL: "==";
NOT_EQUAL: "!=";
LESS_THAN: '<';
LESS_EQUAL: "<=";
MORE_THAN: '>';
MORE_EQUAL: ">=";
QUESTION: '?';
COLON: ':';
SEMI: ';';
COMMA: ',';
DOT: '.';
BANG: '!';

CHAR_LITERAL
	:	'\'' (ESC|~'\'') '\''
	;

STRING_LITERAL
	:	'"' (ESC|~'"')* '"'
	;

protected
ESC	:	'\\'
		(	'n'
		|	'r'
		|	't'
		|	'b'
		|	'f'
		|	'"'
		|	'\''
		|	'\\'
		|	'0'..'3'
			(
				options {
					warnWhenFollowAmbig = false;
				}
			:	DIGIT
				(
					options {
						warnWhenFollowAmbig = false;
					}
				:	DIGIT
				)?
			)?
		|	'4'..'7'
			(
				options {
					warnWhenFollowAmbig = false;
				}
			:	DIGIT
			)?
		)
	;

protected
DIGIT
	:	'0'..'9'
	;

HEXNUMBER
	:	 "0x" ( (DIGIT) | 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'a' | 'b' | 'c' | 'd' | 'e' | 'f')+ 
	;

NUMBER
	:	 (DIGIT)+ (DOT (DIGIT)+ )? (('e' | 'E') ('+'|'-')? (DIGIT)+ )? ('i'|'f')?
	;

ID
options {
	testLiterals = true;
	paraphrase = "an identifier";
}
	:	('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'_'|'0'..'9')*
	;


