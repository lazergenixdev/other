/* Generated by re2c 3.0 on Sun Oct 23 20:57:47 2022 */
#line 1 "lexer.re"

#line 8 "lexer.re"



Token lex_string( Lex_Context& ctx )
{
	ctx.begin = ctx.cursor;
repeat:
	const char * YYMARKER;
	
	
#line 16 "lexer.h"
{
	char yych;
	yych = *ctx.cursor;
	switch (yych) {
		case 0x00: goto yy2;
		case '\n': goto yy1;
		case '"': goto yy5;
		case '\\': goto yy6;
		default: goto yy3;
	}
yy1:
yy2:
	++ctx.cursor;
#line 19 "lexer.re"
	{ return ctx.make_Token(token::END); }
#line 32 "lexer.h"
yy3:
	++ctx.cursor;
yy4:
#line 21 "lexer.re"
	{ goto repeat; }
#line 38 "lexer.h"
yy5:
	++ctx.cursor;
#line 18 "lexer.re"
	{ return ctx.make_String(); }
#line 43 "lexer.h"
yy6:
	yych = *++ctx.cursor;
	switch (yych) {
		case '"': goto yy7;
		default: goto yy4;
	}
yy7:
	++ctx.cursor;
#line 20 "lexer.re"
	{ goto repeat; }
#line 54 "lexer.h"
}
#line 22 "lexer.re"

}


Token lex( Lex_Context& ctx )
{
repeat:
	const char * YYMARKER = ctx.cursor;
	ctx.begin = ctx.cursor;
	
	
#line 68 "lexer.h"
{
	char yych;
	yych = *ctx.cursor;
	switch (yych) {
		case 0x00: goto yy9;
		case 0x08:
		case '\t':
		case '\v':
		case '\f':
		case ' ': goto yy12;
		case '\n': goto yy13;
		case '\r': goto yy15;
		case '"': goto yy16;
		case '#': goto yy17;
		case '.': goto yy18;
		case '@': goto yy19;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case '_':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z': goto yy21;
		default: goto yy10;
	}
yy9:
	++ctx.cursor;
#line 47 "lexer.re"
	{ return ctx.make_Token(token::END); }
#line 144 "lexer.h"
yy10:
	++ctx.cursor;
yy11:
#line 49 "lexer.re"
	{ return ctx.make_Token(); }
#line 150 "lexer.h"
yy12:
	++ctx.cursor;
#line 38 "lexer.re"
	{ ctx.advance(); goto repeat; }
#line 155 "lexer.h"
yy13:
	++ctx.cursor;
yy14:
#line 41 "lexer.re"
	{ return ctx.make_Newline(); }
#line 161 "lexer.h"
yy15:
	yych = *++ctx.cursor;
	switch (yych) {
		case '\n': goto yy13;
		default: goto yy14;
	}
yy16:
	++ctx.cursor;
#line 40 "lexer.re"
	{ return lex_string(ctx); }
#line 172 "lexer.h"
yy17:
	yych = *(YYMARKER = ++ctx.cursor);
	switch (yych) {
		case '-': goto yy23;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': goto yy25;
		default: goto yy11;
	}
yy18:
	yych = *(YYMARKER = ++ctx.cursor);
	switch (yych) {
		case 'a': goto yy27;
		case 'd': goto yy28;
		case 't': goto yy29;
		default: goto yy11;
	}
yy19:
	yych = *++ctx.cursor;
	switch (yych) {
		case '\n':
		case '\r': goto yy20;
		default: goto yy19;
	}
yy20:
#line 37 "lexer.re"
	{ goto repeat; }
#line 207 "lexer.h"
yy21:
	yych = *++ctx.cursor;
	switch (yych) {
		case '.':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case '_':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z': goto yy21;
		default: goto yy22;
	}
yy22:
#line 43 "lexer.re"
	{ return ctx.make_Identifier(); }
#line 280 "lexer.h"
yy23:
	yych = *++ctx.cursor;
	switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': goto yy25;
		default: goto yy24;
	}
yy24:
	ctx.cursor = YYMARKER;
	goto yy11;
yy25:
	yych = *++ctx.cursor;
	switch (yych) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': goto yy25;
		default: goto yy26;
	}
yy26:
#line 42 "lexer.re"
	{ return ctx.make_Immediate(); }
#line 317 "lexer.h"
yy27:
	yych = *++ctx.cursor;
	switch (yych) {
		case 's': goto yy30;
		default: goto yy24;
	}
yy28:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'a': goto yy31;
		default: goto yy24;
	}
yy29:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'e': goto yy32;
		default: goto yy24;
	}
yy30:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'c': goto yy33;
		default: goto yy24;
	}
yy31:
	yych = *++ctx.cursor;
	switch (yych) {
		case 't': goto yy34;
		default: goto yy24;
	}
yy32:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'x': goto yy35;
		default: goto yy24;
	}
yy33:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'i': goto yy36;
		default: goto yy24;
	}
yy34:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'a': goto yy37;
		default: goto yy24;
	}
yy35:
	yych = *++ctx.cursor;
	switch (yych) {
		case 't': goto yy38;
		default: goto yy24;
	}
yy36:
	yych = *++ctx.cursor;
	switch (yych) {
		case 'z': goto yy39;
		default: goto yy24;
	}
yy37:
	++ctx.cursor;
#line 45 "lexer.re"
	{ return ctx.make_Token(token::data); }
#line 382 "lexer.h"
yy38:
	++ctx.cursor;
#line 44 "lexer.re"
	{ return ctx.make_Token(token::text); }
#line 387 "lexer.h"
yy39:
	++ctx.cursor;
#line 46 "lexer.re"
	{ return ctx.make_Token(token::asciz); }
#line 392 "lexer.h"
}
#line 50 "lexer.re"

}

int lex_mnemonic(const char *& s) {
	const char * b = s;
	const char * YYMARKER;
		
	
#line 403 "lexer.h"
{
	char yych;
	yych = *s;
	switch (yych) {
		case 'a': goto yy43;
		case 'b': goto yy44;
		case 'c': goto yy46;
		case 'l': goto yy47;
		case 'm': goto yy48;
		case 'p': goto yy49;
		case 's': goto yy50;
		case 'u': goto yy51;
		default: goto yy41;
	}
yy41:
	++s;
yy42:
#line 78 "lexer.re"
	{ return -1; }
#line 423 "lexer.h"
yy43:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'd': goto yy52;
		default: goto yy42;
	}
yy44:
	yych = *++s;
	switch (yych) {
		case 'l': goto yy54;
		case 'x': goto yy55;
		default: goto yy45;
	}
yy45:
#line 60 "lexer.re"
	{ if(s-b == 1) return 7; return -1; }
#line 440 "lexer.h"
yy46:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'm': goto yy56;
		default: goto yy42;
	}
yy47:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'd': goto yy57;
		default: goto yy42;
	}
yy48:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'o': goto yy58;
		case 'u': goto yy59;
		default: goto yy42;
	}
yy49:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'o': goto yy60;
		case 'u': goto yy61;
		default: goto yy42;
	}
yy50:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'd': goto yy62;
		case 't': goto yy63;
		case 'u': goto yy64;
		default: goto yy42;
	}
yy51:
	yych = *(YYMARKER = ++s);
	switch (yych) {
		case 'd': goto yy65;
		default: goto yy42;
	}
yy52:
	yych = *++s;
	switch (yych) {
		case 'd': goto yy66;
		default: goto yy53;
	}
yy53:
	s = YYMARKER;
	goto yy42;
yy54:
	++s;
#line 62 "lexer.re"
	{ if(*s == 't' || *s == 'e') { --s; return 7; } return 9; }
#line 494 "lexer.h"
yy55:
	++s;
#line 61 "lexer.re"
	{ if(s-b == 2) return 8; return -1; }
#line 499 "lexer.h"
yy56:
	yych = *++s;
	switch (yych) {
		case 'p': goto yy67;
		default: goto yy53;
	}
yy57:
	yych = *++s;
	switch (yych) {
		case 'r': goto yy68;
		default: goto yy53;
	}
yy58:
	yych = *++s;
	switch (yych) {
		case 'v': goto yy69;
		default: goto yy53;
	}
yy59:
	yych = *++s;
	switch (yych) {
		case 'l': goto yy70;
		default: goto yy53;
	}
yy60:
	yych = *++s;
	switch (yych) {
		case 'p': goto yy71;
		default: goto yy53;
	}
yy61:
	yych = *++s;
	switch (yych) {
		case 's': goto yy72;
		default: goto yy53;
	}
yy62:
	yych = *++s;
	switch (yych) {
		case 'i': goto yy73;
		default: goto yy53;
	}
yy63:
	yych = *++s;
	switch (yych) {
		case 'r': goto yy74;
		default: goto yy53;
	}
yy64:
	yych = *++s;
	switch (yych) {
		case 'b': goto yy75;
		default: goto yy53;
	}
yy65:
	yych = *++s;
	switch (yych) {
		case 'i': goto yy76;
		default: goto yy53;
	}
yy66:
	++s;
#line 66 "lexer.re"
	{ if(s-b == 3) return 2; return -1; }
#line 564 "lexer.h"
yy67:
	++s;
#line 63 "lexer.re"
	{ if(s-b == 3) return 12; return -1; }
#line 569 "lexer.h"
yy68:
	++s;
#line 75 "lexer.re"
	{ if(s-b == 3) return 13; return -1; }
#line 574 "lexer.h"
yy69:
	++s;
#line 65 "lexer.re"
	{ if(s-b == 3) return 1; return -1; }
#line 579 "lexer.h"
yy70:
	++s;
#line 68 "lexer.re"
	{ if(s-b == 3) return 4; return -1; }
#line 584 "lexer.h"
yy71:
	++s;
#line 73 "lexer.re"
	{ if(s-b == 3) return 11; return -1; }
#line 589 "lexer.h"
yy72:
	yych = *++s;
	switch (yych) {
		case 'h': goto yy77;
		default: goto yy53;
	}
yy73:
	yych = *++s;
	switch (yych) {
		case 'v': goto yy78;
		default: goto yy53;
	}
yy74:
	++s;
#line 76 "lexer.re"
	{ if(s-b == 3) return 14; return -1; }
#line 606 "lexer.h"
yy75:
	++s;
#line 67 "lexer.re"
	{ if(s-b == 3) return 3; return -1; }
#line 611 "lexer.h"
yy76:
	yych = *++s;
	switch (yych) {
		case 'v': goto yy79;
		default: goto yy53;
	}
yy77:
	++s;
#line 72 "lexer.re"
	{ if(s-b == 4) return 10; return -1; }
#line 622 "lexer.h"
yy78:
	++s;
#line 69 "lexer.re"
	{ if(s-b == 4) return 5; return -1; }
#line 627 "lexer.h"
yy79:
	++s;
#line 70 "lexer.re"
	{ if(s-b == 4) return 6; return -1; }
#line 632 "lexer.h"
}
#line 79 "lexer.re"

}

int lex_register(const char * s) {
	const char * b = s;
	const char * YYMARKER;
		
	
#line 643 "lexer.h"
{
	char yych;
	yych = *s;
	switch (yych) {
		case 'f': goto yy83;
		case 'l': goto yy84;
		case 'p': goto yy85;
		case 'r': goto yy86;
		case 's': goto yy87;
		default: goto yy81;
	}
yy81:
	++s;
yy82:
#line 105 "lexer.re"
	{ return -1; }
#line 660 "lexer.h"
yy83:
	yych = *++s;
	switch (yych) {
		case 'p': goto yy88;
		default: goto yy82;
	}
yy84:
	yych = *++s;
	switch (yych) {
		case 'r': goto yy89;
		default: goto yy82;
	}
yy85:
	yych = *++s;
	switch (yych) {
		case 'c': goto yy90;
		default: goto yy82;
	}
yy86:
	yych = *++s;
	switch (yych) {
		case '0': goto yy91;
		case '1': goto yy92;
		case '2': goto yy94;
		case '3': goto yy95;
		case '4': goto yy96;
		case '5': goto yy97;
		case '6': goto yy98;
		case '7': goto yy99;
		case '8': goto yy100;
		case '9': goto yy101;
		default: goto yy82;
	}
yy87:
	yych = *++s;
	switch (yych) {
		case 'p': goto yy102;
		default: goto yy82;
	}
yy88:
	++s;
#line 100 "lexer.re"
	{ if(s-b == 2) return 11; return -1; }
#line 704 "lexer.h"
yy89:
	++s;
#line 102 "lexer.re"
	{ if(s-b == 2) return 14; return -1; }
#line 709 "lexer.h"
yy90:
	++s;
#line 103 "lexer.re"
	{ if(s-b == 2) return 15; return -1; }
#line 714 "lexer.h"
yy91:
	++s;
#line 89 "lexer.re"
	{ if(s-b == 2) return 0; return -1; }
#line 719 "lexer.h"
yy92:
	yych = *++s;
	switch (yych) {
		case '0': goto yy103;
		default: goto yy93;
	}
yy93:
#line 90 "lexer.re"
	{ if(s-b == 2) return 1; return -1; }
#line 729 "lexer.h"
yy94:
	++s;
#line 91 "lexer.re"
	{ if(s-b == 2) return 2; return -1; }
#line 734 "lexer.h"
yy95:
	++s;
#line 92 "lexer.re"
	{ if(s-b == 2) return 3; return -1; }
#line 739 "lexer.h"
yy96:
	++s;
#line 93 "lexer.re"
	{ if(s-b == 2) return 4; return -1; }
#line 744 "lexer.h"
yy97:
	++s;
#line 94 "lexer.re"
	{ if(s-b == 2) return 5; return -1; }
#line 749 "lexer.h"
yy98:
	++s;
#line 95 "lexer.re"
	{ if(s-b == 2) return 6; return -1; }
#line 754 "lexer.h"
yy99:
	++s;
#line 96 "lexer.re"
	{ if(s-b == 2) return 7; return -1; }
#line 759 "lexer.h"
yy100:
	++s;
#line 97 "lexer.re"
	{ if(s-b == 2) return 8; return -1; }
#line 764 "lexer.h"
yy101:
	++s;
#line 98 "lexer.re"
	{ if(s-b == 2) return 9; return -1; }
#line 769 "lexer.h"
yy102:
	++s;
#line 101 "lexer.re"
	{ if(s-b == 2) return 13; return -1; }
#line 774 "lexer.h"
yy103:
	++s;
#line 99 "lexer.re"
	{ if(s-b == 3) return 10; return -1; }
#line 779 "lexer.h"
}
#line 106 "lexer.re"

}

int lex_condition(const char *& s) {
	const char * b = s;
	const char * YYMARKER;
		
	
#line 790 "lexer.h"
{
	char yych;
	yych = *s;
	switch (yych) {
		case 'e': goto yy107;
		case 'g': goto yy108;
		case 'l': goto yy109;
		case 'n': goto yy110;
		default: goto yy105;
	}
yy105:
	++s;
yy106:
#line 123 "lexer.re"
	{ --s; return -1; }
#line 806 "lexer.h"
yy107:
	yych = *++s;
	switch (yych) {
		case 'q': goto yy111;
		default: goto yy106;
	}
yy108:
	yych = *++s;
	switch (yych) {
		case 'e': goto yy112;
		case 't': goto yy113;
		default: goto yy106;
	}
yy109:
	yych = *++s;
	switch (yych) {
		case 'e': goto yy114;
		case 't': goto yy115;
		default: goto yy106;
	}
yy110:
	yych = *++s;
	switch (yych) {
		case 'e': goto yy116;
		default: goto yy106;
	}
yy111:
	++s;
#line 116 "lexer.re"
	{ if(s-b == 2) return 0; return -1; }
#line 837 "lexer.h"
yy112:
	++s;
#line 119 "lexer.re"
	{ if(s-b == 2) return 3; return -1; }
#line 842 "lexer.h"
yy113:
	++s;
#line 118 "lexer.re"
	{ if(s-b == 2) return 2; return -1; }
#line 847 "lexer.h"
yy114:
	++s;
#line 121 "lexer.re"
	{ if(s-b == 2) return 5; return -1; }
#line 852 "lexer.h"
yy115:
	++s;
#line 120 "lexer.re"
	{ if(s-b == 2) return 4; return -1; }
#line 857 "lexer.h"
yy116:
	++s;
#line 117 "lexer.re"
	{ if(s-b == 2) return 1; return -1; }
#line 862 "lexer.h"
}
#line 124 "lexer.re"

}