#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/************ Key define *******************/
#define CTRL_KEY(k) ((k) & 0x1f)
/************ Global Variable **************/
struct editorConfig {
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

void editorDrawRows() {
    int y;
    for (y = 0; y < E.screenrows; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
    
}

void clearAndRepositionCursor() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void editorRefreshScreen() {
    clearAndRepositionCursor();
    editorDrawRows();
    
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void die(const char *s) {
    clearAndRepositionCursor();
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");

    tcgetattr(STDIN_FILENO, &E.orig_termios);
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* IXON: software flow control 
     * IEXTEN: Ctrl + V         
     * ICRNL ; I - input, CR - carriage return, NL - newline */
    raw.c_iflag &= ~(ICRNL | IXON);
    
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    /* ECHO: print to terminal
     * ICANON: canonical mode
     * ISIG: SIGINT, SIGTSTP */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);

    /* set timeout */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

char editorReadKey() {
    int nread = 0;
    char c;
    while ((nread == read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
    
}

void editorProcessKeyPress() {
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        clearAndRepositionCursor();
        exit(0);
        break;
    
    default:
        break;
    }
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void initEditor() {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
}

int main()
{
    enableRawMode();
    initEditor();

    while (1) {
        clearAndRepositionCursor();
        editorProcessKeyPress();
    }

    return 0;
}
