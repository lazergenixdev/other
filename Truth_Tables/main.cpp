// g++ -std=c++17 -w -o truth_table main.cpp
#include <cstdint>  // better ints
#include <string>   // reading from console to a string
#include <iostream> // basic IO

// Maximum number of Nodes in the AST, increase to support bigger expressions
static constexpr auto max_nodes = 64;

using u8  = uint8_t;
using u16 = uint16_t;

enum token : int {
	End        = 0,
	Identifier = 1,
	If_Then    = '->',
	Error      = 255,
};

struct Token {
	int type;
	char ch;
};

enum Expr_ {
	Expr_Identifier = 0,
	Expr_Unary      = 1,
	Expr_Binary     = 2,
};

struct Ast_Expr {
	u8        id;
	char      name;
	u16       op;
	Ast_Expr* left; // <- unary operand stored here
	Ast_Expr* right;
};

// Print as an AST:
void print_ast(Ast_Expr*, int d = 0);

// Print as a normal expression:
int print(Ast_Expr*, bool prefer_parenthesis = false);

void print_boolean_centered(bool value, int wid);

#define ERROR_TOKEN Token { .type = token::Error }
#define ALPHABET                             \
X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) \
X(j) X(k) X(l) X(m) X(n) X(o) X(p) X(q) X(r) \
X(s) X(t) X(u) X(v) X(w) X(x) X(y) X(z)

// Get next token in string
Token lex(u8 const* &s) {
	u8 c = *s;
	
	while(c) {
		
		// Handle whitespace:
		if( c == ' ' || c == '\t' ) {
			c = *++s;
			continue;
		}
		
		// Handle Arrow:
		if( c == '-' ) {
			c = *++s;
			if( c == '>' ) {
				++s;
				return Token { .type = token::If_Then };
			}
			return ERROR_TOKEN; // '-' is not seen as a token itself
		}
		
		// Handle Identifier:
		switch(c) {
			#define X(n) case #n[0]:
			ALPHABET // handle only single letters
			#undef X
			{
				++s;
				return Token { .type = token::Identifier, .ch = (char)c };
			}
			default:break;
		}
		
		// Handle rest:
		++s;
		return Token { .type = (int)c }; // use char value as the token type.
	}
	// Handle End of Source
	return Token { .type = token::End };
}

struct Parse_Context {
	u8 const * cursor;
	Token      current_token;
	Token      peeked_token;
	bool       peeking;
	
	// Basic Bump Allocator:
	struct Ast_Expr* free;
	struct Ast_Expr* mem_end;
	
	void next() {
		if( !peeking ) {
			current_token = lex(cursor);
			return;
		}
		peeking = false;
		current_token = peeked_token;
	}
	void peek() {
		if( peeking )
			return;
		
		peeking = true;
		peeked_token = lex(cursor);
	}
	
	struct Ast_Expr* alloc() {
		if( free == mem_end ) {
			std::cout << "Ran out of static memory!\n";
			exit(1);
		}
		auto p = free;
		++free;
		return p;
	}
};

int get_precedence(int op) {
	switch( op ) {
		case '->': return 1;
		case '&':
		case '|':  return 2;
		default:   return 0; // @Note: nothing can have zero precedence
	}
}

Ast_Expr* parse_basic_expression(Parse_Context& ctx) {
	switch(ctx.current_token.type) {
		case token::Identifier: {
			auto ident = ctx.alloc();
			ident->id   = Expr_Identifier;
			ident->name = ctx.current_token.ch;
			return ident;
		}
		default: return nullptr;
	}
}

// @Note: hard-coded
#define is_unary(x) (x == '~')

Ast_Expr* parse_expression(Parse_Context& ctx, int precedence = -69420) {
	Ast_Expr* left;
	
	// Parse Expressions Wrapped in Parenthesis:
	if(ctx.current_token.type == '(') {
		ctx.next();
		left = parse_expression(ctx);
		
		ctx.next();
		if( ctx.current_token.type != ')' ) {
			std::cout << "error: unmatched parenthesis!\n";
			return nullptr;
		}
	}
	
	// Parse Unary Expressions
	else if( is_unary(ctx.current_token.type) ) {
		ctx.next();
		
		left = parse_expression(ctx, 3); // @Fix: maybe don't hard-code precedence here?
		if( !left ) return nullptr;
			
		// assume it is the negation operator
		auto neg = ctx.alloc();
		neg->id   = Expr_Unary;
		neg->op   = '~';
		neg->left = left;
		left = neg;
	}
	
	// Default: 
	else {
		left = parse_basic_expression(ctx);
		if( !left ) return nullptr;
	}
	
	ctx.peek();
	
append_operator:
	int prec = get_precedence(ctx.peeked_token.type);
	
	// return early to ensure operator precedence
	if( !prec || prec < precedence )
		return left;
	
	{
		int op = ctx.peeked_token.type;
		
		ctx.next(); // go to operator token
		ctx.next(); // go to whatever is after the operator
		
		// Assume it is a binary operator at this point
		auto right = parse_expression(ctx, prec);
		if( !right ) return nullptr;

		auto binary = ctx.alloc();
		binary->id = Expr_Binary;
		binary->op = op;
		binary->left = left;
		binary->right = right;
		left = binary;
		
		ctx.peek(); // get ready for next operator (in case there is one)
	}
		
	goto append_operator;
}

struct Variable {
	char name;
	u8   index;
	u8   _padding;
	bool value;
};

bool var_exists(Variable v, Variable const* vars, int count) {
	for(int i = 0; i < count; ++i)
		if( v.name == vars[i].name )
			return true;
	return false;
}

int var_fill_set(Variable* data, Ast_Expr* root) {
	int count = 0;
	
	struct Info {
		Ast_Expr const* expr;
		int visits = 0;
	};
	
	static Info stack[max_nodes];
	int i = 0; // stack pointer
	stack[0] = { root };
	
	// I don't want to make spaghetti code, but switch statements are soo AWEFUL
visit_node:
//	std::cout << "visit #" << i << ": " << stack[i].expr << " " << stack[i].visits << '\n';
	if(i == -1)
		return count;

	if(stack[i].expr == nullptr)
		std::cout << "[bug]\n";

	switch(stack[i].expr->id) {
		case Expr_Binary: {
			auto& visits = stack[i].visits;
			if           (visits == 0) stack[i+1] = { stack[i].expr->left, 0 }, i += 1;
			else if (visits == 1) stack[i+1] = { stack[i].expr->right, 0 }, i += 1;
			else                  --i; // pop the stack
			
			++visits;
			goto visit_node;
		}
		case Expr_Unary: {
			auto& visits = stack[i].visits;
			if (visits == 0)  stack[i+1] = { stack[i++].expr->left };
			else              --i; // pop the stack
			
			++visits;
			goto visit_node;
		}
		case Expr_Identifier: {
			Variable name = { stack[i].expr->name, (u8)count };
			
			// Add variable to set if we never seen it before.
			if( !var_exists(name, data, count) ) {
				data[count++] = name;
			}
			
			--i; // pop stack
			goto visit_node;
		}
		default: { std::cout << "Cannot reach here, literally impossible\n"; exit(1); }
	}
}

template <int max_subexpressions>
void get_sub_expressions(Ast_Expr* e, Ast_Expr** dest, int& count) {
	if( count == max_subexpressions ) {
		std::cout << "Not enough memory for sub-expressions!\n";
		exit(0);
	}
		
	switch(e->id) {
		case Expr_Binary: {
			if( e->left->id != Expr_Identifier ) {
				dest[count++] = e->left;
				get_sub_expressions<max_subexpressions>(e->left, dest, count);
			}
			if( e->right->id != Expr_Identifier ) {
				dest[count++] = e->right;
				get_sub_expressions<max_subexpressions>(e->right, dest, count);
			}
			return;
		}
		case Expr_Unary: {
			if( e->left->id != Expr_Identifier ) {
				dest[count++] = e->left;
				get_sub_expressions<max_subexpressions>(e->left, dest, count);
			}
			return;
		}
		default:return;
	}
}

bool interp_boolean_logic(Ast_Expr* e, Variable* vars, int var_count) {
	switch(e->id) {
		case Expr_Identifier: {
			for(int i = 0; i < var_count; ++i)
				if(vars[i].name == e->name)
					return vars[i].value;
			std::cout << "cannot get here.\n";
			return false;
		}
		case Expr_Unary: {
			return !interp_boolean_logic(e->left, vars, var_count);
		}
		case Expr_Binary: {
			bool left  = interp_boolean_logic(e->left,  vars, var_count);
			bool right = interp_boolean_logic(e->right, vars, var_count);
			
			static bool if_then_table[4] = {
				true,  // 00 (0)
				true,  // 01 (1)
				false, // 10 (2)
				true,  // 11 (3)
			};
			
			switch(e->op) {
				case '|':  return left || right;
				case '&':  return left && right;
				case '->': return if_then_table[((int)left << 1) | (int)right];
			}
		}
		default: { std::cout << "(interpreter) Cannot reach here, literally impossible\n"; exit(1); }
	}
}

int main() {
	try{
	std::string raw_input;
	std::cout << "Enter expression: ";
	std::getline(std::cin, raw_input);
	std::cout << '\n';
		
	static Ast_Expr expr_pool[max_nodes];
	Ast_Expr* root;
	{
		Parse_Context ctx = {};
		ctx.free    = expr_pool;
		ctx.mem_end = expr_pool + max_nodes;
		ctx.cursor  = reinterpret_cast<u8 const*>(raw_input.c_str());
		ctx.next();
		
		root = parse_expression(ctx);
		
		if( !root ) {
			std::cout << "Not a valid expression!\n";
			exit(0);
		}
	}
	
//	std::cout << "Abstract Syntax Tree:\n===============================\n";
//	print_ast(root);
//	std::cout << "\n===============================\n";
	
	Variable var_set[32];
	int var_count = var_fill_set(var_set, root);
	
//	std::cout << "\nVariable Set = {";
//	for(int i = 0; i < var_count; ++i) {
//		std::cout << var_set[i].name;
//		if(i != var_count-1) std::cout << ',' << ' ';
//	}
//	std::cout << '}' << '\n';
	
	int height = 1 << var_count;
//	std::cout << "Possible Combinations = " << height << '\n';
	
	Ast_Expr* sub_exprs[max_nodes >> 2];
	sub_exprs[0] = root;
	int sub_count = 1;
	get_sub_expressions<std::size(sub_exprs)>(root, sub_exprs, sub_count);
	
	int width = var_count + sub_count;

	// @Note: Column Major
	// Allocate truth table values
	bool* table = new bool[width*height];
	// Allocate space for the width of each column
	int* col_widths = new int[width];
	
	////////////////////////////////////
	// Fill all variables combinations:
	{
		int h = height;
		for(int var_index = 0; var_index < var_count; ++var_index) {
			h = h >> 1;
			bool b = true;
			for(int i = 0, n = 0; i < height; ++i, ++n) {
				if(n >= h) { b = !b; n = 0; }
				table[var_index*height + i] = b;
			}
		}
	}
	////////////////////////////////////
	
	
	// Solve all expressions:
	
	for(int i = 0; i < sub_count; ++i) {
		int col = i + var_count;
		auto expr = sub_exprs[sub_count - i - 1];
		for(int row = 0; row < height; ++row) {
			
			// Init variables
			for(int v = 0; v < var_count; ++v) {
				var_set[v].value = table[v*height + row];
			}
			
			// Solve
			table[col*height + row] = interp_boolean_logic(expr, var_set, var_count);
		}
	}
	
//	std::cout << '\n';
	std::cout << ' ';
	{
		int row = 0;
		for(int i = 0; i < var_count; ++i) {
			std::cout << var_set[i].name << "   ";
			col_widths[row++] = 3;
		}
		for(int i = sub_count-1; i >= 0; --i) {
			int w = print(sub_exprs[i]); std::cout << "   ";
			col_widths[row++] = w + 2;
		}
	}
	std::cout << '\n';
	
	for(int i = 0; i < width; ++i) {
		std::cout << '-';
		for(int j = 0; j < col_widths[i]; ++j)
			std::cout << '-';
	}
	std::cout << '\n';
	
	for(int row = 0; row < height; ++row) {
		for(int col = 0; col < width; ++col) {
			print_boolean_centered(table[col*height + row], col_widths[col]);
			std::cout << ' ';
		}
		std::cout << '\n';
	}

	return 0;
	}
	catch (std::exception& e) {
		std::cout << "error: " << e.what() << '\n';
	}
	catch (...) {
		std::cout << "unknown error";
	}
}

inline void scope(int n) {
	for(int i = 0; i < n; ++i)
		std::cout << ':' << ' ' << ' ';
}

const char * op_to_str(int op) {
	switch(op) {
		case '|' : return "OR";
		case '&' : return "AND";
		case '->': return "If-Then";
		default:   return "?";
	}
}

void print_ast(Ast_Expr* e, int depth) {
	scope(depth);
	
	if( !e ) {
		std::cout << "[null]";
		return;
	}
	
	switch(e->id) {
		case Expr_Identifier: {
			std::cout << e->name;
			break;
		}
		case Expr_Unary: {
			std::cout << "[negate]\n";
			print_ast(e->left, depth + 1);
			break;
		}
		case Expr_Binary: {
			std::cout << "[binary] op = ";
			std::cout << op_to_str(e->op) << '\n';
			
			print_ast(e->left,  depth + 1);
			std::cout << '\n';
			print_ast(e->right, depth + 1);
			
			break;
		}
	}
}


int print(Ast_Expr* e, bool prefer_parenthesis) {
	if( !e ) {
		std::cout << "(null)";
		return 6;
	}
	
	switch(e->id) {
		case Expr_Identifier: {
			std::cout << e->name;
			return 1;
		}
		case Expr_Unary: {
			std::cout << '~'; // @Note: assume negation
			return print(e->left, true) + 1;
		}
		case Expr_Binary: {
			if(prefer_parenthesis) std::cout << '(';
			
			auto prec = get_precedence(e->op);
			
			int l = print(e->left,  e->left->id == Expr_Binary && get_precedence(e->left->op) <= prec);
			
			l += 4;
			if(e->op == '->') std::cout << " -> ";
			else { std::cout << ' ' << (char)e->op << ' '; --l; }
			
			l += print(e->right, e->right->id == Expr_Binary && get_precedence(e->right->op) <= prec);
			
			if(prefer_parenthesis) std::cout << ')';
			
			return l + (int)prefer_parenthesis * 2;
		}
		default:return 0;
	}
}

void print_boolean_centered(bool value, int wid) {
	int mid = wid / 2;
	for(int i = 0; i < mid; ++i) std::cout << ' ';
	
	// @Note: This coloring scheme will not work in all terminals
	std::cout << (value? "\x1b[92mT":"\x1b[91mF");
	std::cout << "\x1b[0m";
	
	// fill in the rest:
	mid = wid - mid - 1;
	for(int i = 0; i < mid; ++i) std::cout << ' ';
}
