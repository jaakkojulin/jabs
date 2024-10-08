#ifndef WIN_COMPAT_H
#define WIN_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define F_OK 0
#define W_OK 2
#define R_OK 4
char *strsep(char **, const char *);
char *strndup(char *s1, size_t n);
char *dirname(char *path);
#endif
#ifdef _MSC_VER
#include <stdarg.h>
#define vscprintf _vscprintf
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
int asprintf(char **strp, const char *format, ...);
int vasprintf(char **strp, const char *format, va_list ap);
#endif
#ifdef __cplusplus
}
#endif
#endif // WIN_COMPAT_H
