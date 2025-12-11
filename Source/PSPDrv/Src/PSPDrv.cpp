/*=============================================================================
	PSPDrv.cpp: Minimal PSP GU render device.
=============================================================================*/

#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspkernel.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include "Engine.h"
#include "PSPDrv.h"
#include "PSPClient.h"

enum
{
	PSP_BUF_WIDTH = 512
};

// Command list buffer.
uint32_t UPSPRenderDevice::CommandList[262144/4] __attribute__((aligned(16)));

namespace
{
	static uint8_t* GVramBase = nullptr;
	static size_t GVramSize = 0;
	static size_t GVramOffset = 0;
	static void* GFrameBuffers[2] = { nullptr, nullptr };
	static int GDrawBufferIndex = 0; // Index into GFrameBuffers of the buffer we render into this frame.
	static void* GDrawBuffer = nullptr; // Current draw buffer (swapped each frame).
	static void* GDispBuffer = nullptr;
	static void* GDepthBuffer = nullptr;

	static inline unsigned int GetPixelSize( int Psm )
	{
		switch( Psm )
		{
		case GU_PSM_5650:
		case GU_PSM_5551:
		case GU_PSM_4444:
		case GU_PSM_T16:
			return 2;
		default:
			return 4;
		}
	}

	static void InitVramAllocator()
	{
		if( !GVramBase )
		{
			GVramBase = static_cast<uint8_t*>( sceGeEdramGetAddr() );
			GVramSize = sceGeEdramGetSize();
		}
		GVramOffset = 0;
	GDrawBuffer = nullptr;
	GDispBuffer = nullptr;
	GDepthBuffer = nullptr;
	GFrameBuffers[0] = GFrameBuffers[1] = nullptr;
	GDrawBufferIndex = 0;
}

	static void* AllocVram( size_t Size )
	{
		Size = ( Size + 0x3F ) & ~0x3F;
		if( !GVramBase )
		{
			InitVramAllocator();
		}
		if( GVramOffset + Size > GVramSize )
			return nullptr;
		void* Result = GVramBase + GVramOffset;
		GVramOffset += Size;
		return Result;
	}

	static void* AllocVramBuffer( int Width, int Height, int Psm )
	{
		return AllocVram( Width * Height * GetPixelSize( Psm ) );
	}

	static inline void* ToGuRelative( void* Ptr )
	{
		return Ptr ? reinterpret_cast<void*>( reinterpret_cast<uint8_t*>( Ptr ) - GVramBase ) : nullptr;
	}

	static void ResetVram()
	{
		GVramOffset = 0;
		GDrawBuffer = nullptr;
		GDispBuffer = nullptr;
		GDepthBuffer = nullptr;
	}
}

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
	SpanBased = 1;
	FrameBuffered = 1;
	SupportsFogMaps = 0;
	SupportsDistanceFog = 0;
	VolumetricLighting = 0;
	ShinySurfaces = 0;
	Coronas = 0;
	HighDetailActors = 0;
	NoVolumetricBlend = 1;

	// Allocate a CPU buffer for the software renderer to draw into.
	SoftwareStride = Viewport->SizeX;
	const size_t BufferBytes = SoftwareStride * Viewport->SizeY * 2;
	SoftwareBuffer = static_cast<BYTE*>( memalign( 64, BufferBytes ) );
	if( !SoftwareBuffer )
	{
		debugf( NAME_Critical, "PSPDrv: Failed to allocate software framebuffer (%zu bytes)", BufferBytes );
		return 0;
	}
	memset( SoftwareBuffer, 0, BufferBytes );
	Viewport->ScreenPointer = SoftwareBuffer;
	Viewport->Stride = SoftwareStride;
	Viewport->ColorBytes = 2;
	Viewport->Caps |= CC_RGB565;

	InitVramAllocator();
	GFrameBuffers[0] = AllocVramBuffer( PSP_BUF_WIDTH, Viewport->SizeY, GU_PSM_5650 );
	GFrameBuffers[1] = AllocVramBuffer( PSP_BUF_WIDTH, Viewport->SizeY, GU_PSM_5650 );
	GDrawBufferIndex = 0;
	GDrawBuffer = GFrameBuffers[GDrawBufferIndex];
	GDispBuffer = GFrameBuffers[1];
	GDepthBuffer = AllocVramBuffer( PSP_BUF_WIDTH, Viewport->SizeY, GU_PSM_4444 );
	if( !GFrameBuffers[0] || !GFrameBuffers[1] || !GDepthBuffer )
	{
		debugf( NAME_Critical, "PSPDrv: Failed to allocate GU buffers (draw=%p disp=%p depth=%p)", GDrawBuffer, GDispBuffer, GDepthBuffer );
		return 0;
	}

	sceGuInit();
	sceGuStart( GU_DIRECT, CommandList );

	sceGuDrawBuffer( GU_PSM_5650, ToGuRelative( GDrawBuffer ), PSP_BUF_WIDTH );
	sceGuDispBuffer( Viewport->SizeX, Viewport->SizeY, ToGuRelative( GDispBuffer ), PSP_BUF_WIDTH );
	sceGuDepthBuffer( ToGuRelative( GDepthBuffer ), PSP_BUF_WIDTH );

	sceGuOffset( 2048 - ( Viewport->SizeX / 2 ), 2048 - ( Viewport->SizeY / 2 ) );
	sceGuViewport( 2048, 2048, Viewport->SizeX, Viewport->SizeY );
	sceGuDepthRange( 0, 65535 );
	sceGuDepthFunc( GU_GEQUAL );
	sceGuEnable( GU_DEPTH_TEST );
	sceGuEnable( GU_SCISSOR_TEST );
	sceGuScissor( 0, 0, Viewport->SizeX, Viewport->SizeY );
	sceGuFrontFace( GU_CW );
	sceGuShadeModel( GU_SMOOTH );
	sceGuEnable( GU_TEXTURE_2D );
	sceGuEnable( GU_CLIP_PLANES );

	sceGuFinish();
	sceGuSync( GU_SYNC_FINISH, GU_SYNC_WHAT_DONE );
	sceDisplayWaitVblankStart();
	sceGuDisplay( GU_TRUE );

	// Create the software renderer which will rasterize into ScreenPointer.
	UClass* SoftClass = GObj.LoadClass( URenderDevice::StaticClass, NULL, "SoftDrv.SoftwareRenderDevice", NULL, LOAD_KeepImports, NULL );
	if( !SoftClass )
	{
		debugf( NAME_Critical, "PSPDrv: SoftDrv.SoftwareRenderDevice not available" );
		return 0;
	}
	SoftwareDevice = ConstructClassObject<URenderDevice>( SoftClass );
	if( !SoftwareDevice || !SoftwareDevice->Init( Viewport ) )
	{
		debugf( NAME_Critical, "PSPDrv: Failed to initialize SoftDrv renderer" );
		if( SoftwareDevice )
		{
			delete SoftwareDevice;
			SoftwareDevice = nullptr;
		}
		return 0;
	}

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
		ResetVram();
		Initialized = 0;
		debugf( NAME_Exit, "PSPDrv: Exit" );
	}

	if( SoftwareDevice )
	{
		SoftwareDevice->Exit();
		delete SoftwareDevice;
		SoftwareDevice = nullptr;
	}

	if( SoftwareBuffer )
	{
		free( SoftwareBuffer );
		SoftwareBuffer = nullptr;
		Viewport->ScreenPointer = nullptr;
	}
}

void UPSPRenderDevice::Flush()
{
	if( SoftwareDevice )
		SoftwareDevice->Flush();
}

UBOOL UPSPRenderDevice::Exec( const char* Cmd, FOutputDevice* Out )
{
	return SoftwareDevice ? SoftwareDevice->Exec( Cmd, Out ) : 0;
}

void UPSPRenderDevice::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
	if( SoftwareDevice )
		SoftwareDevice->Lock( FlashScale, FlashFog, ScreenClear, RenderLockFlags, HitData, HitSize );
}

void UPSPRenderDevice::Unlock( UBOOL Blit )
{
	if( SoftwareDevice )
		SoftwareDevice->Unlock( Blit );
	BlitSoftwareBuffer();
}

void UPSPRenderDevice::DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{
	if( SoftwareDevice )
		SoftwareDevice->DrawComplexSurface( Frame, Surface, Facet );
}

void UPSPRenderDevice::DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span )
{
	if( SoftwareDevice )
		SoftwareDevice->DrawGouraudPolygon( Frame, Info, Pts, NumPts, PolyFlags, Span );
}

void UPSPRenderDevice::DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )
{
	if( SoftwareDevice )
		SoftwareDevice->DrawTile( Frame, Info, X, Y, XL, YL, U, V, UL, VL, Span, Z, Color, Fog, PolyFlags );
}

void UPSPRenderDevice::Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
{
	if( SoftwareDevice )
		SoftwareDevice->Draw2DLine( Frame, Color, LineFlags, P1, P2 );
}

void UPSPRenderDevice::Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2 )
{
	if( SoftwareDevice )
		SoftwareDevice->Draw2DPoint( Frame, Color, LineFlags, X1, Y1, X2, Y2 );
}

void UPSPRenderDevice::ClearZ( FSceneNode* Frame )
{
	if( SoftwareDevice )
		SoftwareDevice->ClearZ( Frame );
}

void UPSPRenderDevice::PushHit( const BYTE* Data, INT Count )
{
	if( SoftwareDevice )
		SoftwareDevice->PushHit( Data, Count );
}

void UPSPRenderDevice::PopHit( INT Count, UBOOL bForce )
{
	if( SoftwareDevice )
		SoftwareDevice->PopHit( Count, bForce );
}

void UPSPRenderDevice::GetStats( char* Result )
{
	if( SoftwareDevice )
		SoftwareDevice->GetStats( Result );
	else
		Result[0] = 0;
}

void UPSPRenderDevice::ReadPixels( FColor* Pixels )
{
	if( SoftwareDevice )
		SoftwareDevice->ReadPixels( Pixels );
}

void UPSPRenderDevice::EndFlash()
{
	if( SoftwareDevice )
		SoftwareDevice->EndFlash();
}

void UPSPRenderDevice::BlitSoftwareBuffer()
{
	if( !Initialized || !SoftwareBuffer )
		return;

	// Upload the software framebuffer into the current GU draw buffer via CPU copy.
	const int BytesPerPixel = Viewport->ColorBytes; // 2 for RGB565
	const int DestStrideBytes = PSP_BUF_WIDTH * BytesPerPixel;
	const int SrcStrideBytes = SoftwareStride * BytesPerPixel;
	const int RowBytes = Viewport->SizeX * BytesPerPixel;

	sceKernelDcacheWritebackRange( SoftwareBuffer, SrcStrideBytes * Viewport->SizeY );

	uint8_t* Dest = static_cast<uint8_t*>( GDrawBuffer );
	const uint8_t* Src = static_cast<uint8_t*>( SoftwareBuffer );
	for( int y = 0; y < Viewport->SizeY; ++y )
	{
		memcpy( Dest + y * DestStrideBytes, Src + y * SrcStrideBytes, RowBytes );
	}

	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();

	// Toggle draw/disp buffers for the next frame.
	GDrawBufferIndex ^= 1;
	GDrawBuffer = GFrameBuffers[GDrawBufferIndex];
	GDispBuffer = GFrameBuffers[GDrawBufferIndex ^ 1];
}
