/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* 1999-02-22 Arkadiusz Mi¶kiewicz <misiek@misiek.eu.org>
 * - added Native Language Support
 */

/* 2000-12-27 Satoru Takabayashi <satoru@namazu.org>
 * - modify `script' to create `ttyrec'.
 */

/*
 * script
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#if defined(SVR4)
#include <fcntl.h>
#include <stropts.h>
#endif /* SVR4 */

#include <sys/time.h>
#include "kringkastty.h"
#include "io.h"
#include "mongoose.h"
#include "content.h"
#include "lw_terminal_vt100.h"
#include "buffer.h"

#define HAVE_inet_aton
#define HAVE_scsi_h
#define HAVE_kd_h

#define _(FOO) FOO

#ifdef HAVE_openpty
#include <pty.h>
#include <utmp.h>
#endif

#if defined(SVR4) && !defined(CDEL)
#if defined(_POSIX_VDISABLE)
#define CDEL _POSIX_VDISABLE
#elif defined(CDISABLE)
#define CDEL CDISABLE
#else /* not _POSIX_VISIBLE && not CDISABLE */
#define CDEL 255
#endif /* not _POSIX_VISIBLE && not CDISABLE */
#endif /* SVR4 && ! CDEL */

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

const struct mg_str s_get_method = MG_STR("GET");
const char *server_port = "8080";

void done(void);
void fail(void);
void fixtty(void);
void getmaster(void);
void getslave(void);
void doinput(void);
void dooutput(void);
void doshell(const char*);
int ttymain(int, char** const);
void dowebserver(const char *, int, struct winsize*);

char	*shell;
FILE	*fscript;
int	master;
int	slave;
int	child;
int	subchild;
char	*fname;

struct	termios tt;
struct	winsize win;
int	lb;
int	l;
#if !defined(SVR4)
#ifndef HAVE_openpty
char	line[] = "/dev/ptyXX";
#endif
#endif /* !SVR4 */
int	aflg;
int	uflg;
int fds[2];

static void
resize(int dummy) {
	/* transmit window change information to the child */
	(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	(void) ioctl(master, TIOCSWINSZ, (char *)&win);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
    if (pipe(fds) < 0) {
        perror("pipe");
        abort();
    }

    int flags = fcntl(fds[0], F_GETFL, 0);
    if (fcntl(fds[0], F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        abort();

    }

    printf("Starting server on port %s. Type 'exit' to stop recording & broadcasting.\n", server_port);
    int child1 = fork();
    if (child1 == 0) { // web server
        close(fds[1]);
        // Get terminal size
        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        dowebserver(server_port, fds[0], &w);
        close(fds[1]);
    } else { // ttyrec
        close(fds[0]);
        int res= ttymain(argc, argv);
        close(fds[1]);
        return res;
    }
}

int
ttymain(argc, argv)
	int argc;
	char *argv[];
{
	struct sigaction sa;
	extern int optind;
	int ch;
	void finish();
	char *getenv();
	char *command = NULL;

	while ((ch = getopt(argc, argv, "aue:h?")) != EOF)
		switch((char)ch) {
		case 'a':
			aflg++;
			break;
		case 'u':
		        uflg++;
			break;
		case 'e':
			command = strdup(optarg);
			break;
		case 'h':
		case '?':
		default:
			fprintf(stderr, _("usage: ttyrec [-u] [-e command] [-a] [file]\n"));
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		fname = argv[0];
	else
		fname = "ttyrecord";
	if ((fscript = fopen(fname, aflg ? "a" : "w")) == NULL) {
		perror(fname);
		fail();
	}
	setbuf(fscript, NULL);

	shell = getenv("SHELL");
	if (shell == NULL)
		shell = "/bin/sh";

	getmaster();
	fixtty();
    sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = finish;
	sigaction(SIGCHLD, &sa, NULL);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
		}
		if (child) {
			sa.sa_flags = SA_RESTART;
			sigaction(SIGCHLD, &sa, NULL);
			dooutput();
		} else
			doshell(command);
	}
	sa.sa_handler = resize;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGWINCH, &sa, NULL);
	doinput();

	return 0;
}


int
is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

void
broadcast(struct mg_connection *nc, const char *msg, size_t len) {
    struct mg_connection *c;
    for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, msg, len);
    }
}

int
is_equal(const struct mg_str *s1, const struct mg_str *s2) {
    return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}

uint8_t
get_mode(uint32_t mode) {
    switch(mode) {
    case 0x80000:
        return 1;
    case 0x100000:
        return 4;
    case 0x40000:
        return 5;
    case 0x200000:
        return 7;
    default:
        return 0;
    }
}

void
print_ansi_sequence(uint32_t attr, buffer *buf) {
    unsigned i;
    unsigned int d = 0;
    uint32_t modes[4] = {0x80000, 0x100000, 0x40000, 0x200000};
    uint16_t color, highbit;
    buf_printf(buf, "\033[");
    if (attr == 0) {
        buf_printf(buf, "0m");
        return;
    }

    for (i = 0; i<sizeof(modes)/sizeof(uint32_t); ++i) {
        if (attr & modes[i]) {
            buf_printf(buf, d++? ";%d" : "%d", get_mode(modes[i]));
        }
    }

    color = attr & 0xFF;
    highbit = attr & 0x100;
    if (highbit) {
        buf_printf(buf, d++? ";38;5;%d" : "38;5;%d", color);
    } else if ((color >= 30 && color < 38) || color == 39) {
        buf_printf(buf, d++? ";%d" : "%d", color);
    }

    color = (attr >> 9) & 0xFF;
    highbit = (attr >> 9) & 0x100;
    if (highbit) {
        buf_printf(buf, d++? ";48;5;%d" : "48;5;%d", color);
    } else if ((color >= 40 && color < 48) || color == 49) {
        buf_printf(buf, d++? ";%d" : "%d", color);
    }

    buf_printf(buf, "m");
}

void
send_terminal(struct lw_terminal_vt100 *term, struct mg_connection *nc) {
    unsigned int last = 0xFFFFFFFF;
    unsigned int y, x;
    uint32_t g = 0;
    char c;
    buffer buf;

    // Send terminal meta data.
    mg_printf_websocket_frame(nc, WEBSOCKET_OP_TEXT, "{\"x\":%d,\"y\":%d}", term->width, term->height);

    // Send terminal display.
    buf_init(&buf, BUFSIZ);
    for (y = 0; y<term->height; ++y) {
        for (x = 0; x<term->width; ++x) {
            c = lw_terminal_vt100_get(term, x, y);
            g = lw_terminal_vt100_graphics_rendition(term, x, y);
            if (g != last) {
                print_ansi_sequence(g, &buf);
                last=g;
            }
            buf_write(&buf, &c, 1);
        }
    }

    // Set cursor to correct position
    buf_printf(&buf, "\033[%d;%dH", term->y + 1, term->x+1);

    mg_printf_websocket_frame(nc, WEBSOCKET_OP_TEXT, buf_get(&buf), buf_len(&buf));

    buf_free(&buf);
}

void
ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    switch (ev) {
    case MG_EV_HTTP_REQUEST:
        nc->flags |= MG_F_SEND_AND_CLOSE;
        mg_printf(nc, "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: %d\r\n\r\n", build_content_html_len);
        mg_send(nc, build_content_html, build_content_html_len);
        break;
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
        // Initial connection, send terminal size and content
        send_terminal((struct lw_terminal_vt100 *)nc->mgr->user_data, nc);
        break;
    case MG_EV_WEBSOCKET_FRAME:
    case MG_EV_CLOSE:
    default:
        break;
    }
}

void notimplemented(struct lw_terminal* term, char *seq, char chr) {
    (void)(term);
    (void)(seq);
    (void)(chr);
}

void
dowebserver(const char *port, int fd, struct winsize *w) {
    struct mg_mgr mgr;
    struct mg_connection *nc;
    struct lw_terminal_vt100 *term;

    term = lw_terminal_vt100_init(NULL, w->ws_col, w->ws_row, notimplemented);
    lw_terminal_vt100_read_str(term, "\033[?7h\033[20h");

    mg_mgr_init(&mgr, term);
    nc = mg_bind(&mgr, port, ev_handler);

    mg_set_protocol_http_websocket(nc);

    char buf[BUFSIZ];
    int cc;
    while ((cc = read(fd, buf, BUFSIZ)) != 0) {
        if (cc > 0) {
            lw_terminal_vt100_read_buffer(term, buf, cc);
            broadcast(nc, buf, cc);
        }
        mg_mgr_poll(&mgr, 200);
    }
    mg_mgr_free(&mgr);
    lw_terminal_vt100_destroy(term);
}

void
doinput()
{
	register int cc;
	char ibuf[BUFSIZ];

	(void) fclose(fscript);
#ifdef HAVE_openpty
	(void) close(slave);
#endif
	while ((cc = read(0, ibuf, BUFSIZ)) > 0)
		(void) write(master, ibuf, cc);
	done();
}

#include <sys/wait.h>

void
finish()
{
#if defined(SVR4)
	int status;
#else /* !SVR4 */
	union wait status;
#endif /* !SVR4 */
	register int pid;

	while ((pid = wait3((int *)&status, WNOHANG, 0)) > 0)
		if (pid == child)
			break;
}

struct linebuf {
    char str[BUFSIZ + 1]; /* + 1 for an additional NULL character.*/
    int len;
};


void
check_line (const char *line)
{
    static int uuencode_mode = 0;
    static FILE *uudecode;

    if (uuencode_mode == 1) {
	fprintf(uudecode, "%s", line);
	if (strcmp(line, "end\n") == 0) {
	    pclose(uudecode);
	    uuencode_mode = 0;
	}
    } else {
	int dummy; char dummy2[BUFSIZ];
	if (sscanf(line, "begin %o %s", &dummy, dummy2) == 2) {
	    /* 
	     * uuencode line found! 
	     */
	    uudecode = popen("uudecode", "w");
	    fprintf(uudecode, "%s", line);
	    uuencode_mode = 1;
	}
    }
}

static void
uu_check_output(const char *str, int len)
{
    static struct linebuf lbuf = {"", 0};
    int i;

    for (i = 0; i < len; i++) {
	if (lbuf.len < BUFSIZ) {
	    lbuf.str[lbuf.len] = str[i];
	    if (lbuf.str[lbuf.len] == '\r') {
		lbuf.str[lbuf.len] = '\n';
	    }
	    lbuf.len++;
	    if (lbuf.str[lbuf.len - 1] == '\n') {
		if (lbuf.len > 1) { /* skip a blank line. */
		    lbuf.str[lbuf.len] = '\0';
		    check_line(lbuf.str);
		}
		lbuf.len = 0;
	    }
	} else {/* buffer overflow */
	    lbuf.len = 0;
	}
    }
}

static int
check_output(char *str, int len)
{
    char *p;

    /* If we see query string, remove it */
    /* ESC [ > 0 c : Send Device Attributes */
    if (len >= 5 && (p = strstr(str, "\e[>0c")) != NULL) {
       if (len == 5)
           return 0;
       memmove(p, p+5, len-5+1-(p-str));
       return len-5;
    }

    return len;
}

void
dooutput()
{
	int cc;
	char obuf[BUFSIZ];

	setbuf(stdout, NULL);
	(void) close(0);
#ifdef HAVE_openpty
	(void) close(slave);
#endif
	for (;;) {
		Header h;

		cc = read(master, obuf, BUFSIZ);
		if (cc <= 0)
			break;
        write(fds[1], obuf, cc);

		if (uflg)
		    uu_check_output(obuf, cc);
		h.len = cc;
		gettimeofday(&h.tv, NULL);
		(void) write(1, obuf, cc);
               if ((cc = check_output(obuf, cc))) {
                       (void) write_header(fscript, &h);
                       (void) fwrite(obuf, 1, cc, fscript);
               }
	}
	done();
}

void
doshell(const char* command)
{
	/***
	int t;

	t = open(_PATH_TTY, O_RDWR);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	}
	***/
	getslave();
	(void) close(master);
	(void) fclose(fscript);
	(void) dup2(slave, 0);
	(void) dup2(slave, 1);
	(void) dup2(slave, 2);
	(void) close(slave);

	if (!command) {
		execl(shell, strrchr(shell, '/') + 1, "-i", NULL);
	} else {
		execl(shell, strrchr(shell, '/') + 1, "-c", command, NULL);	
	}
	perror(shell);
	fail();
}

void
fixtty()
{
	struct termios rtt;

	rtt = tt;
#if defined(SVR4)
#if !defined(XCASE)
#define XCASE 0
#endif
	rtt.c_iflag = 0;
	rtt.c_lflag &= ~(ISIG|ICANON|XCASE|ECHO|ECHOE|ECHOK|ECHONL);
	rtt.c_oflag = OPOST;
	rtt.c_cc[VINTR] = CDEL;
	rtt.c_cc[VQUIT] = CDEL;
	rtt.c_cc[VERASE] = CDEL;
	rtt.c_cc[VKILL] = CDEL;
	rtt.c_cc[VEOF] = 1;
	rtt.c_cc[VEOL] = 0;
#else /* !SVR4 */
	cfmakeraw(&rtt);
	rtt.c_lflag &= ~ECHO;
#endif /* !SVR4 */
	(void) tcsetattr(0, TCSAFLUSH, &rtt);
}

void
fail()
{

	(void) kill(0, SIGTERM);
	done();
}

void
done()
{
	if (subchild) {
		(void) fclose(fscript);
		(void) close(master);
	} else {
		(void) tcsetattr(0, TCSAFLUSH, &tt);
	}
	exit(0);
}

void
getmaster()
{
#if defined(SVR4)
	(void) tcgetattr(0, &tt);
	(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	if ((master = open("/dev/ptmx", O_RDWR)) < 0) {
		perror("open(\"/dev/ptmx\", O_RDWR)");
		fail();
	}
#else /* !SVR4 */
#ifdef HAVE_openpty
	(void) tcgetattr(0, &tt);
	(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	if (openpty(&master, &slave, NULL, &tt, &win) < 0) {
		fprintf(stderr, _("openpty failed\n"));
		fail();
	}
#else
#ifdef HAVE_getpt
	if ((master = getpt()) < 0) {
		perror("getpt()");
		fail();
	}
#else
	char *pty, *bank, *cp;
	struct stat stb;

	pty = &line[strlen("/dev/ptyp")];
	for (bank = "pqrs"; *bank; bank++) {
		line[strlen("/dev/pty")] = *bank;
		*pty = '0';
		if (stat(line, &stb) < 0)
			break;
		for (cp = "0123456789abcdef"; *cp; cp++) {
			*pty = *cp;
			master = open(line, O_RDWR);
			if (master >= 0) {
				char *tp = &line[strlen("/dev/")];
				int ok;

				/* verify slave side is usable */
				*tp = 't';
				ok = access(line, R_OK|W_OK) == 0;
				*tp = 'p';
				if (ok) {
					(void) tcgetattr(0, &tt);
				    	(void) ioctl(0, TIOCGWINSZ, 
						(char *)&win);
					return;
				}
				(void) close(master);
			}
		}
	}
	fprintf(stderr, _("Out of pty's\n"));
	fail();
#endif /* not HAVE_getpt */
#endif /* not HAVE_openpty */
#endif /* !SVR4 */
}

void
getslave()
{
#if defined(SVR4)
	(void) setsid();
	grantpt( master);
	unlockpt(master);
	if ((slave = open((const char *)ptsname(master), O_RDWR)) < 0) {
		perror("open(fd, O_RDWR)");
		fail();
	}
	if (isastream(slave)) {
		if (ioctl(slave, I_PUSH, "ptem") < 0) {
			perror("ioctl(fd, I_PUSH, ptem)");
			fail();
		}
		if (ioctl(slave, I_PUSH, "ldterm") < 0) {
			perror("ioctl(fd, I_PUSH, ldterm)");
			fail();
		}
#ifndef _HPUX_SOURCE
		if (ioctl(slave, I_PUSH, "ttcompat") < 0) {
			perror("ioctl(fd, I_PUSH, ttcompat)");
			fail();
		}
#endif
	}
       (void) tcsetattr(slave, TCSAFLUSH, &tt);
       (void) ioctl(slave, TIOCSWINSZ, (char *)&win);
       (void) ioctl(slave, TIOCSCTTY, 0);
#else /* !SVR4 */
#ifndef HAVE_openpty
	line[strlen("/dev/")] = 't';
	slave = open(line, O_RDWR);
	if (slave < 0) {
		perror(line);
		fail();
	}
	(void) tcsetattr(slave, TCSAFLUSH, &tt);
	(void) ioctl(slave, TIOCSWINSZ, (char *)&win);
#endif
	(void) setsid();
	(void) ioctl(slave, TIOCSCTTY, 0);
#endif /* SVR4 */
}
