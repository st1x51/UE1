/*=============================================================================
	PSPDrv.cpp: Minimal PSP GU render device.
=============================================================================*/

#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspkernel.h>

#include "Engine.h"
#include "PSPDrv.h"
#include "PSPClient.h"

// Command list buffer.
uint32_t UPSPRenderDevice::CommandList[262144/4] __attribute__((aligned(16)));

IMPLEMENT_CLASS(UPSPRenderDevice);
IMPLEMENT_PACKAGE(PSPDrv);

void UPSPRenderDevice::Destroy()
{
	Exit();
	Super::Destroy();
}

UBOOL UPSPRenderDevice::Init( UViewport* InViewport )
{
	debugf( NAME_Init, "PSPDrv: Init begin" );
	Viewport = InViewport;
	SpanBased = 0;
	FrameBuffered = 1;
	SupportsFogMaps = 0;
	SupportsDistanceFog = 0;
	VolumetricLighting = 0;
	ShinySurfaces = 0;
	Coronas = 0;
	HighDetailActors = 0;
	NoVolumetricBlend = 1;

	sceGuInit();
	sceGuStart( GU_DIRECT, CommandList );
	sceGuDrawBuffer( GU_PSM_8888, 0, 512 );
	sceGuDispBuffer( Viewport->SizeX, Viewport->SizeY, 0, 512 );
	sceGuDepthBuffer( (void*) (0x400000), 512 ); // simple depth buffer in VRAM base
	sceGuDepthRange( 0, 65535 );
	sceGuDisable( GU_SCISSOR_TEST );
	sceGuFinish();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
	sceDisplayWaitVblankStart();
	sceGuDisplay( GU_TRUE );

	Initialized = 1;
	if( Viewport )
		debugf( NAME_Init, "PSPDrv: Init done, viewport %dx%d", Viewport->SizeX, Viewport->SizeY );
	else
		debugf( NAME_Init, "PSPDrv: Init done, no viewport" );
	return 1;
}

void UPSPRenderDevice::Exit()
{
	if( Initialized )
	{
		sceGuTerm();
		Initialized = 0;
		debugf( NAME_Exit, "PSPDrv: Exit" );
	}
}

void UPSPRenderDevice::Flush()
{
	// No-op for now.
}

UBOOL UPSPRenderDevice::Exec( const char* Cmd, FOutputDevice* Out )
{
	return 0;
}

void UPSPRenderDevice::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
	if( !Initialized )
		return;

	sceGuStart( GU_DIRECT, CommandList );
	if( RenderLockFlags & LOCKR_ClearScreen )
	{
		DWORD ClearColor = 0;
		sceGuClearColor( ClearColor );
		sceGuClearDepth( 0 );
		sceGuClear( GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT );
	}
}

void UPSPRenderDevice::Unlock( UBOOL Blit )
{
	if( !Initialized )
		return;

	sceGuFinish();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();
}

void UPSPRenderDevice::DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{
	// TODO: Implement hardware drawing; currently no-op.
}

void UPSPRenderDevice::DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span )
{
	// TODO: Implement hardware drawing; currently no-op.
}

void UPSPRenderDevice::DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )
{
	// TODO: Implement hardware drawing; currently no-op.
}

void UPSPRenderDevice::Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
{
	// TODO: Implement hardware drawing; currently no-op.
}

void UPSPRenderDevice::Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2 )
{
	// TODO: Implement hardware drawing; currently no-op.
}

void UPSPRenderDevice::ClearZ( FSceneNode* Frame )
{
	if( Initialized )
		sceGuClear( GU_DEPTH_BUFFER_BIT );
}

void UPSPRenderDevice::PushHit( const BYTE* Data, INT Count )
{
	// No hit-testing support; ignore.
}

void UPSPRenderDevice::PopHit( INT Count, UBOOL bForce )
{
	// No hit-testing support; ignore.
}

void UPSPRenderDevice::GetStats( char* Result )
{
	appStrcpy( Result, "PSP GU (stub)" );
}

void UPSPRenderDevice::ReadPixels( FColor* Pixels )
{
	// Not implemented; leave blank.
}

void UPSPRenderDevice::EndFlash()
{
}
