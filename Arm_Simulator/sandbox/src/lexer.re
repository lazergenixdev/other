
/*!re2c
	re2c:yyfill:enable   = 0;
	re2c:define:YYCTYPE  = char;
	re2c:define:YYCURSOR = ctx.cursor;
	
	newline = "\r\n" | [\r\n] ;
*/


Token lex_string( Lex_Context& ctx )
{
	ctx.begin = ctx.cursor;
repeat:
	const char * YYMARKER;
	
	/*!re2c
		'"'    { return ctx.make_String(); }
		"\x00" { return ctx.make_Token(token::END); }
		'\\"'  { goto repeat; }
		.      { goto repeat; }
	*/
}


Token lex( Lex_Context& ctx )
{
repeat:
	const char * YYMARKER = ctx.cursor;
	ctx.begin = ctx.cursor;
	
	/*!re2c
		identifier = [a-zA-Z_] [a-zA-Z_.0-9]* ;
		whitespace = [\t\b\v\f ]             ;
		sl_comment = "@" [^\r\n]*            ;

		sl_comment     { goto repeat; }
		whitespace     { ctx.advance(); goto repeat; }
		
		'"'            { return lex_string(ctx); }
		newline        { return ctx.make_Newline(); }
		"#" "-"? [0-9]+{ return ctx.make_Immediate(); }
		identifier     { return ctx.make_Identifier(); }
		".text"        { return ctx.make_Token(token::text); }
		".data"        { return ctx.make_Token(token::data); }
		".asciz"       { return ctx.make_Token(token::asciz); }
		"\x00"         { return ctx.make_Token(token::END); }

		.              { return ctx.make_Token(); }
	*/
}

int lex_mnemonic(const char *& s) {
	const char * b = s;
	const char * YYMARKER;
		
	/*!re2c
		re2c:define:YYCURSOR = s;
	
		"b"     { if(s-b == 1) return 7; return -1; }
		"bx"    { if(s-b == 2) return 8; return -1; }
		"bl"    { if(*s == 't' || *s == 'e') { --s; return 7; } return 9; }
		"cmp"   { if(s-b == 3) return 12; return -1; }
		
		"mov"   { if(s-b == 3) return 1; return -1; }
		"add"   { if(s-b == 3) return 2; return -1; }
		"sub"   { if(s-b == 3) return 3; return -1; }
		"mul"	{ if(s-b == 3) return 4; return -1; }
		"sdiv"  { if(s-b == 4) return 5; return -1; }
		"udiv"  { if(s-b == 4) return 6; return -1; }
		
		"push"  { if(s-b == 4) return 10; return -1; }
		"pop"   { if(s-b == 3) return 11; return -1; }
		
		"ldr"   { if(s-b == 3) return 13; return -1; }
		"str"   { if(s-b == 3) return 14; return -1; }
		
		*       { return -1; }
	*/
}

int lex_register(const char * s) {
	const char * b = s;
	const char * YYMARKER;
		
	/*!re2c
		re2c:define:YYCURSOR = s;
	
		"r0"  { if(s-b == 2) return 0; return -1; }
		"r1"  { if(s-b == 2) return 1; return -1; }
		"r2"  { if(s-b == 2) return 2; return -1; }
		"r3"  { if(s-b == 2) return 3; return -1; }
		"r4"  { if(s-b == 2) return 4; return -1; }
		"r5"  { if(s-b == 2) return 5; return -1; }
		"r6"  { if(s-b == 2) return 6; return -1; }
		"r7"  { if(s-b == 2) return 7; return -1; }
		"r8"  { if(s-b == 2) return 8; return -1; }
		"r9"  { if(s-b == 2) return 9; return -1; }
		"r10" { if(s-b == 3) return 10; return -1; }
		"fp"  { if(s-b == 2) return 11; return -1; }
		"sp"  { if(s-b == 2) return 13; return -1; }
		"lr"  { if(s-b == 2) return 14; return -1; }
		"pc"  { if(s-b == 2) return 15; return -1; }
		
		*    { return -1; }
	*/
}

int lex_condition(const char *& s) {
	const char * b = s;
	const char * YYMARKER;
		
	/*!re2c
		re2c:define:YYCURSOR = s;
	
		"eq"  { if(s-b == 2) return 0; return -1; }
		"ne"  { if(s-b == 2) return 1; return -1; }
		"gt"  { if(s-b == 2) return 2; return -1; }
		"ge"  { if(s-b == 2) return 3; return -1; }
		"lt"  { if(s-b == 2) return 4; return -1; }
		"le"  { if(s-b == 2) return 5; return -1; }
		
		*    { --s; return -1; }
	*/
}