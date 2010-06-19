#ifndef	r_exec_dll_h
#define	r_exec_dll_h


#if defined	EXECUTIVE_EXPORTS
	#define r_exec_dll	__declspec(dllexport)
#else
	#define r_exec_dll	__declspec(dllimport)
#endif


#endif