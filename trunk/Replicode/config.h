#ifndef	config_h
#define	config_h

#ifdef	LINUX
#include	<stdlib.h>
#include	<vector>
#include	<string>
#include	<list>
#include	<unordered_map>
#define		UNORDERED_MAP	std::unordered_map
#else
#ifdef	WINDOWS
#include	<hash_map>
#define		UNORDERED_MAP	stdext::hash_map
#endif
#endif


#endif
