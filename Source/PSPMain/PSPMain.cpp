// Minimal PSP entry point stub to satisfy the linker.
#include <pspkernel.h>

PSP_MODULE_INFO("UE1_PSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main( int argc, char* argv[] )
{
	(void)argc; (void)argv;
	// TODO: hook into engine startup once the PSP platform layer is ready.
	return 0;
}
