/*=============================================================================
	PSPClient.cpp: PSP client/viewport stubs.
=============================================================================*/

#include "Engine.h"
#include "PSPClient.h"
#include "PSPDrv.h"

IMPLEMENT_CLASS(UPSPClient);
IMPLEMENT_CLASS(UPSPViewport);

// UPSPClient

void UPSPClient::Init( UEngine* InEngine )
{
	guard(UPSPClient::Init);
	Engine = InEngine;
	CurrentView = nullptr;
	debugf( NAME_Init, "PSPClient: Init" );
	unguard;
}

void UPSPClient::Flush()
{
	guard(UPSPClient::Flush);
	unguard;
}

void UPSPClient::ShowViewportWindows( DWORD ShowFlags, int DoShow )
{
	guard(UPSPClient::ShowViewportWindows);
	unguard;
}

void UPSPClient::EnableViewportWindows( DWORD ShowFlags, int DoEnable )
{
	guard(UPSPClient::EnableViewportWindows);
	unguard;
}

void UPSPClient::Poll()
{
	guard(UPSPClient::Poll);
	unguard;
}

UViewport* UPSPClient::CurrentViewport()
{
	return CurrentView ? CurrentView : (Viewports.Num() ? Viewports(0) : nullptr);
}

void UPSPClient::Tick()
{
	guard(UPSPClient::Tick);
	unguard;
}

UBOOL UPSPClient::Exec( const char* Cmd, FOutputDevice* Out )
{
	guard(UPSPClient::Exec);
	return 0;
	unguard;
}

UViewport* UPSPClient::NewViewport( ULevel* InLevel, const FName Name )
{
	guard(UPSPClient::NewViewport);
	UPSPViewport* View = new(InLevel)UPSPViewport( InLevel, this );
	Viewports.AddItem( View );
	CurrentView = View;
	return View;
	unguard;
}

void UPSPClient::EndFullscreen()
{
	guard(UPSPClient::EndFullscreen);
	unguard;
}

// UPSPViewport

UPSPViewport::UPSPViewport( ULevel* InLevel, UClient* InClient )
: UViewport( InLevel, InClient )
{
	// Default size/format for PSP.
	SizeX = 480;
	SizeY = 272;
	ColorBytes = 4;
	Stride = SizeX;
	ScreenPointer = nullptr;
	debugf( NAME_Init, "PSPViewport: created %dx%d", SizeX, SizeY );
}

void UPSPViewport::Destroy()
{
	guard(UPSPViewport::Destroy);
	UViewport::Destroy();
	unguard;
}

void UPSPViewport::ReadInput( FLOAT DeltaSeconds )
{
	// Stub: no input yet.
}

void UPSPViewport::WriteBinary( const void* Data, INT Length, EName MsgType )
{
	// Minimal logging stub; forward to global log if available.
	debugf( MsgType, "%.*s", Length, (const char*)Data );
}

void UPSPViewport::UpdateWindow()
{
}

void UPSPViewport::OpenWindow( void* ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY )
{
	SizeX = NewX ? NewX : SizeX;
	SizeY = NewY ? NewY : SizeY;
	ColorBytes = 4;
	Stride = SizeX;
	debugf( NAME_Init, "PSPViewport: OpenWindow %dx%d", SizeX, SizeY );
}

void UPSPViewport::CloseWindow()
{
}

void UPSPViewport::UpdateInput( UBOOL Reset )
{
}

void UPSPViewport::MakeCurrent()
{
}

void UPSPViewport::SetModeCursor()
{
}

void* UPSPViewport::GetWindow()
{
	return nullptr;
}

void UPSPViewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL FocusOnly )
{
}

void UPSPViewport::Repaint()
{
}

void UPSPViewport::MakeFullscreen( INT NewX, INT NewY, UBOOL SaveConfig )
{
	OpenWindow( nullptr, 0, NewX, NewY, NewX, NewY );
}

UBOOL UPSPViewport::Exec( const char* Cmd, FOutputDevice* Out )
{
	return 0;
}
