#include "arm.hpp"
#include <Fission/Core/Console.hh>

namespace arm {
	
	struct Register {
		unsigned int      value;
		int         const index;
		std::string const name;
	};
	
	static unsigned int return_address = rand() * rand() + 9000;
#define R0	regs[0].value
#define R1	regs[1].value
#define R2	regs[2].value
#define R3	regs[3].value
#define R4	regs[4].value
#define R5	regs[5].value
#define R6	regs[6].value
#define R7	regs[7].value
#define R8	regs[8].value
#define R9	regs[9].value
#define R10	regs[10].value
#define FP	regs[11].value
#define IP	regs[12].value
#define SP	regs[13].value
#define LR	regs[14].value
#define PC	regs[15].value
#define CPSR regs[16].value
	static Register regs[17] = {
		{0,	0,		"r0"},
		{0,	1,		"r1"},
		{0,	2,		"r2"},
		{0,	3,		"r3"},
		{0,	4,		"r4"},
		{0,	5,		"r5"},
		{0,	6,		"r6"},
		{0,	7,		"r7"},
		{0,	8,		"r8"},
		{0,	9,		"r9"},
		{0,	10,		"r10"},
		{0,	11,		"r11 (fp)"},
		{0,	12,		"r12 (ip)"},
		{20000,		13,		"r13 (sp)"},
		{return_address,	14,		"r14 (lr)"},
		{0,			15,		"r15 (pc)"},
		{0,			16,		"CPSR"},
	};
	// 0b1   -> r0 was modified
	// 0b10  -> r1 was modified
	// 0b100 -> r2 was modified
	static unsigned int reg_modified_bits = 0;
	
	static inline void reset_modified_registers() {
		reg_modified_bits = 0;
	}
	static inline void modify_register(int reg, unsigned int new_value) {
		regs[reg].value = new_value;
		reg_modified_bits |= (1 << reg);
	}
	static inline int read_register(int reg) {
		return (signed int&)regs[reg].value;
	}
	static inline void simulate_function_call() {
		R0 = rand();
		R1 = rand();
		R2 = rand();
		R3 = rand();
		reg_modified_bits |= 0b1111;
	}
	
	
	enum Program_State_ {
		Program_State_Good  = 0,
		Program_State_Error = 1, // Seg. Fault
	};
	
	static int program_state = Program_State_Good;
	static const char * program_error_status = "Good";

	
	
	struct Stack {
		int data[256];
		int bottom_address = 20000;
		int random_value;

		Stack() { reset(); }

		void check() {
			if( data[0] != random_value) {
				program_state = Program_State_Error;
				program_error_status = "stack was corrupted";
				return;
			}
			if( to_real_address(SP) != data ) {
				program_state = Program_State_Error;
				program_error_status = "sp was not restored";
				return;
			}
		}
		
		void* to_real_address(int arm_addr) {
			return (unsigned char*)data + 20000 - arm_addr;
		}
		void push(int value) {
			if( (SP & 0b11) != 0 ) {
				program_state = Program_State_Error;
				program_error_status = "sp not aligned to 4-byte boundary!";
				return;
			}
			SP -= 4;
			reg_modified_bits |= (1 << 13);
			int* a = (int*)to_real_address(SP);
			if( a < data || a >= data + std::size(data) ) {
				program_state = Program_State_Error;
				program_error_status = "stack overflow!";
				return;
			}
			*a = value;
		}
		int pop() {
			if( (SP & 0b11) != 0 ) {
				program_state = Program_State_Error;
				program_error_status = "sp not aligned to 4-byte boundary!";
				return 0;
			}
			int* a = (int*)to_real_address(SP);
			if( a < data || a >= data + std::size(data) ) {
				program_state = Program_State_Error;
				program_error_status = "stack underflow!";
				return 0;
			}
			SP += 4;
			reg_modified_bits |= ( 1 << 13 );
			return *a;
		}
		void reset() {
			memset( &data, 0, sizeof( data ) );
			random_value = (rand() * 7323) >> 4;
			data[0] = random_value;
		}
	};
	
	static Stack stack;
	static int main_instruction_index;


	static inline void reset( int line_number ) {
		return_address = rand() * rand() + 9000;
		R0 = rand();
		R1 = rand();
		R2 = rand();
		R3 = rand();
		R4 = rand();
		R5 = rand();
		R6 = rand();
		R7 = rand();
		R8 = rand();
		R9 = rand();
		R10 = rand();
		FP = rand();
		IP = rand();
		SP = 20'000;
		LR = return_address;
		PC = 1000 + line_number;
		CPSR = 0;
		reset_modified_registers();
		stack.reset();
	}

	int get_operand_value(Operand& op) {
		switch( op.id )
		{
		case operand::reg: return read_register(op.reg);
		case operand::imm: return op.imm;
		default: {
			program_state = Program_State_Error;
			program_error_status = "internal error: invalid operand id";
			return 0;
		}
		}
	}
#define bit(IDX) static_cast<bool>( (CPSR>>IDX) & 1 )

	bool should_continue( int cond ) {
		static constexpr unsigned int N = 31, Z = 30, C = 29, V = 28;
		switch( cond )
		{
		case condition::eq: return bit(Z);
		case condition::ne: return !bit(Z);
		case condition::gt: return !bit(Z) && (bit(N) == bit(V));
		case condition::ge: return bit(N) == bit(V);
		case condition::lt: return bit(N) != bit(V);
		case condition::le: return bit(Z) || (bit(N) != bit(V));
		default:return true;
		}
	}

	namespace function {
		enum id {
			putchar = 1, printf, scanf,
		};
	}

	void verify_program( Program& p ) {
		std::string name, label;
		for( auto&& i : p.instructions ) {
			if( i.id == instruction::b ) {
				name.assign(i.op0.label_name.data, i.op0.label_name.count);
				for( auto&& l : p.labels ) {
					label = std::string(l.name.data, l.name.count);
					if( label == name ) {
						i.label_line_number = l.line_number;
						goto _continue;
					}
				}
				program_state = Program_State_Error;
				program_error_status = "label does not exist";
				return;
			_continue:
				(void)0;
			}
			else if( i.id == instruction::bl ) {
				i.label_line_number = 0;
				name.assign( i.op0.label_name.data, i.op0.label_name.count );
				for( auto&& l : p.labels ) {
					label = std::string( l.name.data, l.name.count );
					if( label == name ) {
						i.label_line_number = l.line_number + 1000;
						break;
					}
				}
				if( i.label_line_number == 0 ) {
					if( name == "putchar" ) {
						i.label_line_number = function::putchar;
					}
					else if( name == "printf" ) {
						i.label_line_number = function::printf;
					}

					if( i.label_line_number == 0 ) {
						program_state = Program_State_Error;
						program_error_status = "failed to resolve function";
						return;
					}
				}
			}
		}

		int main_line_number = -1;
		label = "main";
		for( auto&& l: p.labels ) {
			name.assign( l.name.data, l.name.count );
			if( name == label ) {
				main_line_number = l.line_number;
			}
		}
		if( main_line_number == -1 ) {
			program_state = Program_State_Error;
			program_error_status = "main not found";
			return;
		}
		int i = 0;
		for( auto&& ins : p.instructions ) {
			if( ins.line_number == main_line_number ) {
				main_instruction_index    = i;
				return;
			}
			++i;
		}
		program_state = Program_State_Error;
		program_error_status = "internal error: label has not line number";
	}
	
	void execute_next_instruction(Program& p) {
		reset_modified_registers();
		
		auto pc = PC;
		pc -= 1000;
		int i = 0;
		for(auto&& ins: p.instructions) {
			if( ins.line_number == pc ) break;
			++i;
		}
		if( i == p.instructions.size() ) {
			program_state = Program_State_Error;
			program_error_status = "invalid executing location";
			return;
		}
		
		// Load instruction to be executed
		auto& ins = p.instructions[i];
		
		// Advance PC to next instruction
		for(;;)
		{
			if( i+1 == p.instructions.size() ) {
				PC += 1;
				break;
			}
			auto& next = p.instructions[i+1];
			PC = 1000 + next.line_number;
			break;
		}
		
		// execute instruction
		switch(ins.id) {
		case instruction::nop: break;
		case instruction::b: {
			if( should_continue(ins.cond) ) {
				PC = ins.label_line_number + 1000;
				break;
			}
			break;
		}
		case instruction::bx: {
			modify_register( 15, read_register(ins.op0.reg) );
			break;
		}
		case instruction::bl: {
			switch( ins.label_line_number )
			{
			case function::putchar: {
				putchar( read_register(0) );
				simulate_function_call();
				modify_register( 14, PC );
				break;
			}
			case function::printf: {
				auto idx = read_register(0) - 10000;

				const char* fmt = p.static_strings[p.string_labels[idx]].c_str();
				printf( fmt, read_register(1), read_register(2), read_register(3) );
				simulate_function_call();
				modify_register( 14, PC );
				break;
			}
			default: {
				if( ins.label_line_number <= 1000 ) {
					program_state = Program_State_Error;
					program_error_status = "internal error: function not implemented";
				}
				modify_register( 14, PC );
				PC = ins.label_line_number; // pre-added
				break;
			}
			}
			break;
		}
		case instruction::cmp: {
			auto a = (long long)read_register( ins.op0.reg );
			auto b = (long long)get_operand_value( ins.op2 );

			int64_t r = a - b;

			bool N = r < 0;
			bool V = ( a >= b ? N : !N );

			unsigned int psr = 0;
			psr |= N << 31;
			psr |= (r == 0 ?1:0) << 30;
		//	psr |= ((r & 0xFFFFFFFF00000000u) ?1:0) << 29; // Carry does not want to work
			psr |= V << 28;

			modify_register( 16, psr );
			break;
		}
		case instruction::str: {
			auto addr = read_register( ins.op1.reg ) + get_operand_value( ins.op2 );

			if( addr > stack.bottom_address || addr <= ( stack.bottom_address - std::size( stack.data ) ) ) {
				program_state = Program_State_Error;
				program_error_status = "invalid memory write";
				break;
			}

			unsigned int* mem = (unsigned int*)stack.to_real_address( addr );

			*mem = read_register(ins.op0.reg);
			break;
		}
		case instruction::ldr: {
			switch( ins.op1.id ) {
			case operand::label: {
				auto label = std::string( ins.op1.label_name.data, ins.op1.label_name.count );
				int i = 0;
				for( auto&& data_label : p.string_labels ) {
					if( data_label == label ) break;
					i++;
				}
				if( i >= p.string_labels.size() ) {
					program_state = Program_State_Error;
					program_error_status = "cannot find data label";
					break;
				}

				modify_register( ins.op0.reg, i + 10'000 );
				break;
			}
			case operand::reg: {
				auto addr = read_register(ins.op1.reg) + get_operand_value(ins.op2);

				if( addr > stack.bottom_address || addr <= (stack.bottom_address - std::size(stack.data)) ) {
					program_state = Program_State_Error;
					program_error_status = "invalid memory access";
					break;
				}

				unsigned int* mem = (unsigned int*)stack.to_real_address(addr);

				modify_register( ins.op0.reg, *mem );
				break;
			}
			default: {
				program_state = Program_State_Error;
				program_error_status = "internal error: operand 1 is incorrect for ldr";
				break;
			}
			}
			break;
		}
		case instruction::push: {
			stack.push( read_register(ins.op0.reg) );
			break;
		}
		case instruction::pop: {
			modify_register(ins.op0.reg, stack.pop());
			break;
		}
		case instruction::mov: {
			auto value = get_operand_value(ins.op2);
			modify_register(ins.op0.reg, value);
			break;
		}
		case instruction::add: {
			auto a = read_register(ins.op1.reg);
			auto b = get_operand_value(ins.op2);
			modify_register(ins.op0.reg, a + b);
			break;
		}
		case instruction::sub: {
			auto a = read_register(ins.op1.reg);
			auto b = get_operand_value(ins.op2);
			modify_register( ins.op0.reg, a - b );
			break;
		}
		case instruction::mul: {
			auto a = read_register( ins.op1.reg );
			auto b = read_register( ins.op2.reg );
			modify_register( ins.op0.reg, a * b );
			break;
		}
		case instruction::sdiv: {
			auto a = read_register( ins.op1.reg );
			auto b = read_register( ins.op2.reg );
			modify_register( ins.op0.reg, a / b );
			break;
		}
		case instruction::udiv: {
			auto a = (unsigned)read_register( ins.op1.reg );
			auto b = (unsigned)read_register( ins.op2.reg );
			modify_register( ins.op0.reg, a / b );
			break;
		}
		default: {
			program_state = Program_State_Error;
			program_error_status = "instruction has not been implemented";
			break;
		}
		}
	}
}