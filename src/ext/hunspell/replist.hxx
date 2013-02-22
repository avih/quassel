/* string replacement list class */
#ifndef _REPLIST_HXX_
#define _REPLIST_HXX_

#include "hunvisapi.h"

#include "w_char.hxx"

class LIBHUNSPELL_DLL_EXPORTED RepList
{
protected:
    replentry ** dat;
    int size;
    int pos;

public:
    RepList(int n);
    ~RepList();

    int get_pos();
    int add(char * pat1, char * pat2);
    replentry * item(int n);

    //for some reason, gcc complains when the name is 'near'. Currently unused anywhere except inside replist.cxx
	int near1(const char * word);

    int match(const char * word, int n);
    int conv(const char * word, char * dest);
};
#endif
