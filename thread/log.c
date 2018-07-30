#include "log.h"

#include <string.h>

char *splitFileName(char *str)
{
    char *pChar = (char *)str;
    pChar  = (strrchr(pChar, '/') ? strrchr(pChar, '/') + 1 : (strrchr(pChar, '\\') ? strrchr(pChar, '\\') + 1 : pChar));
    return pChar;
}
