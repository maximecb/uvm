#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_


extern int * __error(void);
#define errno (*__error())

#endif /* _SYS_ERRNO_H_ */
