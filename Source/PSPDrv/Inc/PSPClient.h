/*=============================================================================
	PSPClient.h: PSP client/viewport stubs.
=============================================================================*/

#pragma once

#include "Engine.h"
#include <pspctrl.h>

class UPSPViewport;

class UPSPClient : public UClient, public FNotifyHook
{
	DECLARE_CLASS_WITHOUT_CONSTRUCT(UPSPClient, UClient, CLASS_Transient|CLASS_Config)

public:
	UPSPClient();
	static void InternalClassInitializer( UClass* Class );

	// Configuration.
	UBOOL UseJoystick;
	UBOOL InvertY;
	UBOOL InvertV;
	FLOAT ScaleXYZ;
	FLOAT ScaleRUV;
	FLOAT DeadZoneXYZ;
	FLOAT DeadZoneRUV;

private:
	UViewport* CurrentView;

public:
	// UClient interface.
	void Init( UEngine* InEngine ) override;
	void Flush() override;
	void ShowViewportWindows( DWORD ShowFlags, int DoShow ) override;
	void EnableViewportWindows( DWORD ShowFlags, int DoEnable ) override;
	void Poll() override;
	UViewport* CurrentViewport() override;
	void Tick() override;
	UBOOL Exec( const char* Cmd, FOutputDevice* Out=GSystem ) override;
	UViewport* NewViewport( ULevel* InLevel, const FName Name ) override;
	void EndFullscreen() override;
};

class UPSPViewport : public UViewport
{
	DECLARE_CLASS_WITHOUT_CONSTRUCT(UPSPViewport,UViewport,CLASS_Transient)
	NO_DEFAULT_CONSTRUCTOR(UPSPViewport)

public:
	// Constructor.
	UPSPViewport( ULevel* InLevel, UClient* InClient );

	// UObject interface.
	void Destroy() override;

	// UViewport interface.
	void ReadInput( FLOAT DeltaSeconds ) override;
	void WriteBinary( const void* Data, INT Length, EName MsgType=NAME_Log ) override;
	void UpdateWindow() override;
	void OpenWindow( void* ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY ) override;
	void CloseWindow() override;
	void UpdateInput( UBOOL Reset ) override;
	void MakeCurrent() override;
	void SetModeCursor() override;
	void* GetWindow() override;
	void SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL FocusOnly=0 ) override;
	void Repaint() override;
	void MakeFullscreen( INT NewX, INT NewY, UBOOL SaveConfig ) override;
	UBOOL Exec( const char* Cmd, FOutputDevice* Out=GSystem ) override;

private:
        UPSPClient* Client;

        // Cached controller state so we can generate press/release events.
        SceCtrlData PrevPad;

        // Time delta from the last read, used for axis scaling.
        FLOAT LastDeltaSeconds;

        UBOOL CauseInputEvent( INT Key, EInputAction Action, FLOAT Delta=0.0f );
};
