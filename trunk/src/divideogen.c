#include <stdlib.h>
#include <png.h>
#include <string.h>
#include <strings.h>

#define OK	0
#define ARGNO	2
#define USAGE	"first last [equ|- [frm|- [shr|- [col|- avg|- [y-size]]]]]\n\
	first\tnumber, the first xxxxxxxx.png to process\n\
	last\tnumber, the last xxxxxxxx.png to process\n\
	equ\tswitch, turn on luminance equalisation\n\
	frm\tswitch, turn on interframe error-correcting bias\n\
	shr\tnumber, set the range-shrink ceiling\n\
	col\tswitch, turn on RGB separate qualisation\n\
	avg\tswitch, turn on interframe error-correcting attributsmooth\n\
	y-size\tnumber, the real y-size, taken from image"

#define STREAM	"stream.bin"
#define TAP	"stream.tap"

#define ZX_X	64
#define ZX_Y	48
#define CHA_X	2
#define CHA_Y	2

#define RI	19456
//19595
#define GI	38144
//38469
#define BI	7936
//7471

#define SHRANGE	64

#define MAXVAL	0x100
#define BRIGHT	0x40
#define BDFLAG	0xff

unsigned char tapprefix[24]=
{0x13,0x0,0x0,0x3,'D','i','v','I','D','E','o',' ',' ',' ',
	0x0,0x9,0x0,0x80,0x0,0x80,0x56,0x2,0x9,0xff};

int sh_ran=SHRANGE;

typedef struct zx_pixel
{
	int r, g, b, y;
} zx_pixel;

void err(char *s)
{
	printf("ERROR: %s\n",s);
	exit(1);
}

void zx_contrib(zx_pixel *z, png_bytep  p, int n)
{
	z->r+=n*(*p++)/(ZX_X*ZX_Y);
	z->g+=n*(*p++)/(ZX_X*ZX_Y);
	z->b+=n*(*p)/(ZX_X*ZX_Y);
}

int zx_huelum(zx_pixel *z, int *o, int a)
{
	int i, j, k, *p;
	int rgb[3];

	bzero(rgb,3*sizeof(int));
	while(a--)
	{
		j=0;
		p=&(z->b);
		for(i=3;i--;p--)
			if(*p>=j && !rgb[i])
			{
				j=*p;
				k=i;
			}
		rgb[k]=MAXVAL;
	}
	*o=rgb[0]>>7|rgb[1]>>6|rgb[2]>>8;
	return (RI*rgb[0]+GI*rgb[1]+BI*rgb[2])>>16;
}

int main(int argc,char **argv)
{  
	png_structp png_ptr;
	png_infop info_ptr, end_info;
	png_bytepp png_rows;
	png_bytep row_ptr;
	png_uint_32 width, height;
	int  bit_depth, color_type, interlace_type, compression_type, filter_method;

	zx_pixel (*zx_ptr)[ZX_X+2], *zx_tmp, *zx_min, *zx_max;
	unsigned char *zx_attr, *zx_atr, *zx_apr, *zx_ahl;
	int newwidth, zx_shift, zy_shift;
	int (*zx_pre)[ZX_X], *zx_hlp;
	int zx_ax, zx_ay;
	int zx_ex, zx_ey;
	int zx_sx, zx_sy;
	int *equ, (*cequ)[MAXVAL];

	int m, n, o, p, a, b, c, d;
	int cr, cg, cb, cy;
	FILE *fp, *fs, *ft;

	int fno;
	char *dum;

	if(argc<ARGNO+1)
	{
		printf("Usage: %s %s\n",*argv,USAGE);
		err("Not enough arguments");
	}
	if(!((fs=fopen(STREAM,"ab"))&&(ft=fopen(TAP,"ab"))))
		err("Cannot create output files");
	if(!((zx_ptr=(zx_pixel (*)[ZX_X+2])malloc((ZX_X+2)*(ZX_Y+2)*sizeof(zx_pixel)))
				&&(zx_pre=(int (*)[ZX_X])malloc((ZX_X)*(ZX_Y)*sizeof(int)))
				&&(zx_attr=(unsigned char *)malloc((ZX_Y*ZX_X)/(CHA_X*CHA_Y)))
				&&(zx_apr=(unsigned char *)malloc((ZX_Y*ZX_X)/(CHA_X*CHA_Y)))
				&&(dum=(char *)(malloc(256)))
				&&(equ=(int *)(malloc(sizeof(int)*MAXVAL)))
				&&(cequ=(int (*)[MAXVAL])(malloc(3*sizeof(int)*MAXVAL)))))
		err("Not enough memory for transformation");
	bzero(zx_pre,(ZX_X)*(ZX_Y)*sizeof(int));
	if(argc>ARGNO+3 && strcmp(argv[5],"-"))
		sh_ran=atoi(argv[5]);
	fno=atoi(argv[1]);
	while(fno<=atoi(argv[2]))
	{
		sprintf(dum,"%08d.png",fno);
		if(!(fp=fopen(dum,"rb")))
			err("Cannot open file");
		if(!(png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,(png_voidp)NULL,NULL,NULL)))
			err("PNG readstruct failed");
		if(!((info_ptr=png_create_info_struct(png_ptr))
					&& (end_info=png_create_info_struct(png_ptr))))
		{
			png_destroy_read_struct(&png_ptr,(png_infopp)NULL,(png_infopp)NULL);
			err("PNG infostruct failed");
		}
		png_init_io(png_ptr,fp);
		png_read_png(png_ptr,info_ptr,PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA,NULL);    
		png_get_IHDR(png_ptr,info_ptr,&width,&height,&bit_depth,&color_type,&interlace_type,&compression_type,&filter_method);
		printf("Image %dx%d, %d-bit %s, ",(int)width,(int)height,bit_depth,(color_type & PNG_COLOR_TYPE_RGB)?"RGB":"nonRGB");
		if(argc>ARGNO+6)
		{
			zy_shift=(height-atoi(argv[8]))>>1;
			height=atoi(argv[8]);
		}
		else
			zy_shift=0;
		printf("used %dx%d, shrink %d, ",newwidth=(ZX_X*height)/ZX_Y,(int)height,sh_ran);
		if((zx_shift=((int)width-newwidth)>>1)<0)
			err("Cannot scale image, use y-realsize parameter");
		png_rows=png_get_rows(png_ptr,info_ptr);
		bzero(zx_ptr,(ZX_X+2)*(ZX_Y+2)*sizeof(zx_pixel));
		zx_ey=0;
		zx_ay=1;
		for(m=0;m<height;m++)
		{
			zx_ey+=ZX_Y;
			zx_sy=ZX_Y;
			if(zx_ey>=height)
			{
				zx_ey-=height;
				zx_sy=zx_ey;
				zx_ay++;
			}
			zx_ex=0;
			zx_ax=1;
			for(n=0;n<newwidth;n++)
			{
				zx_ex+=ZX_X;
				zx_sx=ZX_X;
				row_ptr=png_rows[m+zy_shift]+3*(n+zx_shift);
				if(zx_ex>=newwidth)
				{
					zx_ex-=newwidth;
					zx_sx=zx_ex;
					zx_ax++;
					zx_contrib(zx_ptr[zx_ay-1]+zx_ax-1,row_ptr,(ZX_Y-zx_sy)*(ZX_X-zx_sx));
					zx_contrib(zx_ptr[zx_ay]+zx_ax-1,row_ptr,zx_sy*(ZX_X-zx_sx));
				}
				zx_contrib(zx_ptr[zx_ay-1]+zx_ax,row_ptr,(ZX_Y-zx_sy)*zx_sx);
				zx_contrib(zx_ptr[zx_ay]+zx_ax,row_ptr,zx_sy*zx_sx);  
			}
		}
		fclose(fp);
		png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
		o=height*newwidth;
		bzero(equ,sizeof(int)*MAXVAL);
		bzero(cequ,3*sizeof(int)*MAXVAL);
		for(m=1;m<=ZX_Y;m++)
		{
			zx_tmp=zx_ptr[m]+1;
			for(n=ZX_X;n--;)
			{
				zx_tmp->r=cr=(zx_tmp->r*(ZX_X*ZX_Y))/o;
				cequ[0][cr]+=MAXVAL-1;
				zx_tmp->g=cg=(zx_tmp->g*(ZX_X*ZX_Y))/o;
				cequ[1][cg]+=MAXVAL-1;
				zx_tmp->b=cb=(zx_tmp->b*(ZX_X*ZX_Y))/o;
				cequ[2][cb]+=MAXVAL-1;
				zx_tmp->y=cy=(RI*cr+GI*cg+BI*cb)>>16;
				equ[cy]+=MAXVAL-1;
				zx_tmp++;
			}
		}
		if(argc>ARGNO+4 && !strcmp(argv[6],"col"))
		{
			printf("col, ");
			cr=cg=cb=0;
			for(m=0;m<MAXVAL;m++)
			{
				cr+=cequ[0][m];
				cg+=cequ[1][m];
				cb+=cequ[2][m];
				cequ[0][m]=cr/(ZX_X*ZX_Y);
				cequ[1][m]=cg/(ZX_X*ZX_Y);
				cequ[2][m]=cb/(ZX_X*ZX_Y);
			}
			for(m=1;m<=ZX_Y;m++)
			{
				zx_tmp=zx_ptr[m]+1;
				for(n=ZX_X;n--;zx_tmp++)
				{
					zx_tmp->r=cequ[0][zx_tmp->r];
					zx_tmp->g=cequ[1][zx_tmp->g];
					zx_tmp->b=cequ[2][zx_tmp->b];
				}
			}
		}      
		if(argc>ARGNO+1 && !strcmp(argv[3],"equ"))
		{
			printf("equ, ");
			cy=0;
			for(m=0;m<MAXVAL;m++)
			{
				cy+=equ[m];
				equ[m]=cy/(ZX_X*ZX_Y);
			}
			for(m=1;m<=ZX_Y;m++)
			{
				zx_tmp=zx_ptr[m]+1;
				for(n=ZX_X;n--;zx_tmp++)
					zx_tmp->y=equ[zx_tmp->y];
			}
		}
		zx_atr=zx_attr;    
		for(m=1;m<=ZX_Y;m+=CHA_Y)    
			for(n=1;n<=ZX_X;n+=CHA_X)
			{
				cb=MAXVAL; cg=0;
				zx_max = zx_min = zx_ptr[m+CHA_Y]+n+CHA_X;
				for(o=CHA_Y;o--;)
					for(p=CHA_X;p--;)
					{
						zx_tmp=zx_ptr[m+o]+n+p;
						cr=zx_tmp->y;
						if(cr<=cb)
						{
							zx_min=zx_tmp;
							cb=cr;
						}
						if(cr>=cg)
						{
							zx_max=zx_tmp;
							cg=cr;
						}
					}
				do
				{	    
					c=3;  
					do  
					{
						a=zx_huelum(zx_min,&cr,--c);
					}
					while(a>cb);
					d=0;
					do
					{
						b=zx_huelum(zx_max,&cy,++d);
					}
					while(b<cg);
					if(d-c==2 && cg!=cb && (MAXVAL*(cg-cb))/(b-a)<sh_ran)
					{
						cy=MAXVAL;
						cg=cb=(cg+cb)>>1;
					} 
				}
				while(cy>7);    
				*zx_atr++=BRIGHT|cr<<3|cy;
				if(b!=a)
				{
					for(o=CHA_Y;o--;)
						for(p=CHA_X;p--;)
						{
							zx_tmp=zx_ptr[m+o]+n+p;
							cy=zx_tmp->y;
							if(cy<a)
								cy=a;
							else
								if(cy>b)
									cy=b;
							zx_tmp->y=((MAXVAL-1)*(cy-a))/(b-a);
						} 
				}
			}
		if(argc>ARGNO+5 && !strcmp(argv[7],"avg"))
		{
			printf("avg, ");
			zx_atr=zx_attr;
			zx_ahl=zx_apr;
			for(o=0;o<(ZX_X*ZX_Y)/(CHA_X*CHA_Y);o++)
			{
				p=(0x38&(((*zx_atr&0x38) + (*zx_ahl&0x38))>>1)) | (0x7&(((*zx_atr&0x7) + (*zx_ahl&0x7))>>1)) | BRIGHT;
				*zx_ahl=*zx_atr;
				*zx_atr=p;
				zx_atr++;
				zx_ahl++;
			}
		}
		if(argc>ARGNO+2 && !strcmp(argv[4],"frm"))
		{
			printf("frm, ");
			for(m=1;m<=ZX_Y;m++)
			{
				zx_hlp=zx_pre[m-1];
				zx_tmp=zx_ptr[m]+1;
				for(n=ZX_X;n--;zx_hlp++)
				{
					cy=((zx_tmp->y)<<1)-*zx_hlp;
					if(cy<0)
						cy=0;
					else
						if(cy>=MAXVAL)
							cy=MAXVAL-1;
					zx_tmp->y=cy;
					*zx_hlp=cy;
					zx_tmp++;
				}
			}
		}
		fwrite(zx_attr,256,3,fs);
		fwrite(dum,256,1,fs);
		fwrite(tapprefix,sizeof(tapprefix),1,ft);
		p=BDFLAG;
		for(m=1;m<=ZX_Y;m++)
			for(n=ZX_X;n;n-=2)
			{
				fputc((o=(((zx_ptr[m][n].y)>>4)|((zx_ptr[m][n-1].y)&0xf0))),fs);
				fputc(o,ft);
				p^=o;
			}
		zx_atr=zx_attr;
		for(o=0;o<(ZX_X*ZX_Y)/(CHA_X*CHA_Y);o++)
		{
			fputc(*zx_atr,ft);
			p^=*zx_atr++;
		}
		fputc(p,ft);  
		printf("(%s).\n",dum);
		fno++;
	}
	fclose(fs);
	fclose(ft);                
	return OK;
}
