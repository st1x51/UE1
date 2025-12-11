// Minimal PSP launcher (no SDL) modeled after SDLLaunch.cpp
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <psputility.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Engine.h"

// Command line storage
static char GCmdLine[512] = "";
static INT GArgc = 0;
static const char* GArgv[32];

static bool DirectoryExists( const char* Path )
{
	SceUID Dir = sceIoDopen( Path );
	if( Dir >= 0 )
	{
		sceIoDclose( Dir );
		return true;
	}
	return false;
}

static void EnsureTrailingSlash( char* Path, size_t PathLen )
{
	size_t Len = strlen( Path );
	if( Len > 0 )
	{
		char Last = Path[Len-1];
		if( Last != '/' && Last != '\\' && Len < PathLen - 1 )
		{
			Path[Len++] = '/';
			Path[Len] = 0;
		}
	}
}

static bool GetExecutableDir( char* Out, size_t OutLen )
{
	if( GArgc > 0 && GArgv[0] && GArgv[0][0] )
	{
		appStrncpy( Out, GArgv[0], OutLen );
		char* SlashA = strrchr( Out, '/' );
		char* SlashB = strrchr( Out, '\\' );
		char* Slash = SlashA ? SlashA : SlashB;
		if( Slash )
		{
			Slash[1] = 0;
			return true;
		}
	}
	return false;
}

static bool ValidateRootDir( const char* Root, char* Out, size_t OutLen )
{
	if( !Root || !Root[0] )
		return false;
	char Test[512];
	appStrncpy( Test, Root, ARRAY_COUNT(Test) );
	EnsureTrailingSlash( Test, ARRAY_COUNT(Test) );

	char SystemTest[512];
	appStrncpy( SystemTest, Test, ARRAY_COUNT(SystemTest) );
	appStrncat( SystemTest, "System", ARRAY_COUNT(SystemTest) );
	EnsureTrailingSlash( SystemTest, ARRAY_COUNT(SystemTest) );
	if( DirectoryExists( SystemTest ) )
	{
		appStrncpy( Out, Test, OutLen );
		return true;
	}
	return false;
}

static bool FindRootPath( char* Out, size_t OutLen )
{
	char ExeDir[256];
	if( GetExecutableDir( ExeDir, ARRAY_COUNT(ExeDir) ) )
	{
		if( ValidateRootDir( ExeDir, Out, OutLen ) )
			return true;
	}

	const char* Candidates[] =
	{
		"./",
		"ms0:/PSP/GAME/UE1/",
		"ms0:/PSP/GAME/",
		"ms0:/",
		"umd0:/PSP/GAME/UE1/",
		"umd0:/PSP/GAME/",
		"umd0:/",
	};

	for( size_t i = 0; i < ARRAY_COUNT(Candidates); ++i )
	{
		if( ValidateRootDir( Candidates[i], Out, OutLen ) )
			return true;
	}

	Out[0] = 0;
	return false;
}

static void WaitForButtonAndExit()
{
    pspDebugScreenPrintf("\nPress X to exit...\n");
    SceCtrlData pad;
    while(1)
    {
        sceCtrlPeekBufferPositive(&pad, 1);
        if(pad.Buttons & PSP_CTRL_CROSS)
            break;
        sceKernelDelayThread(50 * 1000);
    }
    //sceKernelExitGame();
}

PSP_MODULE_INFO("UE1_PSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);  // Reserve more heap
PSP_MAIN_THREAD_STACK_SIZE_KB(512);  // Larger stack for Unreal

extern CORE_API FGlobalPlatform GTempPlatform;
extern DLL_IMPORT UBOOL GTickDue;
extern "C" { HINSTANCE hInstance; }
extern "C" { char GCC_HIDDEN THIS_PACKAGE[64] = "Launch"; }

// FExecHook implementation
class FExecHook : public FExec
{
public:
    UBOOL Exec(const char* Cmd, FOutputDevice* Out)
    {
        // Handle PSP-specific commands here
        if(ParseCommand(&Cmd, "QUIT") || ParseCommand(&Cmd, "EXIT"))
        {
            GIsRequestingExit = 1;
            return 1;
        }
        return 0;
    }
};

FExecHook GLocalHook;
DLL_EXPORT FExec* GThisExecHook = &GLocalHook;

// PSP Log output device
static bool GLogToDebugScreen = true;

class FPSPLogOutput : public FOutputDevice
{
public:
	void WriteBinary( const void* Data, INT Length, EName Event = NAME_None ) override
	{
		if( !GLogToDebugScreen )
			return;
		if( Event != NAME_None )
		{
			FName EventName( Event );
			if( EventName.IsValid() )
				pspDebugScreenPrintf( "[%s] ", *EventName );
        }
        pspDebugScreenPrintf( "%.*s", Length, static_cast<const char*>( Data ) );
    }
};

static FPSPLogOutput GPSPLog;

static void DisableDebugScreenLog()
{
	GLogToDebugScreen = false;
}

// Exit callback thread
static int ExitCallbackThread(SceSize args, void* argp)
{
    int cbid = sceKernelCreateCallback("ExitCallback", 
        [](int arg1, int arg2, void* common) -> int
        {
            GIsRequestingExit = 1;
            return 0;
        }, nullptr);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void PlatformPreInit()
{
    // Enable VFPU flush-to-zero mode
    asm volatile (
        ".set push\n"
        ".set noreorder\n"
        "cfc1    $2, $31\n"
        "lui     $8, 0x80\n"
        "and     $8, $2, $8\n"
        "ctc1    $8, $31\n"
        ".set pop\n"
        ::: "$2", "$8"
    );

    pspDebugScreenInit();
    pspDebugScreenPrintf("Unreal Engine 1 - PSP Port\n");
    pspDebugScreenPrintf("==========================\n\n");

    // Create exit callback thread
    int cbTh = sceKernelCreateThread("cb_thread", ExitCallbackThread, 0x11, 0x1000, 0, nullptr);
    if(cbTh >= 0)
        sceKernelStartThread(cbTh, 0, nullptr);

    // Set CPU/GPU clock to maximum
    scePowerSetClockFrequency(333, 333, 166);
    
    // Initialize controller
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    // Find writable base directory (root containing System/)
    char RootPath[256] = "";
    if( !FindRootPath( RootPath, sizeof(RootPath) ) )
        appStrcpy( RootPath, "./" );
    EnsureTrailingSlash( RootPath, sizeof(RootPath) );

    char SystemDir[256];
    appStrncpy( SystemDir, RootPath, sizeof(SystemDir) );
    appStrncat( SystemDir, "System/", sizeof(SystemDir) );
    EnsureTrailingSlash( SystemDir, sizeof(SystemDir) );

    if( chdir( SystemDir ) != 0 )
    {
        pspDebugScreenPrintf("Failed to chdir to System dir: %s\n", SystemDir );
        chdir( "./" );
    }
    
    pspDebugScreenPrintf("Base path: %s\n", SystemDir );

    // Helper lambdas
    auto FileExists = [](const char* Path) -> bool {
        FILE* F = fopen(Path, "rb");
        if(F) { fclose(F); return true; }
        return false;
    };

    auto CopyFileSafe = [](const char* Src, const char* Dest) -> bool {
        FILE* In = fopen(Src, "rb");
        if(!In) return false;
        FILE* Out = fopen(Dest, "wb");
        if(!Out) { fclose(In); return false; }
        char Buf[4096];
        size_t N;
        while((N = fread(Buf, 1, sizeof(Buf), In)) > 0)
        {
            if(fwrite(Buf, 1, N, Out) != N)
            {
                fclose(In);
                fclose(Out);
                return false;
            }
        }
        fclose(In);
        fclose(Out);
        return true;
    };

    auto WriteDefaultIni = [](const char* Dest) -> bool {
        const char* Contents =
            "[Engine.Engine]\n"
            "ViewportManager=PSPDrv.PSPClient\n"
            "GameRenderDevice=PSPDrv.PSPRenderDevice\n"
            "WindowedRenderDevice=PSPDrv.PSPRenderDevice\n"
            "AudioDevice=SoundDrv.NullAudioSubsystem\n"
            "NetworkDevice=IpDrv.TcpNetDriver\n"
            "Console=Engine.Console\n"
            "GameEngine=Engine.GameEngine\n"
            "EditorEngine=Editor.EditorEngine\n"
            "Render=Render.Render\n"
            "Input=Engine.Input\n"
            "Canvas=Engine.Canvas\n"
            "DefaultGame=UnrealI.SinglePlayer\n"
            "DefaultServerGame=UnrealI.DeathMatchGame\n"
            "Language=int\n"
            "\n"
            "[Core.System]\n"
            "PurgeCacheDays=30\n"
            "SavePath=../Save\n"
            "CachePath=../Cache\n"
            "CacheExt=.uxx\n"
            "Paths[0]=../System/*.u\n"
            "Paths[1]=../Maps/*.unr\n"
            "Paths[2]=../Textures/*.utx\n"
            "Paths[3]=../Sounds/*.uax\n"
            "Paths[4]=../Music/*.umx\n"
            "\n"
            "[Engine.GameEngine]\n"
            "CacheSizeMegs=2\n"
            "UseSound=False\n"
            "FirstRun=True\n"
            "\n"
            "[PSPDrv.PSPClient]\n"
            "ViewportX=480\n"
            "ViewportY=272\n"
            "Brightness=0.6\n"
            "StartupFullscreen=True\n"
            "\n"
            "[PSPDrv.PSPRenderDevice]\n"
            "FullScreen=True\n"
            "ScreenWidth=480\n"
            "ScreenHeight=272\n"
            "ColorDepth=32\n"
            "DetailTextures=True\n"
            "Coronas=False\n"
            "ShinySurfaces=True\n"
            "VolumetricLighting=False\n"
            "\n";
        FILE* Out = fopen(Dest, "wb");
        if(!Out) return false;
        size_t Len = strlen(Contents);
        size_t Written = fwrite(Contents, 1, Len, Out);
        fclose(Out);
        return Written == Len;
    };

    // Ensure config files exist
    const char* DefaultIni = "Default.ini";
    const char* UnrealIni = "Unreal.ini";

    auto MakeRootSystemPath = [&]( const char* Root, const char* File, char* Out, size_t OutSize )
    {
        appStrncpy( Out, Root, OutSize );
        EnsureTrailingSlash( Out, OutSize );
        appStrncat( Out, "System/", OutSize );
        appStrncat( Out, File, OutSize );
    };

    if(!FileExists(DefaultIni))
    {
        char BaseCandidate[512];
        MakeRootSystemPath( RootPath, "Default.ini", BaseCandidate, sizeof(BaseCandidate) );
        const char* Candidates[] =
        {
            BaseCandidate,
            "umd0:/PSP/GAME/UE1/System/Default.ini",
            "umd0:/System/Default.ini",
            "umd0:/Default.ini"
        };
        for( size_t i=0; i<ARRAY_COUNT(Candidates); ++i )
        {
            if( FileExists(Candidates[i]) )
            {
                if( CopyFileSafe( Candidates[i], DefaultIni ) )
                {
                    pspDebugScreenPrintf("Copied Default.ini from %s\n", Candidates[i]);
                    break;
                }
            }
        }
    }
    
    if(!FileExists(UnrealIni))
    {
        if(FileExists(DefaultIni))
        {
            if(CopyFileSafe(DefaultIni, UnrealIni))
                pspDebugScreenPrintf("Created Unreal.ini from Default.ini\n");
        }
        else
        {
            if(WriteDefaultIni(UnrealIni))
                pspDebugScreenPrintf("Synthesized minimal Unreal.ini\n");
        }
    }

    pspDebugScreenPrintf("\nInitialization complete.\n\n");
    sceKernelDelayThread(500 * 1000);  // Brief pause to see messages
}

// Handle critical error
void HandleError()
{
    GIsGuarded = 0;
    GIsCriticalError = 1;
    
    debugf(NAME_Exit, "Shutting down after catching exception");
    
    try
    {
        GObj.ShutdownAfterError();
    }
    catch(...) {}
    
    debugf(NAME_Exit, "Exiting due to exception");
    
    GErrorHist[ARRAY_COUNT(GErrorHist) - 1] = 0;
    
    pspDebugScreenInit();
    pspDebugScreenPrintf("=== CRITICAL ERROR ===\n\n%s\n", GErrorHist);
}

// Initialize engine
UEngine* InitEngine()
{
    guard(InitEngine);

    // Platform init
    appInit();
    
    // Initialize memory subsystems (PSP only has ~45MB available)
    const INT DynMemSize   = 20 * 1024;
    const INT SceneMemSize = 24 * 1024;
    GDynMem.Init(DynMemSize);
    GSceneMem.Init(SceneMemSize);
    // First-run check
    UBOOL FirstRun = 0;
    GetConfigBool("FirstRun", "FirstRun", FirstRun);

    // Determine engine class to load
    UClass* EngineClass;
    
    if(!GIsEditor)
    {
        // Game engine
        EngineClass = GObj.LoadClass(
            UGameEngine::StaticClass, 
            NULL, 
            "ini:Engine.Engine.GameEngine", 
            NULL, 
            LOAD_NoFail | LOAD_KeepImports, 
            NULL
        );
    }
    else if(ParseParam(appCmdLine(), "MAKE"))
    {
        // Editor engine (make mode)
        EngineClass = GObj.LoadClass(
            UEngine::StaticClass, 
            NULL, 
            "ini:Engine.Engine.EditorEngine", 
            NULL, 
            LOAD_NoFail | LOAD_DisallowFiles | LOAD_KeepImports, 
            NULL
        );
    }
    else
    {
        // Editor engine
        EngineClass = GObj.LoadClass(
            UEngine::StaticClass, 
            NULL, 
            "ini:Engine.Engine.EditorEngine", 
            NULL, 
            LOAD_NoFail | LOAD_KeepImports, 
            NULL
        );
    }

    // Create and initialize engine
    UEngine* Engine = ConstructClassObject<UEngine>(EngineClass);
    check(Engine);
    Engine->Init();

    debugf(NAME_Init, "Engine initialized successfully");
	pspDebugScreenPrintf("Engine initialized successfully.\n");
    return Engine;

    unguard;
}

// Process PSP input and convert to Unreal input events
static void ProcessPSPInput(UEngine* Engine)
{
    static SceCtrlData PrevPad = {0};
    SceCtrlData Pad;
    
    sceCtrlPeekBufferPositive(&Pad, 1);
    
    // Detect button state changes
    DWORD Pressed = Pad.Buttons & ~PrevPad.Buttons;
    DWORD Released = ~Pad.Buttons & PrevPad.Buttons;
    
    // Map PSP buttons to Unreal keys
    // This would need to interface with the input system
    // For now, just handle critical buttons
    
    if(Pressed & PSP_CTRL_START)
    {
        // Could open menu or pause
    }
    
    if(Pressed & PSP_CTRL_SELECT)
    {
        // Could toggle console
    }
    
    // Check for exit combo (L + R + Start)
    if((Pad.Buttons & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_START)) == 
       (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_START))
    {
        GIsRequestingExit = 1;
    }
    
    PrevPad = Pad;
}

// Main loop
void MainLoop(UEngine* Engine)
{
    guard(MainLoop);

    GIsRunning = 1;
    DOUBLE OldTime = appSeconds();
    
    while(GIsRunning && !GIsRequestingExit)
    {
        // Process PSP-specific input
        ProcessPSPInput(Engine);
        
        // Calculate delta time
        DOUBLE NewTime = appSeconds();
        FLOAT DeltaTime = (FLOAT)(NewTime - OldTime);
        OldTime = NewTime;
        
        // Clamp delta time to avoid huge jumps
        if(DeltaTime > 0.5f)
            DeltaTime = 0.5f;
        
        // Update engine
        Engine->Tick(DeltaTime);
        
        // Frame rate limiting
        INT MaxTickRate = Engine->GetMaxTickRate();
        if(MaxTickRate > 0)
        {
            DOUBLE TargetDelta = 1.0 / MaxTickRate;
            DOUBLE Elapsed = appSeconds() - OldTime;
            if(Elapsed < TargetDelta)
            {
                DWORD SleepUS = (DWORD)((TargetDelta - Elapsed) * 1000000.0);
                sceKernelDelayThread(SleepUS);
            }
        }
        else
        {
            // Default to ~30 FPS if no limit specified
            sceDisplayWaitVblankStart();
        }
        
        // Check kernel callbacks (for exit, etc.)
        sceKernelCheckCallback();
    }
    
    GIsRunning = 0;
    
    unguard;
}

// Exit engine
void ExitEngine(UEngine* Engine)
{
    guard(ExitEngine);

    debugf(NAME_Exit, "Shutting down engine...");

    if(Engine)
    {
        delete Engine;
        Engine = nullptr;
    }

    GObj.Exit();
    GCache.Exit(1);
    GSceneMem.Exit();
    GDynMem.Exit();
    GMem.Exit();
    
    appDumpAllocs(&GTempPlatform);

    unguard;
}

// Command line setup for PSP
static void SetupCommandLine(int argc, char* argv[])
{
    GCmdLine[0] = 0;
    GArgc = argc;
    
    for(int i = 0; i < argc && i < 32; ++i)
    {
        GArgv[i] = argv[i];
        if(i > 0)
        {
            if(strlen(GCmdLine) + strlen(argv[i]) + 2 < sizeof(GCmdLine))
            {
                strcat(GCmdLine, " ");
                strcat(GCmdLine, argv[i]);
            }
        }
    }
}

// Main entry point
int main(int argc, char* argv[])
{
    // Setup command line
    SetupCommandLine(argc, argv);
    appSetCmdLine(argc, (const char**)argv);

    // Null out Windows-specific handle
    hInstance = NULL;
    
    // Platform pre-initialization
    PlatformPreInit();
    // Mark as started
    GIsStarted = 1;

    // Set package name
    appStrcpy(THIS_PACKAGE, appPackage());
    // Initialize mode flags
    GIsServer = 1;
    GIsClient = !ParseParam(appCmdLine(), "SERVER") && !ParseParam(appCmdLine(), "MAKE");
    GIsEditor = ParseParam(appCmdLine(), "EDITOR") || ParseParam(appCmdLine(), "MAKE");
    // Set working directory
    appChdir(appBaseDir());

    // Initialize logging/exec hook
    GLogHook = &GPSPLog;
    GExecHook = GThisExecHook;
    // Main execution
#ifndef _DEBUG
    try
    {
#endif
        GIsGuarded = 1;
        GSystem = &GTempPlatform;
        UEngine* Engine = InitEngine();
        
        if(Engine && !GIsRequestingExit)
        {
            // Stop writing logs into the debug screen once rendering starts to avoid corrupting the frame.
            DisableDebugScreenLog();
            MainLoop(Engine);
        }
        
        if(Engine)
        {
            ExitEngine(Engine);
        }
        
        GIsGuarded = 0;
#ifndef _DEBUG
    }
    catch(...)
    {
        try { HandleError(); } catch(...) {}
    }
#endif

    // Shutdown
    
    GExecHook = NULL;
    GLogHook = NULL;
    
    appExit();
    GIsStarted = 0;

    // Wait for user if error occurred
    if(GIsCriticalError)
    {
        WaitForButtonAndExit();
    }

    sceKernelExitGame();
    return 0;
}
