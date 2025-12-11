/*=============================================================================
	PSPClient.cpp: PSP client/viewport stubs.
=============================================================================*/

#include "Engine.h"
#include "PSPClient.h"
#include "PSPDrv.h"

IMPLEMENT_CLASS(UPSPClient);
IMPLEMENT_CLASS(UPSPViewport);

//
// Spawn or replace the current render device for a viewport.
//
static void PSPTryRenderDevice( UViewport* Viewport, const char* ClassName )
{
	guard(PSPTryRenderDevice);

	if( !Viewport )
		return;

	// Tear down previous device if present.
	if( Viewport->RenDev )
	{
		Viewport->RenDev->Exit();
		delete Viewport->RenDev;
		Viewport->RenDev = nullptr;
	}

	UClass* RenderClass = GObj.LoadClass( URenderDevice::StaticClass, NULL, ClassName, NULL, LOAD_KeepImports, NULL );
	if( !RenderClass )
	{
		debugf( NAME_Critical, "PSPClient: Unable to find render class '%s'", ClassName );
		return;
	}

	URenderDevice* Device = ConstructClassObject<URenderDevice>( RenderClass );
	if( Device && Device->Init( Viewport ) )
	{
		Viewport->RenDev = Device;
		debugf( NAME_Init, "PSPClient: Render device %s initialized", ClassName );
	}
	else
	{
		debugf( NAME_Critical, "PSPClient: Failed to initialize render device '%s'", ClassName );
		if( Device )
		{
			Device->Exit();
			delete Device;
		}
	}

	unguard;
}

// UPSPClient

void UPSPClient::Init( UEngine* InEngine )
{
	guard(UPSPClient::Init);
	UClient::Init( InEngine );
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

	// Find the best realtime viewport and repaint it.
	UPSPViewport* BestViewport = nullptr;
	for( INT i=0; i<Viewports.Num(); ++i )
	{
		UPSPViewport* Viewport = CastChecked<UPSPViewport>( Viewports(i) );
		if( Viewport->IsRealtime()
			&& Viewport->SizeX && Viewport->SizeY
			&& !Viewport->OnHold
			&& Viewport->RenDev )
		{
			if( !BestViewport || Viewport->LastUpdateTime < BestViewport->LastUpdateTime )
			{
				BestViewport = Viewport;
			}
		}
	}

	if( BestViewport )
	{
		BestViewport->Repaint();
	}

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
 , Client( CastChecked<UPSPClient>( InClient ) )
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

	// Attempt to create the configured render device.
	PSPTryRenderDevice( this, "ini:Engine.Engine.GameRenderDevice" );
	if( !RenDev )
	{
		// Fall back to the classic software renderer if PSPDrv failed.
		PSPTryRenderDevice( this, "SoftDrv.SoftwareRenderDevice" );
	}
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
	guard(UPSPViewport::Repaint);

	if( !OnHold && RenDev && SizeX && SizeY && Client && Client->Engine )
	{
		Client->Engine->Draw( this, 0 );
	}

	unguard;
}

void UPSPViewport::MakeFullscreen( INT NewX, INT NewY, UBOOL SaveConfig )
{
	OpenWindow( nullptr, 0, NewX, NewY, NewX, NewY );
}

UBOOL UPSPViewport::Exec( const char* Cmd, FOutputDevice* Out )
{
	return 0;
}
