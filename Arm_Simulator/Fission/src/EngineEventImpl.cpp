#pragma once
#include "Engine.h"

namespace Fission {

	EventResult FissionEngine::OnKeyDown( KeyDownEventArgs & args )
	{
		if( m_DebugLayer.OnKeyDown( args ) == EventResult::Handled )
			return EventResult::Handled;

		if( m_ConsoleLayer.OnKeyDown( args ) == EventResult::Handled )
			return EventResult::Handled;

		return m_pCurrentScene->OnKeyDown( args );
	}

	EventResult FissionEngine::OnKeyUp( KeyUpEventArgs & args )
	{
		if( m_DebugLayer.OnKeyUp( args ) == EventResult::Handled )
			return EventResult::Handled;

		if( m_ConsoleLayer.OnKeyUp( args ) == EventResult::Handled )
			return EventResult::Handled;

		return m_pCurrentScene->OnKeyUp( args );
	}

	EventResult FissionEngine::OnTextInput( TextInputEventArgs & args )
	{
		if( m_DebugLayer.OnTextInput( args ) == EventResult::Handled )
			return EventResult::Handled;

		if( m_ConsoleLayer.OnTextInput( args ) == EventResult::Handled )
			return EventResult::Handled;

		return m_pCurrentScene->OnTextInput( args );
	}

	EventResult FissionEngine::OnMouseMove( MouseMoveEventArgs & args )
	{
		if( m_DebugLayer.OnMouseMove( args ) == EventResult::Handled )
			return EventResult::Handled;

		if( m_ConsoleLayer.OnMouseMove( args ) == EventResult::Handled )
			return EventResult::Handled;

		return m_pCurrentScene->OnMouseMove( args );
	}

	EventResult FissionEngine::OnMouseLeave( MouseLeaveEventArgs & args )
	{
		if( m_DebugLayer.OnMouseLeave( args ) == EventResult::Handled )
			return EventResult::Handled;

		if( m_ConsoleLayer.OnMouseLeave( args ) == EventResult::Handled )
			return EventResult::Handled;

		return m_pCurrentScene->OnMouseLeave( args );
	}

	EventResult FissionEngine::OnSetCursor( SetCursorEventArgs & args )
	{
		if( m_DebugLayer.OnSetCursor( args ) == EventResult::Handled )
			return EventResult::Handled;

		if( m_ConsoleLayer.OnSetCursor( args ) == EventResult::Handled )
			return EventResult::Handled;

		return m_pCurrentScene->OnSetCursor( args );
	}

	EventResult FissionEngine::OnHide()
	{
		m_pCurrentScene->OnHide();

		// Pause the Main thread
		m_bMinimized = true;
		return EventResult::Handled;
	}

	EventResult FissionEngine::OnShow()
	{
		m_pCurrentScene->OnShow();

		if( m_bMinimized )
		{
			m_bMinimized = false;
			// Notify Main thread to continue rendering frames
			m_PauseCondition.notify_one();
		}
		return EventResult::Handled;
	}

	EventResult FissionEngine::OnClose( CloseEventArgs & args )
	{
		if( m_bMinimized )
		{
			m_bMinimized = false;
			m_PauseCondition.notify_one();
		}

		m_ExitCode = args.ExitCode;

		m_pCurrentScene->OnClose( args );

		m_bRunning = false;

		return EventResult::Handled;
	}

	EventResult FissionEngine::OnResize( ResizeEventArgs & args )
	{
		m_NewSize = args.size;
		m_bWantResize = true;

		m_DebugLayer.OnResize(args);
		m_ConsoleLayer.OnResize(args);
		m_pCurrentScene->OnResize(args);

		return EventResult::Handled;
	}

}