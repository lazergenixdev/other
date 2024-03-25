#include "parser.hpp"
#include "lexer.h"

#include <vector>

static const char * parser_error_message = "no error";
static int 			parser_error_line_number = 0;

using reg_t = unsigned char;

struct Label {
	String name;
	int line_number;
};

#define XALL_INSTRUCTIONS \
X(nop , 0) \
X(mov , 1) \
X(add , 2) \
X(sub , 3) \
X(mul , 4) \
X(sdiv, 5) \
X(udiv, 6) \
X(b,    7) \
X(bx,   8) \
X(bl,   9) \
X(push, 10) \
X(pop,  11) \
X(cmp,  12) \
X(ldr,  13) \
X(str,  14) \

namespace instruction {
	enum id {
#define X(NAME, VALUE) NAME = VALUE,
		XALL_INSTRUCTIONS
#undef X
	};
}
const char * instr_repr(int id) {
	switch(id) {
#define X(NAME, VALUE) case VALUE: return #NAME;
		XALL_INSTRUCTIONS
#undef X
	default: return "unknown";
	}
}
#define XALL_CONDITIONS \
X(eq, 0)   \
X(ne, 1)   \
X(gt, 2)   \
X(ge, 3)   \
X(lt, 4)   \
X(le, 5)   \

namespace condition {
	enum id {
#define X(NAME,VALUE) NAME = VALUE,
		XALL_CONDITIONS
#undef X
	};
}

namespace operand {
	enum id {
		reg, imm, label,
	};
}
struct Operand {
	int id;
	union {
		String label_name;
		reg_t reg;
		int imm;
	};
};
struct Operand2 : public Operand {
	unsigned int shift_operator; // top bit set if we using register
	union {
		reg_t shift_reg;
		int shift_imm;
	};
};

struct Instruction {
	int line_number;
	
	int id;
	int cond;
	
	Operand  op0;
	union {
		int label_line_number;
		Operand  op1;
	};
	Operand2 op2;
};

struct Program {
	std::vector<Label>       labels;
	std::vector<Instruction> instructions;
	std::unordered_map<std::string,std::string> static_strings;
	std::vector<std::string> string_labels;

	std::unordered_map<int, Instruction*> map_line_to_ins;

	void clear() {
		labels.clear();
		instructions.clear();
		static_strings.clear();
		string_labels.clear();
		map_line_to_ins.clear();
	}
};

struct Parse_Context {
	Lex_Context lexContext;
	Token       currentToken;
	Token       lookAhead;
	bool        peeking = false;
	bool		text_section = true;
	
	Program*	program;
	
	// Get next token after the current
	inline Token& next() {
		if( !peeking )
			return currentToken = lex(lexContext);
		peeking = false;
		return currentToken = lookAhead;
	}
	// Get next token after the current without replacing the current
	inline Token& peek() {
		if( peeking )
			return lookAhead;
		peeking = true;
		return lookAhead = lex(lexContext);
	}
};

#define expect(T,W) if(ctx.currentToken.type != T) { parser_error_message = W; return 1; }

int parse_register(Parse_Context& ctx, Operand& dst) {
	dst.id = operand::reg;
	expect(token::identifier, "expected register");
	dst.reg = lex_register(ctx.currentToken.name.data);
	if( dst.reg == -1 ) {
		parser_error_message = "not a valid register";
		return 1;
	}
	return 0;
}
int parse_register_or_imm(Parse_Context& ctx, Operand& dst) {
	switch(ctx.currentToken.type) {
	case token::identifier: {
		dst.id = operand::reg;
		dst.reg = lex_register(ctx.currentToken.name.data);
		if( dst.reg == -1 ) {
			parser_error_message = "not a valid register";
			return 1;
		}
		return 0;
	}
	case token::immediate: {
		dst.id = operand::imm;
		dst.imm = ctx.currentToken.intValue;
		return 0;
	}
	default: {
		parser_error_message = "expected a register or immediate";
		return 1;
	}
	}
}
int parse_label(Parse_Context& ctx, Operand& dst) {
	dst.id = operand::label;
	expect(token::identifier, "expected label");
	dst.label_name = ctx.currentToken.name;
	return 0;
}

int parse_instruction(Parse_Context& ctx) {
	switch(ctx.currentToken.type) {
	case token::identifier: {
		ctx.peek();
		
		// Handle Labels
		if( ctx.lookAhead.type == ':' ) {
			Label l;
			l.name = ctx.currentToken.name;
			l.line_number = ctx.lexContext.loc.line;
			ctx.program->labels.emplace_back(l);
			
			Instruction nop = {};
			nop.line_number = ctx.lexContext.loc.line;
			ctx.program->instructions.emplace_back(nop);
			
			ctx.next();
			ctx.next();
			if( ctx.currentToken.type != token::newline ) {
				parser_error_message = "expected a newline after label";
				parser_error_line_number = ctx.currentToken.loc.line;
				return 1;
			}
			return 0;
		}
		
		Instruction ins = {};
		ins.cond = 0;
		ins.line_number = ctx.currentToken.loc.line;
		parser_error_line_number = ins.line_number;
		{
			const char* s = ctx.currentToken.name.data;

			// Read name of mnemonic
			ins.id = lex_mnemonic(ctx.currentToken.name.data);
			if( ins.id == -1 ) {
				parser_error_message = "unknown mnemonic";
				return 1;
			}
			// Read condition; if there is one
			ins.cond = lex_condition(ctx.currentToken.name.data);

			if( ctx.currentToken.name.data - s != ctx.currentToken.name.count ) {
				parser_error_message = "invalid instruction condition";
				return 1;
			}
		}
		
		// Handle Instruction inputs
		switch(ins.id) {
		case instruction::b: {
			ctx.next(); // expecting a label
			if( parse_label(ctx,ins.op0) )
				return 1;
			
			ctx.next(); // expecting a newline
			expect(token::newline, "expected newline at end of instruction");
			
			break;
		}
		case instruction::bx: {
			ctx.next(); // expecting a register
			if( parse_register(ctx,ins.op0) )
				return 1;
			
			ctx.next(); // expecting a newline
			expect(token::newline, "expected newline at end of instruction");
			
			break;
		}
		case instruction::bl: {
			ctx.next(); // expecting a label
			if( parse_label( ctx, ins.op0 ) )
				return 1;

			ctx.next(); // expecting a newline
			expect( token::newline, "expected newline at end of instruction" );
			break;
		}
		case instruction::ldr: {
			ctx.next(); // expecting a register
			if( parse_register( ctx, ins.op0 ) )
				return 1;

			ctx.next(); // expecting a ','
			expect( ',', "expected ','" );

			ctx.next(); // expecting a '[' or '='
			if( ctx.currentToken.type == '[' ) {
				ctx.next(); // expecting a register
				if( parse_register( ctx, ins.op1 ) )
					return 1;

				ctx.next(); // either a ',' or ']'
				if( ctx.currentToken.type == ']' ) {
					ins.op2.id = operand::imm;
					ins.op2.imm = 0;
				}
				else if( ctx.currentToken.type == ',' ) {
					ctx.next(); // expect imm or register
					if( parse_register_or_imm(ctx, ins.op2) )
						return 1;

					ctx.next(); // expect ']'
					expect( ']', "expected closing ']'" );
				}
				else {
					parser_error_message = "expected ',' or ']' after register";
					return 1;
				}

			} else if( ctx.currentToken.type == '=' ) {
				ctx.next();
				expect(token::identifier, "expected a label after '='");

				ins.op1.id = operand::label;
				ins.op1.label_name = ctx.currentToken.name;
			} else {
				parser_error_message = "expected '[' or '=' with ldr instruction";
				return 1;
			}

			ctx.next(); // expecting a newline
			expect( token::newline, "expected newline at end of instruction" );

			break;
		}
		case instruction::str: {
			ctx.next(); // expecting a register
			if( parse_register( ctx, ins.op0 ) )
				return 1;

			ctx.next(); // expecting a ','
			expect( ',', "expected ','" );

			ctx.next(); // expecting a '['
			if( ctx.currentToken.type == '[' ) {
				ctx.next(); // expecting a register
				if( parse_register( ctx, ins.op1 ) )
					return 1;

				ctx.next(); // either a ',' or ']'
				if( ctx.currentToken.type == ']' ) {
					ins.op2.id = operand::imm;
					ins.op2.imm = 0;
				}
				else if( ctx.currentToken.type == ',' ) {
					ctx.next(); // expect imm or register
					if( parse_register_or_imm( ctx, ins.op2 ) )
						return 1;

					ctx.next(); // expect ']'
					expect( ']', "expected closing ']'" );
				}
				else {
					parser_error_message = "expected ',' or ']' after register";
					return 1;
				}

			}
			else {
				parser_error_message = "expected '[' with str instruction";
				return 1;
			}

			ctx.next(); // expecting a newline
			expect( token::newline, "expected newline at end of instruction" );

			break;
		}
		case instruction::push:
		case instruction::pop: {
			ctx.next(); // expect '{'
			expect( '{', "expected '{'" );

			ctx.next(); // expect a register
			if( parse_register(ctx, ins.op0) )
				return 1;

			ctx.next(); // expect '}'
			expect( '}', "expected '}'" );
			ctx.next(); // expecting a newline
			expect( token::newline, "expected newline at end of instruction" );
			break;
		}
		case instruction::mov: {
			ctx.next(); // expecting a register
			if( parse_register(ctx,ins.op0) )
				return 1;

			ctx.next(); // expecting a ','
			expect(',', "expected ','");

			ctx.next(); // expecting a register
			if( parse_register_or_imm(ctx,ins.op2) )
				return 1;
			
			ctx.next(); // expecting a newline
			expect(token::newline, "expected newline at end of instruction");
			
			break;
		}
		case instruction::cmp: {
			ctx.next(); // expecting a register
			if( parse_register( ctx, ins.op0 ) )
				return 1;

			ctx.next(); // expecting a ','
			expect( ',', "expected ','" );

			ctx.next(); // expecting a register
			if( parse_register_or_imm( ctx, ins.op2 ) )
				return 1;

			ctx.next(); // expecting a newline
			expect( token::newline, "expected newline at end of instruction" );

			break;
		}
		default: {
			ctx.next(); // expecting a register
			if( parse_register(ctx,ins.op0) )
				return 1;

			ctx.next(); // expecting a ','
			expect(',', "expected ','");

			ctx.next(); // expecting a register
			if( parse_register(ctx,ins.op1) )
				return 1;

			ctx.next(); // expecting a ','
			expect(',', "expected ','");

			ctx.next(); // expecting a register
			if( ins.id != instruction::udiv && ins.id != instruction::sdiv && ins.id != instruction::mul ) {
				if( parse_register_or_imm(ctx,ins.op2) )
					return 1;
			} else
			if( parse_register( ctx, ins.op2 ) )
				return 1;
			
			ctx.next(); // expecting a newline
			expect(token::newline, "expected newline at end of instruction");
			
			break;
		}
		}
		
		ctx.program->instructions.emplace_back( ins );
		return 0;
	}
	default: return 1;
	}
}

static int parse_data(Parse_Context& ctx) {
	auto& strings = ctx.program->static_strings;

	expect(token::identifier, "expected label");
	auto name = std::string(ctx.currentToken.name.data, ctx.currentToken.name.count);

	ctx.next();
	expect(':', "expected ':' after label name");

	ctx.next();
	expect(token::asciz, "only .asciz is implemented for data");

	ctx.next();
	expect(token::string_literal, "expected string literal");

	strings.emplace( name, ctx.lexContext.temp );
	ctx.program->string_labels.push_back( name );
	
	ctx.next(); // expecting a newline
	expect( token::newline, "expected newline at end of data" );

	return 0;
}

static int build_program(const char * src, Program& program) {
	Parse_Context ctx = {};
	ctx.lexContext.cursor     = src;
	ctx.lexContext.loc.source = ctx.lexContext.cursor;
	ctx.program = &program;
	ctx.next();

	while( ctx.currentToken.type != token::END )
	{
		switch( ctx.currentToken.type )
		{
		case token::newline: goto skip;
		case token::text: { ctx.text_section = true; goto skip; }
		case token::data: { ctx.text_section = false; goto skip; }
		default:break;
		}

		if( ctx.text_section ) {
			if( parse_instruction(ctx) )
				return 1;
		} else {
			if( parse_data(ctx) )
				return 1;
		}
	skip:
		ctx.next();
	}
	
	return 0;
}
