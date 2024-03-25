#include <Fission/Platform/EntryPoint.h>
#include <Fission/Core/Monitor.hh>
#include <Fission/Base/Time.hpp>
#include <Fission/Simple2DLayer.h>
#include <Fission/Core/Console.hh>
#include <fstream>
#include <shobjidl.h> 

#include "arm_simulator.hpp"

#pragma comment(lib, "user32")
#pragma comment(lib, "Ole32.lib")

#define _neutron_key_primary_mouse   Fission::Keys::Mouse_Left
#define _neutron_key_secondary_mouse Fission::Keys::Mouse_Right

#define _neutron_char_type    char32_t
#define _neutron_key_type     Fission::Keys::Key
#define _neutron_cursor_type  Fission::Cursor*

#define _neutron_point_type   Fission::v2i32
#define _neutron_rect_type    Fission::ri32
#define _neutron_vector_type  std::vector
#include <Fission/neutron.hpp>

template <typename T>
struct DefaultDelete : public T { virtual void Destroy() override { delete this; } };

using namespace Fission::base;
using Fission::v2f32, Fission::rf32;

static Fission::IFRenderer2D * g_r2d;

static Program program;
static std::string err_string = "no error";
static std::string input_file = "main.s";

static HWND hwnd = NULL;

static Fission::string source_code;
static bool compile_error = false;

namespace Fission {
	template <typename T>
	struct SetValue {
		T* value;
		string operator()( string const& in ) {
			try { *value = std::stof( in.str() ); }
			catch( ... ) {}
			Fission::Console::WriteLine( cat( "Set Value to: ", std::to_string( *value ) ) );
			return {};
		}
	};
}

class Button : public neutron::Button
{
public:
	v2f32 pos, size;
	std::string label;
	std::function<void()> action;

	Button(const char * label, v2f32 pos, v2f32 size, std::function<void()> action):
		pos(pos),size(size),label(label), action(action) {
	//	Fission::Console::RegisterCommand( "set:x", Fission::SetValue{&this->pos.x} );
	}

	virtual bool isInside( neutron::point pos ) override
	{
		rf32 rc = rf32::from_topleft( this->pos, size );
		return rc[(v2f32)pos];
	}

	virtual neutron::Result OnSetCursor( neutron::SetCursorEventArgs & args ) override
	{
		args.cursor = Fission::Cursor::Get( Fission::Cursor::Default_Hand );
		return neutron::Handled;
	}

	virtual void OnUpdate(float) override
	{
		auto rect = rf32{ pos.x, pos.x + size.x, pos.y, pos.y + size.y };

		if( parent->GetHover() == this )
		g_r2d->FillRoundRect( rect, 5.0f, Fission::colors::CadetBlue );
		else
		g_r2d->FillRoundRect( rect, 5.0f, Fission::color(Fission::colors::Gray,0.5f) );

		g_r2d->DrawRoundRect( rect, 5.0f, Fission::color(Fission::colors::White,0.5f), 0.5f );

		auto tl = g_r2d->CreateTextLayout( label.c_str() );
		auto start = v2f32{ ( rect.x.distance() - tl.width ) * 0.5f,( rect.y.distance() - tl.height ) * 0.5f } + pos;
		g_r2d->DrawString( label.c_str(), start, Fission::colors::White );
	}

	virtual void OnPressed() override {
		action();
	}
};

void print_value( unsigned int value, Fission::v2f32 pos, std::optional<Fission::color> c = {} ) {
	unsigned int exp = 1'000'000'000;
	unsigned int len = 0;
	unsigned int digit = 0;
	while( exp != 0 ) {
		digit = (value / exp) % 10;
		exp /= 10;
		if( digit != 0 ) break;
		len++;
	}
	
	{
		char zeros[] = "0000000000";
		auto tl = g_r2d->DrawString( Fission::string_view(zeros,len), pos, c.value_or(Fission::colors::Gray) );
		pos.x += tl.width;
	}

	if( len != 10 ) {
		char buffer[10];
		len = 0;
		buffer[len++] = '0' + digit;
		while( exp != 0 ) {
			digit = (value / exp) % 10;
			exp /= 10;
			buffer[len++] = '0' + digit;
		}
		g_r2d->DrawString( Fission::string_view(buffer,len), pos, c.value_or(Fission::colors::White) );
	}
}

static constexpr auto s_code_rect     = rf32{70.0f, 500.0f, 40.0f, 700.0f};
static constexpr auto s_registers_pos = v2f32{520.0f, 40.0f};

static constexpr auto s_stack_addr_width = 80.0f;
static constexpr auto s_stack_rect    = rf32{740.0f + s_stack_addr_width, 740.0f + s_stack_addr_width + 90.0f, 40.0f, 40.0f + 20.0f * 32.0f + 10.0f };

static void reset_console() {
	FreeConsole();
	AllocConsole();
	auto f = freopen( "CONOUT$", "w", stdout );
	f = freopen( "CONIN$", "r", stdin );
	SetConsoleTitleA( "Program Console Output" );
	SetForegroundWindow( hwnd );
}

class MainLayer : public DefaultDelete<Fission::Simple2DLayer>
{
public:
	virtual void OnCreate( Fission::FApplication * app ) override
	{
		Simple2DLayer::OnCreate(app);
		wnd = app->f_pMainWindow;
		font = Fission::FontManager::GetFont("$console");
		g_r2d = m_pRenderer2D;

		{
			auto[x,y] = wnd->GetSize();
			wm.Initialize( x, y );
		}

		wm.addWindow( new Button( "Load File", { s_code_rect.x.high + 20.0f,400.0f }, { 120.0f, 24.0f }, [this] { if(open_file()) return; load_file(); } ) );
		wm.addWindow( new Button( "Reload File", { s_code_rect.x.high + 20.0f,440.0f }, { 120.0f, 24.0f }, [this] { load_file(); } ) );
		wm.addWindow( new Button( "Reset [R]", { s_code_rect.x.high + 20.0f,480.0f }, { 120.0f, 24.0f }, [this] { reset(); } ) );
		wm.addWindow( new Button( "Step [N]", { s_code_rect.x.high + 20.0f,520.0f }, { 120.0f, 24.0f }, [this] { step(); } ) );
		
		load_file();
	}

	virtual void OnUpdate( Fission::timestep dt ) override
	{
		using namespace Fission;

		float h = font->GetSize() + 2.0f;

		m_pRenderer2D->SelectFont( font );
		wm.OnUpdate( 0.0f );
		g_r2d->Render();

		Fission::TextLayout tl;

		m_pRenderer2D->DrawString( "Registers", s_registers_pos - v2f32{0.0f,h+2.0f}, colors::LawnGreen );
		m_pRenderer2D->DrawRect( rf32::from_topleft( s_registers_pos, 200.0f, 330.0f ).expanded(1.0f),
			colors::LawnGreen, 2.0f, StrokeStyle::Outside);

		float y = s_registers_pos.y;
		for( auto&& reg : arm::regs ) {
			if( arm::reg_modified_bits & (1 << reg.index) ) {
				print_value( reg.value, { s_registers_pos.x + 2.0f, y }, std::make_optional( color{1.0f, 0.4f, 0.4f} ) );
				tl = m_pRenderer2D->DrawString( reg.name.c_str(), {s_registers_pos.x + 110.0f, y}, color{1.0f, 0.5f, 0.5f});
			} else {
				print_value( reg.value, { s_registers_pos.x + 2.0f, y } );
				tl = m_pRenderer2D->DrawString( reg.name.c_str(), { s_registers_pos.x + 110.0f, y }, colors::LightGreen );
			}
			y += tl.height;
			if( reg.index == 3 || reg.index == 10 ) {
				y += 10.0f;
			}
		}


		constexpr v2f32 stack_pos = s_stack_rect.topLeft();
		m_pRenderer2D->DrawString( "Stack", stack_pos - v2f32{0.0f, h+2.0f}, colors::DodgerBlue );
		y = 40.0f;
		{
			char buffer[16];
			int address = arm::stack.bottom_address - 35 * 4;
			for( int i = 0; i < 36; ++i ) {
				auto size = sprintf( buffer, "%d", address );


				tl = m_pRenderer2D->CreateTextLayout( buffer, size );
				float x_left = stack_pos.x - tl.width - 10.0f;
				m_pRenderer2D->DrawString( string_view(buffer,size), {x_left, y}, colors::LightGray);

				if( arm::SP == address )
					m_pRenderer2D->DrawString( "sp =", { x_left - 40.0f, y }, colors::White );

				size = sprintf( buffer, "%11d", arm::stack.data[36-i-1] );
				tl = m_pRenderer2D->DrawString( string_view(buffer,size), {stack_pos.x, y}, colors::White);

				y += tl.height;
				address += 4;
			}
			m_pRenderer2D->DrawRect( s_stack_rect.expanded(2.0f), colors::DodgerBlue, 2.0f, StrokeStyle::Outside);
			{
				auto x = stack_pos.x - 30.0f;
				m_pRenderer2D->FillTriangle( { x - 4.0f, 38.0f }, { x + 4.0f, 38.0f }, { x, 33.0f }, colors::LightGray );
			}
		}


		constexpr v2f32 pos = s_code_rect.topLeft();
		m_pRenderer2D->DrawRect( s_code_rect.expanded(2.0f), colors::Gray, 2.0f, StrokeStyle::Outside );
		m_pRenderer2D->DrawString( "Assembly", pos + v2f32{0.0f,-h-2.0f}, colors::Gray);

		{
			char buffer[128];
			sprintf( buffer, "(%s)", input_file.c_str() );
			m_pRenderer2D->DrawString( buffer, pos + v2f32{ 100.0f,-h - 2.0f }, colors::White );

			sprintf( buffer, "status: (%s) line %i", arm::program_error_status, parser_error_line_number );
			m_pRenderer2D->DrawString( buffer, v2f32{ s_code_rect.x.low, s_code_rect.y.high + 2.0f },
				(arm::program_error_status == "program exited successfully"? colors::LimeGreen:colors::White) );
		}

		{

			float y = pos.y;
			{
				char buffer[16];
				float x = pos.x - 35.0f;
				for( int yy = 0;; yy++ ) {
					auto size = sprintf( buffer, "%03d", (int)yy + 1 );
					auto y = pos.y + (float)yy * h;
					m_pRenderer2D->DrawString(string_view(buffer,size), {x, y}, colors::Gray);
					if( yy == arm::PC - 1000 ) {
						m_pRenderer2D->FillArrow( { 10.0f, y - 10.0f }, { 30.0f, y - 10.0f }, 8.0f, colors::White );
					}
					if( y >= s_code_rect.y.high - 20.0f ) break;
				}
			}

			std::string line;
			line.reserve(64);

			auto print_line = [&]( const char* start ) {
				int len = 0;
				while( start[len] != '\n' && start[len] != '\0' ) len++;

				line.clear();
				for( auto&& c: std::string_view(start,len) ) {
					if( c == '\t' ) {
						static constexpr auto tabsize = 6;
						int nl = tabsize - line.size() % tabsize;
						for( int i = 0; i < nl; ++i ) line.push_back(' ');
					} else line.push_back(c);
				}

				m_pRenderer2D->DrawString( line.c_str(), {pos.x,y}, colors::White);
				y += h;

				return start + len + 1;
			};

			const char* start = source_code.c_str();
			const char* end = source_code.c_str() + source_code.length();
			while( start < end ) start = print_line(start);

		}

		m_pRenderer2D->Render();
	}

	void step() {
		if( arm::program_state == arm::Program_State_Good ) {
			if( arm::PC == arm::return_address ) {
				arm::program_state = arm::Program_State_Error;
				arm::program_error_status = "program exited successfully";
				arm::stack.check();
			} else
			arm::execute_next_instruction(program);
		}
	}

	void reset() {
		if( compile_error == false ) {
			arm::reset(program.instructions[arm::main_instruction_index].line_number);
			arm::program_state = arm::Program_State_Good;
			arm::program_error_status = "Good";

			reset_console();
		}
	}

	bool open_file() {
		bool failed = true;
		IFileOpenDialog* pFileOpen;

		// Create the FileOpenDialog object.
		HRESULT hr = CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>( &pFileOpen ) );

		if(SUCCEEDED( hr ))
		{
			// Show the Open dialog box.
			hr = pFileOpen->Show( NULL );

			// Get the file name from the dialog box.
			if(SUCCEEDED( hr ))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult( &pItem );
				if(SUCCEEDED( hr ))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath );

					// Display the file name to the user.
					if(SUCCEEDED( hr ))
					{
						using namespace Fission;
						auto path = std::filesystem::path(pszFilePath);
						auto rel = std::filesystem::relative( path );
						input_file = rel.string();
						CoTaskMemFree( pszFilePath );
						failed = false;
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		return failed;
	}
	void load_file() {
		if(auto f = std::ifstream( input_file )) {
			// https://stackoverflow.com/questions/2912520/read-file-contents-into-a-string-in-c
			std::string content( (std::istreambuf_iterator<char>(f) ),
								 (std::istreambuf_iterator<char>()) );

			source_code = std::move( content );
			arm::reset(1);
			program.clear();

			arm::program_state = arm::Program_State_Good;
			arm::program_error_status = "Good";
			if(build_program( source_code.c_str(), program )) {
				arm::program_state = arm::Program_State_Error;
				err_string = "compile error: ";
				err_string += parser_error_message;
				arm::program_error_status = err_string.c_str();
				compile_error = true;
			}
			else
				arm::verify_program( program );

			if(arm::program_state == arm::Program_State_Good) {
				arm::PC = program.instructions[arm::main_instruction_index].line_number + 1000;
			}

			reset_console();
		}
		else {
			err_string = std::string("Error: Failed to load file: ") + input_file;
			arm::program_error_status = err_string.c_str();
			return;
		}
	}

	virtual Fission::EventResult OnKeyDown( Fission::KeyDownEventArgs & args ) override
	{
		switch( args.key )
		{
		case Fission::Keys::N: {
			step();
			return Fission::EventResult::Handled;
		}
		case Fission::Keys::R: {
			reset();
			return Fission::EventResult::Handled;
		}
		default:break;
		}
		neutron::KeyDownEventArgs nargs = { args.key };
		return (Fission::EventResult)wm.OnKeyDown( nargs );
	}
	virtual Fission::EventResult OnKeyUp( Fission::KeyUpEventArgs & args ) override
	{
		neutron::KeyUpEventArgs nargs = { args.key };
		return (Fission::EventResult)wm.OnKeyUp( nargs );
	}
	virtual Fission::EventResult OnMouseMove( Fission::MouseMoveEventArgs & args ) override
	{
		neutron::MouseMoveEventArgs nargs = { args.position };
		return (Fission::EventResult)wm.OnMouseMove( nargs );
	}
	virtual Fission::EventResult OnSetCursor( Fission::SetCursorEventArgs & args ) override
	{
		neutron::SetCursorEventArgs nargs = { args.cursor };
		auto r = (Fission::EventResult)wm.OnSetCursor( nargs );
		if( nargs.cursor != args.cursor )
		{
			args.cursor = nargs.cursor;
			args.bUseCursor = true;
		}
		return r;
	}
private:
	Fission::IFWindow * wnd;
	Fission::Font * font;

	neutron::WindowManager wm;
};

class MainScene : public DefaultDelete<Fission::FMultiLayerScene>
{
public:
	MainScene() { PushLayer( new MainLayer ); }

	virtual Fission::SceneKey GetKey() override { return {}; }
};

class MyApp : public Fission::FApplication
{
public:
	MyApp() : FApplication( "ARM Simulator", {0,0,0} ) {}

	virtual void OnStartUp( CreateInfo * info ) override
	{
		info->window.title = u8"ARM Simulator ❔❓❔❓";
		info->window.size = { 950, 750 };
		info->graphics.api = Fission::IFGraphics::API::DirectX11;
		f_VersionInfo = "dev";
	}
	virtual void OnCreate() override {
		hwnd = f_pMainWindow->native_handle();
	}
	virtual Fission::IFScene * OnCreateScene( const Fission::SceneKey& key ) override
	{
		return new MainScene;
	}
	virtual void Destroy() override
	{
		delete this;
	}
};

Fission::FApplication * CreateApplication() {
	return new MyApp;
}