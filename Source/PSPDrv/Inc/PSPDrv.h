/*=============================================================================
	PSPDrv.h: PSP GU render device declarations.
=============================================================================*/

#pragma once

#include <stdint.h>
#include "UnRender.h"

class UPSPRenderDevice : public URenderDevice
{
	DECLARE_CLASS(UPSPRenderDevice,URenderDevice,CLASS_Config)

	// Basic GU state.
	UBOOL Initialized;
	static uint32_t CommandList[262144/4]; // 256 KB command list (aligned by definition).

public:
	// UObject interface.
	void Destroy() override;

	// URenderDevice interface.
	UBOOL Init( UViewport* InViewport ) override;
	void Exit() override;
	void Flush() override;
	UBOOL Exec( const char* Cmd, FOutputDevice* Out ) override;
	void Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize ) override;
	void Unlock( UBOOL Blit ) override;
	void DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet ) override;
	void DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span ) override;
	void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags ) override;
	void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 ) override;
	void Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2 ) override;
	void ClearZ( FSceneNode* Frame ) override;
	void PushHit( const BYTE* Data, INT Count ) override;
	void PopHit( INT Count, UBOOL bForce ) override;
	void GetStats( char* Result ) override;
	void ReadPixels( FColor* Pixels ) override;
	void EndFlash() override;
};
