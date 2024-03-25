#include <stdint.h>
#include <string>
#include <iostream>

struct String {
	const char * data;
	uint64_t count;
};

struct source_location {
    const char* source;          // line in the source where location is
    int col_first = 1, col_last; // column number
    int line = 1;                // line number
};

#define XTOKEN_END X(END, "end of file", 0)
#define XTOKEN_IDENTIFIER X(identifier,"identifier",256)

#define XTOKEN_LITERAL                     \
X(string_literal, "string literal",500)    \
X(immediate,"integer immediate",501) \

#define XTOKEN_OTHER     \
X(dot_identifier,"dot identifier",600) \
X(asciz,".asciz",601) \
X(data,".data",602) \
X(text,".text",603) \
X(newline,"newline",604) \

#define XALL_TOKENS XTOKEN_END XTOKEN_IDENTIFIER XTOKEN_LITERAL XTOKEN_OTHER

namespace token {
	enum type : int32_t {
#define X(NAME,_,VALUE) NAME = VALUE,
		XALL_TOKENS
#undef X
	};
}

struct Token {
	token::type type;
	source_location loc;
	union {
		uint32_t intValue;
		String stringValue;
		String name;
	};
};

static const char * token_repr(token::type t) {
	static char buffer[4] = {'\'',0,'\'','\0'}; // threading issue?
	switch( t )
	{
#define X(NAME,STR,_) case token::NAME: return STR;
		XALL_TOKENS
#undef X
	default: { buffer[1] = (char)t; return buffer; }
	}
}


	// +----------------> When making tokens <---------------+
	// | cursor will be set to the end of a token            |
	// | begin will be set to the first character in a token |
	// +-----------------------------------------------------+
	
struct Lex_Context {
	source_location loc;

	const char* cursor;
	const char* begin;
	
	std::string temp;
	
	Token make_Token( token::type t ) {
		auto out = Token { .type = t, .loc = loc };
		out.loc.col_last = out.loc.col_first + int(cursor - begin);
		loc.col_first = out.loc.col_last;
		return out;
	}

	Token make_Token() {
		auto out = Token { .type = token::type( cursor[-1]&0xFF ), .loc = loc};
		out.loc.col_last = out.loc.col_first;
		out.loc.col_last++;

		advance();
		return out;
	}
	
	Token make_Newline() {
		auto out = Token { .type = token::type( token::newline ), .loc = loc};
		out.loc.col_last = out.loc.col_first;
		out.loc.col_last++;

		nextline();
		return out;
	}
	
	Token make_Immediate() {
		auto out = Token { .type = token::immediate, .loc = loc };

		const char * b = begin + 1;
		auto s = std::string(b, cursor-b);
		
		out.intValue = std::stoi(s);
	//	std::cout << '\n' << out.intValue;

		out.loc.col_last = out.loc.col_first + (int)out.name.count;

		loc.col_first = out.loc.col_last;
		return out;
	}
	
	Token make_Identifier() {
		auto out = Token { .type = token::identifier, .loc = loc };

		out.name.data = begin;
		out.name.count = cursor - begin;

		out.loc.col_last = out.loc.col_first + (int)out.name.count;

		loc.col_first = out.loc.col_last;
		
		return out;
	}
	
	Token make_String() {
		auto out = Token { .type = token::string_literal, .loc = loc };

		out.name.data = begin;
		out.name.count = cursor - begin - 1;
		
		temp.clear();
		temp.reserve(out.name.count);

		if( out.name.count > 1 )
		for(int i = 0; i < out.name.count; ++i) {
			auto& ch = out.name.data[i];
			auto& next = out.name.data[i+1];
			
			switch(ch) {
			case '\\': {
				switch(next)
				{
				case 'n': {
					temp.push_back('\n');
					++i;
					break;
				}
				case '"': {
					temp.push_back( '"' );
					++i;
					break;
				}
				case '\\': {
					temp.push_back( '\\' );
					++i;
					break;
				}
				default:break;
				}
				break;
			}
			default: {
				temp.push_back(ch);
				break;
			}
			}
		}
		out.name.data = temp.data();
		out.name.count = temp.size();

		out.loc.col_last = out.loc.col_first + (int)out.name.count;
		loc.col_first = out.loc.col_last;
		
		return out;
	}
			
	inline constexpr void advance() {
		++loc.col_first;
	}

	inline constexpr void advance(int n) {
		loc.col_first += n;
		loc.col_first += n;
	}

	inline constexpr void nextline() {
		loc.source = cursor;
		++loc.line;
		loc.col_first = 1;
	}
};
