/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "rrutil.h"
#include "rrtimer.h"
#include "fbx.h"
#ifdef _WIN32
 char *program_name;
#else
 extern "C" {
 #define NeedFunctionPrototypes 1
 #include "dsimple.h"
 }
 #include <sys/signal.h>
 #include <sys/time.h>
 #include <sys/resource.h>
#endif

#define _throw(f, l, m) {fprintf(stderr, "%s (%d):\n%s\n", f, l, m);  fflush(stderr);  exit(1);}
#define fbx(a) {if((a)==-1)_throw("fbx.c", fbx_geterrline(), fbx_geterrmsg());}
#define dieifnot(a) {if(!(a)) {_throw(__LINE__, __FILE__, hputil_strerror());}}
#define hpu(a) {if((a)==-1) {_throw(__LINE__, __FILE__, hputil_strerror());}}

void usage(void)
{
	printf("\nUSAGE: %s [-h|-?] -t<seconds> -s<samples per second>\n", program_name);
	printf("       -wh<window handle in hex>\n");
	printf("       -x<x offset> -y<y offset>\n\n");
	printf("-h or -? = This help screen\n");
	printf("-t = Run the benchmark for this many seconds\n");
	printf("-s = Sample the window this many times per second\n");
	printf("-wh = Explicitly specify a window (if auto-detect fails)\n");
	printf("-x = x coordinate (relative to window) of the sampling block\n");
	printf("-y = y coordinate (relative to window) of the sampling block\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int i;  fbx_wh wh;
	double benchtime=30.0, benchstart=0., benchend;  rrtimer timer;
	int samplerate=50, xcoord=-1, ycoord=-1;

	program_name=argv[0];
	memset(&wh, 0, sizeof(wh));

	if(argc>1) for(i=1; i<argc; i++)
	{
		double tempf;  int temp;
		if(!stricmp(argv[i], "-h") || !stricmp(argv[i], "-?")) usage();
		if(!strnicmp(argv[i], "-t", 2) && strlen(argv[i])>2 &&
		  (tempf=atof(&argv[i][2]))>0.) benchtime=tempf;
		if(!strnicmp(argv[i], "-s", 2) && strlen(argv[i])>2 &&
		  (temp=atoi(&argv[i][2]))>1) samplerate=temp;
		if(!strnicmp(argv[i], "-x", 2) && strlen(argv[i])>2 &&
		  (temp=atoi(&argv[i][2]))>0) xcoord=temp;
		if(!strnicmp(argv[i], "-y", 2) && strlen(argv[i])>2 &&
		  (temp=atoi(&argv[i][2]))>0) ycoord=temp;
		if(!strnicmp(argv[i], "-wh", 3) && strlen(argv[i])>3)
		{
			fbx_wh temp;
			memset(&temp, 0, sizeof(temp));
			#ifdef _WIN32
			if(sscanf(&argv[i][3], "%x", &temp)==1) wh=temp;
			#else
			if(sscanf(&argv[i][3], "%x", &temp.win)==1) wh.win=temp.win;
			#endif
		}
	}

	printf("\nThin Client Benchmark\n");

	#ifdef _WIN32
	if(!wh) {
	printf("Click the mouse in the window that you wish to monitor ... ");
	wh=GetForegroundWindow();
	double tStart, tElapsed;  int t, oldt=-1;
	tStart=timer.time();
	do
	{
		tElapsed=timer.time()-tStart;
		t=(int)(10.-tElapsed);
		if(t!=oldt) {oldt=t;  printf("%.2d\b\b", t);}
		Sleep(50);
	} while(wh==GetForegroundWindow() && tElapsed<10.);
	wh=GetForegroundWindow();
	FlashWindow(wh, TRUE);
	printf("  \n");
	}
	char temps[1024];
	GetWindowText(wh, temps, 1024);
	printf("Monitoring window 0x%.8x (%s)\n", wh, temps);
	#else
	if(!XInitThreads()) {fprintf(stderr, "XInitThreads() failed\n");  exit(1);}
	if(!(wh.dpy=XOpenDisplay(0))) {fprintf(stderr, "Could not open display %s\n", XDisplayName(0));  exit(1);}
	if(!wh.win) {
	printf("Click the mouse in the window that you wish to monitor ...\n");
	wh.win=Select_Window(wh.dpy);
	XSetInputFocus(wh.dpy, wh.win, RevertToNone, CurrentTime);
	}
	#endif

	fbx_struct fb;
	memset(&fb, 0, sizeof(fb));
	fbx(fbx_init(&fb, wh, 0, 0, 1));
	int width=fb.width, height=fb.height;
	fbx_term(&fb);
	memset(&fb, 0, sizeof(fb));
	fbx(fbx_init(&fb, wh, 32, 32, 1));

	int frames=0, samples=0;
	if(xcoord<0) xcoord=width/2;
	if(ycoord<0) ycoord=height/2;
	printf("Sample block location: %d, %d\n", xcoord, ycoord);
	unsigned char buf[32*32*4];
	int first=1;
	benchend=timer.time();
	do
	{
		fbx(fbx_read(&fb, xcoord, ycoord));
		samples++;
		if(first)
		{
			first=0;
			benchstart=timer.time();
		}
		else
		{
			if(memcmp(buf, fb.bits, fb.ps*32*32)) frames++;
		}
		memcpy(buf, fb.bits, fb.ps*32*32);
		int sleeptime=(int)(1000000.*(1./(float)samplerate-(timer.time()-benchend)));
		if(sleeptime>0) usleep(sleeptime);
	} while((benchend=timer.time())-benchstart<benchtime);
	printf("Samples: %d  Time: %f s  Frames/sec: %f\n", samples, benchend-benchstart, (double)frames/(benchend-benchstart));
	return 0;
}
