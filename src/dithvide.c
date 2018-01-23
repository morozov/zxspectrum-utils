#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define	OK 0
#define VERSION "1.6"
#define LABEL	"DithvIDE"
#define USAGE	"[-aATTRHEIGHT] [-lMINSAT] [-hMAXSAT] [-mMODEL] [-cP1,I1,P2,I2[,B1,B2]] [-pPENALTY] [-oOUTFILE] [-bBRIGHTRATIO] [-grey] [-dark] [-vic] file1.jpg [file2.jpg ...]\nClick on the image to change selection [left button, right button, enter, space]"

#define	HR_X	320
#define	HR_Y	200
#define HR_L	15
#define HR_M	2
#define ZX_X	256
#define ZX_Y	192
#define ZX_L	7
#define ZX_M	2

#define VR_X	320
#define VR_Y	200
#define	PA_X	8
#define	PA_Y	8
#define MAXVAL	255
#define BRIGHTR 66
#define BRIGHT	64

#define OUTTAP	"dithvide.tap"
#define OUTVIC	"dithvide.vic"
#define ZX_P	40000
#define HR_P	1000
#define GR_P	10

#define	ENDKEY	36
#define ORIGKEY	65
#define LBUTTON	1
#define RBUTTON	3
#define XORVAL	0xff
#define MAXDIST 0xfffffff

#define LOSATUR	0
#define HISATUR 255

#define RI	45875
#define GI	-38666
#define BI	-7209

#define RG	299
#define GG	587
#define BG	114
#define SH	1000

static unsigned char tapprefix[24]={0x13,0x0,0x0,0x3,'D','i','t','h','v','I','D','E',' ',0xaa,0x0,0x0,0x0,0x40,0x0,0x80,0x46,0x02,0x0,0xff};

struct rgbpixel {int r, g, b;};

static char *zx_models[ZX_M]={"RGB saturated","PAL measured"};

static struct rgbpixel zx_palls[ZX_M][2*(ZX_L+1)]=
{
	{
		{0x00,0x00,0x00},
		{0x00,0x00,0xff},
		{0xff,0x00,0x00},
		{0xff,0x00,0xff},
		{0x00,0xff,0x00},
		{0x00,0xff,0xff},
		{0xff,0xff,0x00},
		{0xff,0xff,0xff}
	},
	{
		{0x00,0x00,0x00},
		{0x10,0x10,0xb0},
		{0xb0,0x00,0x00},
		{0xb0,0x30,0xe0},
		{0x20,0xa0,0x20},
		{0x30,0xd0,0xff},
		{0xd0,0xd0,0x40},
		{0xff,0xff,0xff}
	}
};

static char *hr_models[HR_M]={"CCS64 default","VIC-II by Pepto"};

static struct rgbpixel hr_palls[HR_M][HR_L+1]=
{
	{
		{0x00,0x00,0x00},
		{0xff,0xff,0xff},
		{0x68,0x37,0x2b},
		{0x70,0xa4,0xb2},
		{0x6f,0x3d,0x86},
		{0x58,0x8d,0x43},
		{0x35,0x28,0x79},
		{0xb8,0xc7,0x6f},
		{0x6f,0x4f,0x25},
		{0x43,0x39,0x00},
		{0x9a,0x67,0x59},
		{0x44,0x44,0x44},
		{0x6c,0x6c,0x6c},
		{0x9a,0xd2,0x84},
		{0x6c,0x5e,0xb5},
		{0x95,0x95,0x95}
	},
	{
		{0x19,0x1d,0x19},
		{0xfc,0xf9,0xfc},
		{0x93,0x3a,0x4c},
		{0xb6,0xfa,0xfa},
		{0xd2,0x7d,0xed},
		{0x6a,0xcf,0x6f},
		{0x4f,0x44,0xd8},
		{0xfb,0xfb,0x8b},
		{0xd8,0x9c,0x5b},
		{0x7f,0x53,0x07},
		{0xef,0x83,0x9f},
		{0x57,0x57,0x53},
		{0xa3,0xa7,0xa7},
		{0xb7,0xfb,0xbf},
		{0xa3,0x97,0xff},
		{0xef,0xf9,0xe7}
	}
};

static struct rgbpixel zx_pall[2*(ZX_L+1)];
static struct rgbpixel hr_pall[HR_L+1];

struct multiatr {unsigned char b[2][PA_Y], a[2]; struct rgbpixel base[4];};

char t[1024]=LABEL;
Display *dis;
Window root;
Window win;
XImage *xim;
GC gc;
XSetWindowAttributes wattr;
struct rgbpixel base[4], pure[4];
struct rgbpixel *pall=zx_pall;
struct multiatr atr[VR_Y][VR_X/PA_X];
int vic=0;
int rr;
int brr=0;
int brt=BRIGHT;
int los=LOSATUR;
int pen=ZX_P;
int his=HISATUR;
int col=0;
int pls=ZX_L;
int ay=PA_Y;
int wlong;
int vrx=ZX_X;
int vry=ZX_Y;
int bpp=4;
int wx, wy;
int scr, dpt;

int cols[6];

struct rgbpixel rgb[VR_Y+1][VR_X+2], *ptmp;
struct rgbpixel *rtmp;
int rg,rb;
struct jpeg_decompress_struct jdecs;
unsigned char *bram;
unsigned long wmask;

void err(char *s)
{
	printf("\nERROR: %s\n",s);
	exit(1);
}

void ban(char * s)
{
	t[sizeof(LABEL)-1]=0;
	strcat(t," ");
	strcat(t,VERSION);
	strcat(t,": ");
	strcat(t,s);
	XStoreName(dis,win,t);
}

void conv16(unsigned char *p, int r, int g, int b)
{
	*(unsigned short *)p=((r>>3)<<11) + ((g>>2)<<5) + (b>>3);
}

void conv32(unsigned char *p, int r, int g, int b)
{
	*(unsigned long *)p=(r<<16)+(g<<8)+b;
}    

static void __inline__ purecols(int a, int b, int c, int d, int e, int f)
{
	memcpy(pure,pall+a+e,sizeof(struct rgbpixel));
	memcpy(pure+1,pall+b+e,sizeof(struct rgbpixel));
	memcpy(pure+2,pall+c+f,sizeof(struct rgbpixel));
	memcpy(pure+3,pall+d+f,sizeof(struct rgbpixel));
}

static void __inline__ basecols(void)
{
	int i, j, k=0;
	for(i=0;i<2;i++)
		for(j=2;j<4;j++)
		{
			base[k].r=(pure[i].r+pure[j].r)>>1;
			base[k].g=(pure[i].g+pure[j].g)>>1;
			base[k].b=(pure[i].b+pure[j].b)>>1;
			k++;
		}
}

void decodeatr(int y, int x)
{
	int a1=atr[y][x].a[0], a2=atr[y][x].a[1];
	if(vic)
		purecols(HR_L&a1,HR_L&(a1>>4),HR_L&a2,HR_L&(a2>>4),0,0);
	else
		purecols(ZX_L&(a1>>3),ZX_L&a1,ZX_L&(a2>>3),ZX_L&a2,(BRIGHT&~a1)>>3,(BRIGHT&~a2)>>3);
}

void savecols(int a, int b, int c, int d, int e, int f, int x, int y)
{
	memcpy(atr[y][x].base,base,sizeof(base));
	if(vic)
	{
		atr[y][x].a[0]=(b<<4)+a;
		atr[y][x].a[1]=(d<<4)+c;
	}
	else
	{
		atr[y][x].a[0]=(a<<3)+b+(brr?(e?0:BRIGHT):brt);
		atr[y][x].a[1]=(c<<3)+d+(brr?(f?0:BRIGHT):brt);
	}
}



static long __inline__ ratecols(void)
{
	int dist, pick, p=0, k;
	int r,g,b;
	long s=0;
	dist=MAXDIST;
	rr+=rtmp->r;
	rg+=rtmp->g;
	rb+=rtmp->b;
	if(rr<los) {rr=los; s+=pen;}
	if(rr>his) {rr=his; s+=pen;}
	if(rg<los) {rg=los; s+=pen;}
	if(rg>his) {rg=his; s+=pen;}
	if(rb<los) {rb=los; s+=pen;}
	if(rb>his) {rb=his; s+=pen;}
	for(k=4;k--;)
	{
		r=base[k].r-rr;
		g=base[k].g-rg;
		b=base[k].b-rb;
		pick=r*r+g*g+b*b;;
		if(pick<dist)
		{
			p=k;
			dist=pick;
		}
	}
	rr-=base[p].r;
	rg-=base[p].g;
	rb-=base[p].b;
	s+=dist;
	return s;
}

void cmpatr(int y, int x)
{
	int i,j;
	int a,b,c,d, e=0, f=0;
	long suma, diff=MAXDIST;


	if(col)
	{
		purecols(cols[0],cols[1],cols[2],cols[3],cols[4],cols[5]);
		basecols();
		savecols(cols[0],cols[1],cols[2],cols[3],cols[4],cols[5],x,y);
		return;
	}
	do{
		for(a=0;a<pls;a++)
			for(c=0;c<pls;c++)
				for(b=pls;b>a;b--)
					for(d=pls;d>c;d--)
					{
						purecols(a,b,c,d,e,f);
						basecols();
						suma=0;
						rr=rg=rb=0;
						for(i=ay*y;i<ay*(y+1);i++)
						{
							rtmp=((PA_X-1)*(i&1))+rgb[i]+PA_X*x+1;
							for(j=PA_X;j--;)
							{
								suma+=ratecols();
								if(suma>=diff) break;
								rtmp+=1-2*(i&1);
							}		
							if(suma>=diff) break;
						}
						for(j=PA_X*x;j++<PA_X*x+PA_X;)
						{
							rtmp=rgb[ay*y+(ay-1)*(j&1)]+j;
							for(i=ay;i--;)
							{
								suma+=ratecols();
								if(suma>=diff) break;
								rtmp+=(VR_X+2)*(1-2*(j&1));
							}		
							if(suma>=diff) break;
						}		  
						if(suma<diff)
						{
							diff=suma;
							savecols(a,b,c,d,e,f,x,y);
						}
					}
		if(e&f) break;
		if(f) e=f;
		f=pls+1;
	}while(brr);
}

void killwindow(void)
{
	XDestroyImage(xim);
	XFreeGC(dis,gc);
	XUnmapWindow(dis,win);
	XDestroyWindow(dis,win);
	XFlush(dis);
}

void rgbadd(unsigned long *d, int x, int y, int m, int n)
{
	int c;
	if(m>=0)
	{
		m=wlong-m;
		if(m>vrx) m=vrx;
	}
	else
	{
		m+=vrx;
		if(m<0)
			m=0;
	}
	if(n>=0)
	{
		n=wlong-n;
		if(n>vrx) n=vrx;
	}
	else
	{
		n+=vrx;
		if(n<0)
			n=0;
	}    
	c=m*n;
	rgb[y][x].r+=c*((unsigned char *)d)[2]/(vrx*vrx);
	rgb[y][x].g+=c*((unsigned char *)d)[1]/(vrx*vrx);
	rgb[y][x].b+=c*((unsigned char *)d)[0]/(vrx*vrx);
}

void xorme(unsigned char *t)
{
	int c=bpp;
	while(c--)
		t[c]^=XORVAL;
}

void wtoggle(void)
{
	int i, h=jdecs.output_width-wx, v=vry*wlong/vrx-2;
	unsigned char *t=bram+bpp*(jdecs.output_width*wy+wx), *d;

	if(h>wlong)
		h=wlong;
	d=t;
	for(i=h;i--;d+=bpp)
		xorme(d);
	d=t+bpp*jdecs.output_width;
	i=wy+1;
	while(v-- && i<jdecs.output_height)
	{
		xorme(d);
		if(h==wlong)
			xorme(d+bpp*(h-1));
		d+=bpp*jdecs.output_width;
		i++;
	}
	if(i<jdecs.output_height)
		for(i=h;i--;d+=bpp)
			xorme(d);
}

void popwindow(unsigned char *d, int a, int b, char *s)
{
	wattr.border_pixel=BlackPixel(dis,scr);
	wattr.background_pixel=BlackPixel(dis,scr);
	wmask=CWBackPixel|CWBorderPixel;
	win=XCreateWindow(dis,root,0,0,a,b,0,dpt,InputOutput,CopyFromParent,wmask,&wattr);
	XMapWindow(dis,win);
	gc=XCreateGC(dis,win,0,0);
	xim=XCreateImage(dis,CopyFromParent,dpt,ZPixmap,0,(char*)d,a,b,8*bpp,a*bpp);
	ban(s);
}



int main(int argc, char **argv)
{
	struct jpeg_error_mgr jerr;

	static int grey=0;
	static int modno=0;
	static int actarg=0;
	static char *ofn=OUTTAP;
	static char *cls="automatic";
	static int of=1;
	static int pns=1;
	static int bri=BRIGHTR;

	char u[1024];
	char *modname;

	int i, j, k, l=0, m, n, o, p=0, q, r, s;

	JSAMPARRAY buff;
	JSAMPLE *btmp;
	FILE	*file, *tap;

	XEvent xev;
	unsigned long *data, *dtmp;
	unsigned char *vram, *vtmp;


	unsigned char c, d;

	static void (*conv)(unsigned char *, int, int, int);





	while(++actarg<argc && *argv[actarg]=='-')
		switch(argv[actarg][1])
		{
			case 'c':
			case 'C': 
				k=1; c=0;
				do{
					k++;
					d=argv[actarg][k];
					if(d>='0' && d<='9')
						c=10*c+d-'0';
					else
					{cols[col]=c;col++;c=0;}
				}while(d && col<6);
				cls=argv[actarg]+2;
				break;
			case 'b':
			case 'B': bri=brr=atoi(argv[actarg]+2); if(brr<0 || brr>100) err("Bad bright ratio in -b switch"); 
				  break;
			case 'a':
			case 'A': ay=atoi(argv[actarg]+2); n=ay; while(n && n<0x100) n<<=1; if(!n || n>0x100 || ay>PA_Y) err("Bad attribut height in -a switch"); break;
			case 'v':
			case 'V': vic=1; vrx=HR_X; vry=HR_Y; pall=hr_pall; pls=HR_L; if(of) ofn=OUTVIC; break;
			case 'd':
			case 'D': brt=0; break;
			case 'o':
			case 'O': of=0; ofn=argv[actarg]+2; break;
			case 'l':

			case 'L': los=atoi(argv[actarg]+2); break;
			case 'h':
			case 'H': his=atoi(argv[actarg]+2); break;
			case 'p':
			case 'P': pns=0; pen=atoi(argv[actarg]+2); break;
			case 'm':
			case 'M': modno=atoi(argv[actarg]+2); break;
			case 'g':
			case 'G': grey=1; break;
			default: printf("Unknown switch '%s' ignored\n",argv[actarg]);
		}
	if(modno<0)
		err("Negative model number");
	if(vic)
	{
		brr=0;
		if(pns) pen=HR_P;
		if(modno>=HR_M)
			err("Undefined C-64 palette variation");
		memcpy(hr_pall,hr_palls[modno],sizeof(hr_pall));
		modname=hr_models[modno];
	}
	else
	{
		if(modno>=ZX_M)
			err("Undefined ZX palette variation");
		memcpy(zx_pall,zx_palls[modno],sizeof(zx_pall));
		modname=zx_models[modno];
	}  
	if(actarg>=argc)
	{
		printf("USAGE: %s %s",*argv,USAGE);
		err("Not enough arguments");
	}
	if(col)
	{
		if(col!=(brr?6:4))
			err("Bad number of colors in -c switch");
		for(c=4;c--;)
			if(cols[c]>pls) err("Color out of palette range in -c switch");
		if(brr)
		{
			for(i=5;i>3;i--)
				if(cols[i]) cols[i]=0;
				else cols[i]=ZX_L+1;
		}
	}
	if(grey)
	{
		if(pns) pen=GR_P;
		for(k=pls+1;k--;)
		{
			i=(RG*pall[k].r+GG*pall[k].g+BG*pall[k].b)/SH;
			pall[k].r=i;
			pall[k].g=i;
			pall[k].b=i;
		}
	}
	for(k=ZX_L+1;k--;)
	{
		zx_pall[k+ZX_L+1].r=bri*zx_pall[k].r/100;
		zx_pall[k+ZX_L+1].g=bri*zx_pall[k].g/100;
		zx_pall[k+ZX_L+1].b=bri*zx_pall[k].b/100;
	}
	dis=XOpenDisplay(NULL);
	scr=DefaultScreen(dis);
	conv=conv32;
	if((dpt=DefaultDepth(dis,scr))<16)
		err("Insufficient display depth, use at least 16 bits");
	if(dpt==16)
	{
		bpp=2;
		conv=conv16;
	}

	printf("Version: %s\nDisplay depth: %d (%d Bpp)\n%s output file: %s\n",VERSION,dpt,bpp,vic?"C-64 VRAM":"ZX Spectrum",ofn);
	if(!(tap=fopen(ofn,"ab")))
		err("Cannot open output file");
	c=0x18+((vry*vrx)/(PA_X*0x100))/ay;
	tapprefix[15]=tapprefix[22]=c;
	tapprefix[20]^=c;
	root=DefaultRootWindow(dis);
	while(actarg<argc)
	{
		if((file=fopen(argv[actarg],"rb")))
		{
			printf("\nImage: %s\n",argv[actarg]);
			jpeg_create_decompress(&jdecs);
			jpeg_stdio_src(&jdecs,file);
			jdecs.err=jpeg_std_error(&jerr);
			jpeg_read_header(&jdecs,FALSE);
			jpeg_start_decompress(&jdecs);
			printf("Size: %d x %d\nAttribut: %d x %d\nSaturation: %d..%d\nPenalty: %d\nColors: %s\nBright: %d%%, %s\nPalette: %s, %s\n",jdecs.output_width,jdecs.output_height,PA_X,ay,los,his,pen,cls,bri,vic?"not used":(brr?"computed":(brt?"light":"dark")),modname,grey?"grey":"color");
			if((data=(unsigned long *)malloc(jdecs.output_width*jdecs.output_height*sizeof(unsigned long)))
					&& (bram=(unsigned char *)malloc(jdecs.output_width*jdecs.output_height*bpp))
					&& (vram=(unsigned char *)malloc(vrx*vry*bpp)))
			{
				popwindow(bram,jdecs.output_width,jdecs.output_height,argv[actarg]);
				buff=jdecs.mem->alloc_sarray((j_common_ptr)&jdecs,JPOOL_IMAGE,jdecs.output_width*jdecs.output_components,1);
				dtmp=data;
				vtmp=bram;
				while(jdecs.output_scanline<jdecs.output_height)
				{
					jpeg_read_scanlines(&jdecs,buff,1);
					btmp=buff[0];
					for(i=0;i<jdecs.output_width;i++)
					{
						*dtmp++=(*btmp<<16) + (btmp[1]<<8) + btmp[2];
						conv(vtmp,*btmp,btmp[1],btmp[2]);
						btmp+=jdecs.output_components;
						vtmp+=bpp;
					}
				}
				wx=0; wy=0; wlong=vrx;
				wtoggle();
				XPutImage(dis,win,gc,xim,0,0,0,0,jdecs.output_width,jdecs.output_height);
				XSelectInput(dis,win,ExposureMask|KeyPressMask|ButtonPressMask);
				XFlush(dis);

				while(i)
				{
					XNextEvent(dis,&xev);
					switch(xev.type)
					{
						case Expose:
							XPutImage(dis,win,gc,xim,0,0,0,0,jdecs.output_width,jdecs.output_height);
							XFlush(dis);
							break;
						case KeyPress:
							i=(xev.xkey.keycode!=ENDKEY);
							if(xev.xkey.keycode==ORIGKEY)
							{
								wtoggle();
								wlong=vrx;
								wtoggle();
								XPutImage(dis,win,gc,xim,0,0,0,0,jdecs.output_width,jdecs.output_height);
								XFlush(dis);
							}
							break;
						case ButtonPress:
							wtoggle();
							switch(xev.xbutton.button)
							{
								case LBUTTON: wx=xev.xbutton.x; wy=xev.xbutton.y; break;
								case RBUTTON: wlong=(xev.xbutton.x-wx>(j=vrx*(xev.xbutton.y-wy)/vry))?xev.xbutton.x-wx:j;
									      if(wlong<vrx)
										      wlong=vrx;
							}
							wtoggle();
							XPutImage(dis,win,gc,xim,0,0,0,0,jdecs.output_width,jdecs.output_height);
							XFlush(dis);
					}
				}
				wtoggle();
				XPutImage(dis,win,gc,xim,0,0,0,0,jdecs.output_width,jdecs.output_height);
				XFlush(dis);
				bzero(rgb,sizeof(rgb));
				bzero(atr,sizeof(atr));
				i=0; j=0; m=wy;
				while(i<vry && m<jdecs.output_height)
				{
					dtmp=data+jdecs.output_width*m+wx;
					k=0; l=0; n=wx;
					while(k<vrx && n<jdecs.output_width)
					{
						rgbadd(dtmp,k+1,i,l,j);
						rgbadd(dtmp,k+2,i,l-wlong,j);
						rgbadd(dtmp,k+1,i+1,l,j-wlong);
						rgbadd(dtmp,k+2,i+1,l-wlong,j-wlong);
						l+=vrx;
						if(l>=wlong)
						{
							l-=wlong;
							k++;
						}
						dtmp++;
						n++;
					}
					j+=vrx;
					if(j>=wlong)
					{
						j-=wlong;
						i++;
					}
					m++;
				}
				vtmp=vram; ptmp=(struct rgbpixel *)rgb+1;
				for(i=vry;i--;ptmp+=2+VR_X-vrx)
					for(j=vrx;j--;)
					{
						k=(vrx*vrx)*ptmp->r/wlong/wlong;
						l=(vrx*vrx)*ptmp->g/wlong/wlong;
						m=(vrx*vrx)*ptmp->b/wlong/wlong;
						if(grey)
							k=l=m=(RG*k+GG*l+BG*m)/SH;
						ptmp->r=k;
						ptmp->g=l;
						ptmp->b=m;
						conv(vtmp,k,l,m);
						vtmp+=bpp;
						ptmp++;
					}
				killwindow();
				free(data);
				printf("Selection: %d..%d x %d..%d\nScaling: %d%% to %d x %d\n",wx,wx+wlong-1,wy,wy-1+(wlong*vry)/vrx,100*vrx/wlong,vrx,vry);
				sprintf(u,"Scaled to %d x %d",vrx,vry);
				popwindow(vram,vrx,vry,u);
				XPutImage(dis,win,gc,xim,0,0,0,0,vrx,vry);
				XSelectInput(dis,win,ExposureMask|KeyPressMask);
				XFlush(dis);
				n=1; m=1;
				while(n)
				{
					XNextEvent(dis,&xev);
					switch(xev.type)
					{
						case Expose:
							XPutImage(dis,win,gc,xim,0,0,0,0,vrx,vry);
							XFlush(dis);
							break;
						case KeyPress:
							if(xev.xkey.keycode==ENDKEY && !m)
								n=0;
							if(xev.xkey.keycode==ORIGKEY && !m)
							{
								vtmp=vram;
								for(i=0;i<vry;i++)
									for(j=0;j<vrx/PA_X;j++)
									{
										decodeatr(i/ay,j);
										basecols();
										r=atr[i/ay][j].b[0][i%ay];
										s=atr[i/ay][j].b[1][i%ay];
										o=(l&1)?s:r;

										for(k=8;k--;)
										{
											switch(l)
											{
												case 0:
												case 1: r=(1&(o>>7))+(2&(l<<1));
													conv(vtmp,pure[r].r,pure[r].g,pure[r].b); break;
												case 2:
												case 3: r=(o&128)?0:MAXVAL; conv(vtmp,r,r,r); break;
												case 4: o=((MAXVAL*(3-((2&(r>>6))+(1&(s>>7)))))/3); conv(vtmp,o,o,o); break;
												default: o=(2&(r>>6))+(1&(s>>7)); conv(vtmp,base[o].r,base[o].g,base[o].b);
											}
											r<<=1;
											s<<=1; 
											o<<=1;
											vtmp+=bpp;
										}


									}
								XPutImage(dis,win,gc,xim,0,0,0,0,vrx,vry);
								switch(l)
								{
									case 0: ban("Even VRAM"); break;
									case 1: ban("Odd VRAM"); break;
									case 2: ban("Even bitmap"); break;
									case 3: ban("Odd bitmap"); break;
									case 4: ban("Bitmap contents"); break;
									default: ban("Average VRAM");
								}
								XFlush(dis);
								l++;
								if(l>5) l=0;
							}	
							if(xev.xkey.keycode==ORIGKEY && m)
							{
								ban("Color matching...");
								for(i=vry/ay;i--;)
								{
									XFlush(dis);
									for(j=vrx/PA_X;j--;)
										cmpatr(i,j);
									vtmp=vram+bpp*ay*vrx*i;
									for(p=ay;p--;)
										for(l=0;l<vrx/PA_X;l++)
										{
											memcpy(base,atr[i][l].base,sizeof(base));
											for(o=0;o<PA_X;vtmp+=bpp,o++)
												conv(vtmp,base[o>>1].r,base[o>>1].g,base[o>>1].b);
										}							
									XPutImage(dis,win,gc,xim,0,0,0,0,vrx,vry);

								}
								printf("Mapped, ");
								ban("F-S dithering...");
								XFlush(dis);
								fflush(stdout);
								vtmp=vram; ptmp=(struct rgbpixel *)rgb;
								for(i=0;i<vry;i++)
								{
									if(i&1)
										ptmp--;
									else
										ptmp++;
									for(j=0;j<vrx;j++)
									{
										if(!(j%PA_X))
										{
											if(i&1)
												memcpy(base,atr[i/ay][(vrx-1-j)/PA_X].base,sizeof(base));
											else
												memcpy(base,atr[i/ay][j/PA_X].base,sizeof(base));
										}
										if(ptmp->r<los) ptmp->r=los;
										if(ptmp->r>his) ptmp->r=his;
										if(ptmp->g<los) ptmp->g=los;
										if(ptmp->g>his) ptmp->g=his;
										if(ptmp->b<los) ptmp->b=los;
										if(ptmp->b>his) ptmp->b=his;
										l=MAXDIST;
										for(k=4;k--;)
										{
											m=(base[k].r-ptmp->r)*(base[k].r-ptmp->r)+(base[k].g-ptmp->g)*(base[k].g-ptmp->g)+(base[k].b-ptmp->b)*(base[k].b-ptmp->b);
											if(m<l)
											{
												l=m;
												p=k;
											}
										}
										if(i&1)
										{
											atr[i/ay][(vrx-1-j)/PA_X].b[0][i%ay]=((p&2)<<6)+(atr[i/ay][(vrx-1-j)/PA_X].b[0][i%ay]>>1);
											atr[i/ay][(vrx-1-j)/PA_X].b[1][i%ay]=((p&1)<<7)+(atr[i/ay][(vrx-1-j)/PA_X].b[1][i%ay]>>1);
											o=(ptmp->r-base[p].r);
											(ptmp-1)->r+=7*o/16;
											(ptmp+VR_X+3)->r+=3*o/16;
											(ptmp+VR_X+2)->r+=5*o/16;
											(ptmp+VR_X+1)->r+=o/16;
											o=(ptmp->g-base[p].g);
											(ptmp-1)->g+=7*o/16;
											(ptmp+VR_X+3)->g+=3*o/16;
											(ptmp+VR_X+2)->g+=5*o/16;
											(ptmp+VR_X+1)->g+=o/16;
											o=(ptmp->b-base[p].b);
											(ptmp-1)->b+=7*o/16;
											(ptmp+VR_X+3)->b+=3*o/16;
											(ptmp+VR_X+2)->b+=5*o/16;
											(ptmp+VR_X+1)->b+=o/16;
											vtmp-=bpp;
											conv(vtmp,base[p].r,base[p].g,base[p].b );
											ptmp--;
										}
										else
										{
											atr[i/ay][j/PA_X].b[0][i%ay]=((p&2)>>1)+(atr[i/ay][j/PA_X].b[0][i%ay]<<1);
											atr[i/ay][j/PA_X].b[1][i%ay]=(p&1)+(atr[i/ay][j/PA_X].b[1][i%ay]<<1);
											o=(ptmp->r-base[p].r);
											(ptmp+1)->r+=7*o/16;
											(ptmp+VR_X+1)->r+=3*o/16;
											(ptmp+VR_X+2)->r+=5*o/16;
											(ptmp+VR_X+3)->r+=o/16;
											o=(ptmp->g-base[p].g);
											(ptmp+1)->g+=7*o/16;
											(ptmp+VR_X+1)->g+=3*o/16;
											(ptmp+VR_X+2)->g+=5*o/16;
											(ptmp+VR_X+3)->g+=o/16;
											o=(ptmp->b-base[p].b);
											(ptmp+1)->b+=7*o/16;
											(ptmp+VR_X+1)->b+=3*o/16;
											(ptmp+VR_X+2)->b+=5*o/16;
											(ptmp+VR_X+3)->b+=o/16;
											conv(vtmp,base[p].r,base[p].g,base[p].b);
											vtmp+=bpp;
											ptmp++;
										}
									}
									ptmp+=VR_X+2;
									vtmp+=vrx*bpp;
								}
								printf("dithered, ");
								fflush(stdout);

								if(!col)
								{
									ban("VRAM reordering...");
									XFlush(dis); 
									for(i=vry/ay;i--;)
										for(j=vrx/PA_X;j--;)
										{

											for(q=2;q--;)
											{
												r=s=0;
												for(k=ay;k--;)
													for(l=128;l;l>>=1)
														if(atr[i][j].b[q][k]&l) s++; else r++;
												if(s>r)
												{
													l=atr[i][j].a[q];
													if(vic)
														l=((HR_L&l)<<4) + ((l>>4)&HR_L);
													else
														l=((ZX_L&l)<<3) + ((l>>3)&ZX_L) + (l&BRIGHT);
													atr[i][j].a[q]=l;
													for(k=ay;k--;)
														atr[i][j].b[q][k]=~atr[i][j].b[q][k];
												}
											}
											r=s=0;
											decodeatr(i,j);
											m=RI*pure[1].r+GI*pure[1].g+BI*pure[1].b;
											n=RI*pure[0].r+GI*pure[0].g+BI*pure[0].b;
											o=RI*pure[3].r+GI*pure[3].g+BI*pure[3].b;
											p=RI*pure[2].r+GI*pure[2].g+BI*pure[2].b;
											for(k=ay;k--;)
												for(l=128;l;l>>=1)
												{
													r+=atr[i][j].b[0][k]&l?m:n;
													s+=atr[i][j].b[1][k]&l?o:p;
												}
											if(r>s)
											{
												q=atr[i][j].a[0];
												atr[i][j].a[0]=atr[i][j].a[1];
												atr[i][j].a[1]=q;
												for(k=ay;k--;)
												{
													q=atr[i][j].b[0][k];
													atr[i][j].b[0][k]=atr[i][j].b[1][k];
													atr[i][j].b[1][k]=q;
												}
											}
										}
									printf("ordered, ");
								}
								ban("Conversion finished");
								XPutImage(dis,win,gc,xim,0,0,0,0,vrx,vry);
								fflush(stdout);
								XFlush(dis);
								m=0; l=0; n=1;
							}
					}
				}
				killwindow();
				for(m=2;m--;)
				{
					if(vic)
					{
						for(j=0;j<vry/PA_Y;j++)
							for(i=0;i<vrx/PA_X;i++)
								for(l=0;l<PA_Y/ay;l++)
									for(k=0;k<ay;k++)
										fputc(atr[(PA_Y/ay)*j+l][i].b[m][k],tap);
					}
					else
					{
						c=0xff;
						fwrite(tapprefix,sizeof(tapprefix),1,tap);
						for(l=0;l<3;l++)
							for(n=0;n<PA_Y/ay;n++)
								for(k=0;k<ay;k++)
									for(j=0;j<PA_Y;j++)
										for(i=0;i<vrx/PA_X;i++)
										{
											d=atr[(vry/(3*ay))*l+(PA_Y/ay)*j+n][i].b[m][k];
											fputc(d,tap);
											c=c^d;
										}
					}
					for(j=0;j<vry/ay;j++)
						for(i=0;i<vrx/PA_X;i++)
						{
							d=atr[j][i].a[m];
							fputc(d,tap);
							c^=d;
						}
					if(!vic) fputc(c,tap);
				}
				printf("saved.\n");
			}
			else
				printf("Not enough memory for viewing\n");
			jpeg_finish_decompress(&jdecs);
			jpeg_destroy_decompress(&jdecs);
			fclose(file);
		}
		else
			printf("\nCannot open image: %s\n",argv[actarg]);
		actarg++;
	}
	fclose(tap);
	return OK;
}
