#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

// Redirect printf to your console output
extern void console_putc(char c);

caddr_t _sbrk(int incr) {
    extern char _end;         // Defined by the linker
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }

    prev_heap_end = heap_end;
    heap_end += incr;
    return (caddr_t) prev_heap_end;
}

int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        console_putc(ptr[i]);
    }
    return len;
}

int _read(int file, char *ptr, int len) {
    return 0;
}

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}
