/*=============================================================================
	PSPClient.cpp: PSP client/viewport stubs.
=============================================================================*/

#include "Engine.h"
#include "PSPClient.h"
#include "PSPDrv.h"

IMPLEMENT_CLASS(UPSPClient);
IMPLEMENT_CLASS(UPSPViewport);

void UPSPClient::InternalClassInitializer( UClass* Class )
{
	guard(UPSPClient::InternalClassInitializer);
	if( appStricmp( Class->GetName(), "PSPClient" ) == 0 )
	{
		new(Class,"UseJoystick",  RF_Public)UBoolProperty( CPP_PROPERTY(UseJoystick),  "Joystick", CPF_Config );
		new(Class,"DeadZoneXYZ",  RF_Public)UFloatProperty( CPP_PROPERTY(DeadZoneXYZ), "Joystick", CPF_Config );
		new(Class,"DeadZoneRUV",  RF_Public)UFloatProperty( CPP_PROPERTY(DeadZoneRUV), "Joystick", CPF_Config );
		new(Class,"ScaleXYZ",     RF_Public)UFloatProperty( CPP_PROPERTY(ScaleXYZ),    "Joystick", CPF_Config );
		new(Class,"ScaleRUV",     RF_Public)UFloatProperty( CPP_PROPERTY(ScaleRUV),    "Joystick", CPF_Config );
		new(Class,"InvertY",      RF_Public)UBoolProperty(  CPP_PROPERTY(InvertY),     "Joystick", CPF_Config );
		new(Class,"InvertV",      RF_Public)UBoolProperty(  CPP_PROPERTY(InvertV),     "Joystick", CPF_Config );
	}
	unguard;
}

UPSPClient::UPSPClient()
{
	guard(UPSPClient::UPSPClient);
	UseJoystick  = 1;
	InvertY      = 0;
	InvertV      = 0;
	ScaleXYZ     = 200.0f;
	ScaleRUV     = 500.0f;
	DeadZoneXYZ  = 0.10f;
	DeadZoneRUV  = 0.10f;
	unguard;
}

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
, LastDeltaSeconds( 0.0f )
{
        // Default size/format for PSP.
        SizeX = 480;
        SizeY = 272;
        ColorBytes = 4;
        Stride = SizeX;
        ScreenPointer = nullptr;
        appMemset( &PrevPad, 0, sizeof( PrevPad ) );
        PrevPad.Lx = PrevPad.Ly = 128;
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
        guard(UPSPViewport::ReadInput);

        LastDeltaSeconds = DeltaSeconds;

        // Poll PSP controller state and dispatch input events.
        UpdateInput( 0 );

        // Run the engine input processing (hold states, key bindings, etc.).
        if( Input )
        {
                Input->ReadInput( DeltaSeconds, GSystem );
        }

        unguard;
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
	(void)ParentWindow;
	(void)Temporary;
	(void)OpenX;
	(void)OpenY;

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
        guard(UPSPViewport::UpdateInput);

        if( !Client || !Client->UseJoystick )
        {
                appMemset( &PrevPad, 0, sizeof( PrevPad ) );
                PrevPad.Lx = PrevPad.Ly = 128;
                LastDeltaSeconds = 0.0f;
                return;
        }

        if( Reset )
        {
                appMemset( &PrevPad, 0, sizeof( PrevPad ) );
                PrevPad.Lx = PrevPad.Ly = 128;
                LastDeltaSeconds = 0.0f;
                return;
        }

        SceCtrlData Pad;
        // Prefer read (blocks until fresh state), fall back to peek if it fails.
        if( sceCtrlReadBufferPositive( &Pad, 1 ) <= 0 )
        {
                if( sceCtrlPeekBufferPositive( &Pad, 1 ) <= 0 )
                        return;
        }

        // Button mapping between the PSP pad and Unreal's joystick keys.
        static const struct
        {
                DWORD Button;
                EInputKey Key;
        } ButtonMap[] =
        {
                { PSP_CTRL_CROSS,     IK_Joy1 },
                { PSP_CTRL_CIRCLE,    IK_Joy2 },
                { PSP_CTRL_SQUARE,    IK_Joy3 },
                { PSP_CTRL_TRIANGLE,  IK_Joy4 },
                { PSP_CTRL_LTRIGGER,  IK_Joy5 },
                { PSP_CTRL_RTRIGGER,  IK_Joy6 },
                { PSP_CTRL_START,     IK_Joy7 },
                { PSP_CTRL_SELECT,    IK_Joy8 },
                { PSP_CTRL_UP,        IK_JoyPovUp },
                { PSP_CTRL_DOWN,      IK_JoyPovDown },
                { PSP_CTRL_LEFT,      IK_JoyPovLeft },
                { PSP_CTRL_RIGHT,     IK_JoyPovRight },
        };

        const DWORD Pressed  = Pad.Buttons & ~PrevPad.Buttons;
        const DWORD Released = PrevPad.Buttons & ~Pad.Buttons;

        // Debug helper: log any change in buttons or axes to verify the pad is being sampled.
        const UBOOL bLogInput = (Pressed | Released) != 0;

        // Map pad buttons to both joystick keys (for config-driven binds) and keyboard keys (fallback for menus/prompts).
        struct FPadBinding
        {
                DWORD Button;
                EInputKey JoyKey;
                EInputKey FallbackKey;
        };

        const FPadBinding PadBindings[] =
        {
                { PSP_CTRL_CROSS,    IK_Joy1,        IK_Space },        // Jump
                { PSP_CTRL_CIRCLE,   IK_Joy2,        IK_RightMouse },   // Alt fire
                { PSP_CTRL_SQUARE,   IK_Joy3,        IK_Ctrl },         // Duck
                { PSP_CTRL_TRIANGLE, IK_Joy4,        IK_MouseWheelUp }, // Next weapon
                { PSP_CTRL_LTRIGGER, IK_Joy5,        IK_MouseWheelDown },// Prev weapon
                { PSP_CTRL_RTRIGGER, IK_Joy6,        IK_LeftMouse },    // Fire
                { PSP_CTRL_START,    IK_Joy7,        IK_Escape },       // Menu / exit prompts
                { PSP_CTRL_SELECT,   IK_Joy8,        IK_Enter },        // Inventory / confirm
                { PSP_CTRL_UP,       IK_JoyPovUp,    IK_Up },
                { PSP_CTRL_DOWN,     IK_JoyPovDown,  IK_Down },
                { PSP_CTRL_LEFT,     IK_JoyPovLeft,  IK_Left },
                { PSP_CTRL_RIGHT,    IK_JoyPovRight, IK_Right },
        };

        for( INT i=0; i<ARRAY_COUNT(PadBindings); ++i )
        {
                const DWORD Mask = PadBindings[i].Button;
                const EInputKey JoyKey = PadBindings[i].JoyKey;
                const EInputKey FallbackKey = PadBindings[i].FallbackKey;

                if( Pressed & Mask )
                {
                        if( JoyKey != IK_None )       CauseInputEvent( JoyKey, IST_Press );
                        if( FallbackKey != IK_None )  CauseInputEvent( FallbackKey, IST_Press );

                        if( Mask == PSP_CTRL_START && Client && Client->Engine )
                        {
                                // Force menu toggle in case bindings/config are missing.
                                Client->Engine->Exec( "SHOWMENU", GSystem );

                                // Also call the PlayerPawn ShowMenu exec directly to bypass input binding issues.
                                if( Actor )
                                {
                                        if( APlayerPawn* Pawn = Cast<APlayerPawn>( Actor ) )
                                        {
                                                static const FName ShowMenuName( "ShowMenu" );
                                                if( UFunction* Fn = Pawn->FindFunction( ShowMenuName ) )
                                                {
                                                        Pawn->ProcessEvent( Fn, nullptr );
                                                }
                                                else
                                                {
                                                        Pawn->bShowMenu = 1;
                                                }
                                        }
                                }
                        }
                }
                if( Released & Mask )
                {
                        if( JoyKey != IK_None )       CauseInputEvent( JoyKey, IST_Release );
                        if( FallbackKey != IK_None )  CauseInputEvent( FallbackKey, IST_Release );
                }
        }

        // Convert the analog nub to joystick axes.
        const INT RawX = (INT)Pad.Lx - 128;
        const INT RawY = (INT)Pad.Ly - 128;

        const INT DeadZoneThreshold = Clamp<INT>( (INT)( Client->DeadZoneXYZ * 127.0f ), 0, 127 );
        const FLOAT NormX = ( Abs(RawX) > DeadZoneThreshold ) ? ( RawX / 127.0f ) : 0.0f;
        const FLOAT NormY = ( Abs(RawY) > DeadZoneThreshold ) ? ( RawY / 127.0f ) : 0.0f;

        if( NormX || PrevPad.Lx != 128 )
                CauseInputEvent( IK_JoyX, IST_Axis, Client->ScaleXYZ * NormX );

        const FLOAT AxisYScale = Client->InvertY ? 1.0f : -1.0f;
        if( NormY || PrevPad.Ly != 128 )
                CauseInputEvent( IK_JoyY, IST_Axis, Client->ScaleXYZ * AxisYScale * NormY );

        if( bLogInput || NormX || NormY )
        {
                debugf( NAME_Log, "PSP pad Buttons=%08X Press=%08X Release=%08X Lx=%u Ly=%u NX=%f NY=%f",
                        Pad.Buttons, Pressed, Released, Pad.Lx, Pad.Ly, NormX, NormY );
        }

        PrevPad = Pad;

        unguard;
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

UBOOL UPSPViewport::CauseInputEvent( INT Key, EInputAction Action, FLOAT Delta )
{
        guard(UPSPViewport::CauseInputEvent);
        if( Key >= 0 && Key < IK_MAX && Client && Client->Engine )
                return Client->Engine->InputEvent( this, (EInputKey)Key, Action, Delta );
        return 0;
        unguard;
}
