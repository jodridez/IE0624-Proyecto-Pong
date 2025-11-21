#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>



/* Escritura: no hace nada, devuelve la longitud */
int _write(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    return len;
}

/* Lectura: no hace nada */
int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/* Cierre de archivo */
int _close(int file) {
    (void)file;
    return -1;
}

/* Devuelve estado de archivo */
int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

/* Terminal */
int _isatty(int file) {
    (void)file;
    return 1;
}

/* Reposicionamiento */
int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

/* Funciones necesarias para abort() */
void _exit(int status) {
    (void)status;
    while(1);
}

int _getpid(void) { return 1; }
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    return -1;
}

caddr_t _sbrk(int incr) {
    extern char _end; // definido en linker script
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;
    heap_end += incr;
    return (caddr_t) prev_heap_end;
}


