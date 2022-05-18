
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

Display *display;
GC gc;
Window win, root;
int mouse_x = 0, mouse_y = 0;
short unsigned int scale = 2;
short unsigned int hex = 0, grid = 1;

void usage(char * prog) {
	printf("Usage:\n\
%s [-scale <picture scale>] [-hex <0,1>] [-grid <0,1>] <zx scr picture>\n", prog);
}

int getPixOffset(int x, int y) {
	int third, p_line, a_line, column;

	third = y/64;
	p_line = y % 8;
	a_line = (y / 8) % 8;
	column = x / 8;

	return (third * 8 + p_line) * 256 + (32 * a_line + column);
}

int getAttrOffset(int x, int y) {
	int row, column;

	row = y / 8;
	column = x / 8;

	return 6144 + 32*row + column;
}

long getColour(unsigned char *data, int x, int y, int pix) {
	int offset;
	unsigned char attr, bright;

	offset = getAttrOffset(x, y);
	attr = data[offset];
	bright = attr & 64 ? 0xFF : 0xD8;
	if ((x & (~7)) == (mouse_x & (~7)) && (y & (~7)) == (mouse_y & (~7)))
		bright /= 2;
	attr &= 63;
	if (!pix)
		attr = attr >> 3;

	return (attr & 2 ? ((long)bright << 16) : 0L) + (attr & 4 ? ((long)bright << 8) : 0L) +
		(attr & 1 ? (long)bright : 0L);
}

void drawSpeccyScreen(XImage *img, unsigned char *data, short unsigned int scale) {
	int x,y, xs, ys;
	int offset;
	long colour;

	for (y = 0; y < 192; y++)
		for (x = 0; x < 256; x++) {
			offset = getPixOffset(x, y);
			colour = getColour(data, x, y, (data[offset] >> (7 - x % 8)) & 1);
			for (ys = 0; ys < scale; ys++)
				for (xs = 0; xs < scale; xs++)
					XPutPixel(img, x*scale + xs, y*scale + ys, colour);

		}
}

void updateInfo(int x, int y) {
	char info[50];
	char *decimal = "POS:[%3d,%3d] [%2d,%2d] ADDR: %5d %5d %5d";
	char *hexadecimal = "POS:[%02X,%02X] [%02X,%02X] ADDR: #%4X #%4X #%4X";

	XSetForeground(display, gc, 0xFFFFFFL);
	XFillRectangle(display, win, gc, 0, scale*192, scale*256, 16);
	XSetForeground(display, gc, 0L);
	sprintf(info, hex ? hexadecimal : decimal,
		x, y, x/8, y/8,
		16384 + getPixOffset(x, y), 16384 + getPixOffset(x, y&(~7)), 16384 + getAttrOffset(x, y));
	XDrawString(display, win, gc, 0, scale*192+12, info, strlen(info));
}

void drawGrid(XImage *img, short unsigned int scale) {
	long yellow = 0xC0C000L;
	long gray = 0xD8D8D8L;
	int x,y;

	for (x = 0; x < scale*256; x++) {
		XPutPixel(img, x, 64*scale, yellow);
		XPutPixel(img, x, 128*scale, yellow);
	}

	for (x = 0; x < 32; x++)
		for (y = 0; y < scale*192; y++)
			XPutPixel(img, scale*x*8, y, gray);

	for (y = 0; y < 24; y++)
		for (x = 0; x < scale*256; x++)
			XPutPixel(img, x, scale*y*8, gray);

}

int main (int argc, char **argv) { 
	char * filename = NULL;
	FILE * screen_file = NULL;
	unsigned char *data;
	char *pixmap_data;
	int screen_num;
	Visual *visual;
	XImage *img;
	XEvent event;
	short unsigned int loop = 1;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-scale") == 0) {
			if (i + 2 >= argc) {
				usage(argv[0]);
				return -1;
			}
			scale = atoi(argv[i + 1]);
			i += 1;
			continue;
		}
		if (strcmp(argv[i], "-hex") == 0) {
			if (i + 2 >= argc) {
				usage(argv[0]);
				return -1;
			}
			hex = atoi(argv[i + 1]) ? 1 : 0;
			i += 1;
			continue;
		}
		if (strcmp(argv[i], "-grid") == 0) {
			if (i + 2 >= argc) {
				usage(argv[0]);
				return -1;
			}
			grid = atoi(argv[i + 1]) ? 1 : 0;
			i += 1;
			continue;
		}
		filename = argv[i];
	}

	if (filename == NULL) {
		usage(argv[0]);
		return -1;
	}

	data = malloc(6912);
	pixmap_data = malloc(4*256*192*scale*scale);

	screen_file = fopen(filename, "rb");
	if (screen_file == NULL) {
		printf("Can't open screen file!\n");
		return -2;
	}
	fread(data, 6912, 1, screen_file);
	fclose(screen_file);

	display = XOpenDisplay(NULL);
	screen_num = DefaultScreen(display);
	root = RootWindow(display, screen_num);
	visual = DefaultVisual(display, screen_num);
	img = XCreateImage(display, visual, DefaultDepth(display, screen_num), ZPixmap,
			0, pixmap_data, scale*256, scale*192, 32, 0);

	if (img == NULL ) {
		printf("can't create image\n");
		return -1;
	}

	drawSpeccyScreen(img, data, scale);
	if (scale > 1 && grid)
		drawGrid(img, scale);

	win = XCreateSimpleWindow(display, root, 0, 0, scale*256, scale*192+16, 1, 0, 0);
	XSelectInput(display, win, ExposureMask);
	XMapWindow(display, win);
	XSelectInput(display, win, ExposureMask | PointerMotionMask | KeyPressMask);

	while(loop) {
		XNextEvent(display, &event);
		if (event.type == Expose) {
			gc = DefaultGC(display, screen_num);
			XPutImage(display, win, gc, img, 0, 0, 0, 0, scale*256, scale*192);
			updateInfo(mouse_x, mouse_y);
		}
		if (event.type == MotionNotify) {
			XMotionEvent *xm = (XMotionEvent*)&event;
			mouse_x = xm->x/scale;
			mouse_y = xm->y/scale;

			drawSpeccyScreen(img, data, scale);
			if (scale > 1 && grid)
				drawGrid(img, scale);
			XPutImage(display, win, gc, img, 0, 0, 0, 0, scale*256, scale*192);
			updateInfo(mouse_x, mouse_y);
		}
		if (event.type == KeyPress) {
			if (event.xkey.keycode == 9 || event.xkey.keycode == 24) /* ESC + Q */
				loop = 0;
			if (event.xkey.keycode == 43) { /* H */
				hex ^= 1;
				updateInfo(mouse_x, mouse_y);
			}
			if (event.xkey.keycode == 42) { /* G */
				grid ^= 1;
				drawSpeccyScreen(img, data, scale);
				if (scale > 1 && grid)
					drawGrid(img, scale);
				XPutImage(display, win, gc, img, 0, 0, 0, 0, scale*256, scale*192);
			}
		}
	}

	//sleep(10);
	XCloseDisplay(display);

	free(data);
	free(pixmap_data);
}

