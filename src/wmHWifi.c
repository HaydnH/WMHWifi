/*
 *
 *  	wmHWifi-1.0
 * 
 *
 * 	This program is free software; you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	the Free Software Foundation; either version 2, or (at your option)
 * 	any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 * 	along with this program (see the file COPYING); if not, write to the
 * 	Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
 *      Boston, MA  02111-1307, USA
 *
 *
 *      Changes:
 *
 *	Version 1.00  -	released June 21st, 2017
 *
 */

// Includes
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>
#include <X11/X.h>
#include <X11/xpm.h>
#include <ctype.h>
#include <stdarg.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include "xutils_cairo.h"
#include "wmHWifi_master.xpm"
#include "wmHWifi_mask.xbm"


// Delay between refreshes (in microseconds) 
#define DELAY 500000L
#define WMHWIFI_VERSION "1.00"

// Color struct, h = hex (#000000), rgb components in range 0-1.
struct patCol {
  char  h[7];
  float r,g,b;
};
typedef struct patCol PatCol;

void ParseCMDLine(int argc, char *argv[]);
void ButtonPressEvent(XButtonEvent *);
void print_usage();
void h2dCol(PatCol *col);

int     winH = 64, winW = 64;
int     GotFirstClick1, GotDoubleClick1;
int     GotFirstClick2, GotDoubleClick2;
int     GotFirstClick3, GotDoubleClick3;
int     DblClkDelay;
int     HasExecute = 0;		/* controls perf optimization */
char	ExecuteCommand[1024];
char    wifiCmd[1024] = "nmcli -t --fields \"ACTIVE,SIGNAL,SSID\" dev wifi |awk -F \":\" '/^yes/ {print $2 \" \" $3}'";

char    TimeColor[30] = "#000000", BackgroundColor[30] = "#181818";
int     wqi = 0;
char    essid[40];
char	efont[30] = "Arial Narrow";
int	fs = 14, st = 0, sd = 0, eX = 14, ss = 0, ws2 = 20, ws3 = 40, ws4 = 60, ws5 = 80;

PatCol  efg, wbg, ebg, wfgg, wfgs, wfge, wfgs1, wfge1, wfgs2, wfge2, wfgs3, wfge3, wfgs4, wfge4, wfgs5, wfge5;


// Function to convert hex color to rgb values
void h2dCol(PatCol *col) {
  char hCol[3]; 
  strncpy(hCol, col->h+1, 2);
  hCol[2] = '\0';
  col->r = strtol(hCol, NULL, 16)/255.0;
  strncpy(hCol, col->h+3, 2);
  hCol[2] = '\0';
  col->g = strtol(hCol, NULL, 16)/255.0;
  strncpy(hCol, col->h+5, 2);
  hCol[2] = '\0';
  col->b = strtol(hCol, NULL, 16)/255.0;
}


// Get the Wifi signal strength and essid
void getWifiQ() {
  FILE *fp = NULL;
  char    wq[40], rCmd[1024] = "set -o pipefail; ";

  // Default values
  wqi = 0;
  strcpy(essid, "No Wifi");

  // Run the Wifi command
  strcat(rCmd, wifiCmd);
  fp = popen(rCmd, "r");
  if (fp == NULL) {
    printf("ERROR: Failed to run Wifi Command:\n\n  # %s\n\n", wifiCmd);
    exit(1);
  }

  // Wait for the output of the command
  while (fgets(wq, 40, fp)) {
    usleep(50000L);
  }
  strtok(wq, "\n");

  // If we've got values for Wifi quality, set the values
  if (strcmp(wq, "\n") > 0 && strcspn(wq, " ") < strlen(wq)) {
    wqi = strtol(wq, NULL, 10);
    if (wqi)
      strcpy(essid, strchr(wq, ' ')+1);
  }

  // Close fp and check exit status
  if (WEXITSTATUS(pclose(fp))) {
    printf("ERROR: The following command to get Wifi quality exited with a non zero exit code:\n\n  # %s\n\n", wifiCmd);
    exit(1);
  }
}


// Draw the ESSID area 
void drawESSID(cairo_t *ctx) {
  int	eb = 6, sx = 4;
  cairo_text_extents_t  extents;

  cairo_set_antialias(ctx, CAIRO_ANTIALIAS_NONE);

  // Draw essid background 
  cairo_set_source_rgb(ctx, ebg.r, ebg.g, ebg.b);
  cairo_rectangle(ctx, 5, 5, 54, 15);
  cairo_fill(ctx);

  cairo_set_antialias(ctx, CAIRO_ANTIALIAS_BEST);

  // Calculate the X position of the essid text
  cairo_text_extents(ctx, essid, &extents);
  if (extents.width < 54) {
    eX = winW/2-extents.width/2;
    ss = 0;

  } else {

    ss = 1;
    // "Bounce" scrolling
    if (st == 0) {
      if (eX+extents.width < winW-eb && sd == 0) {
        sd = 1;
      } else if (eX > eb && sd == 1) {
        sd = 0;
      } else if (sd != 1) {
        eX = eX-sx;
      } else {
        eX = eX+sx;
      }
    // "Full" scrolling
    } else if (st == 1) {
      if (eX+extents.width < 1) {
        eX = winW-eb;
      } else {
        eX = eX-sx;
      }
    }
  }

  // Draw essid text
  cairo_set_source_rgb(ctx, efg.r, efg.g, efg.b);
  cairo_move_to(ctx, eX, 13-extents.height/2-extents.y_bearing);
  cairo_show_text(ctx, essid);
}


// Draw Wifi Logo area
void drawLogo(cairo_t *ctx) {
  int           wlX = 33, wlY = 58;
  float         pgX = 0, wlRS = 225*(M_PI/180), wlRE = 315*(M_PI/180);
  cairo_pattern_t *pat;

  // Disable antialiasing for background & borders
  cairo_set_antialias(ctx, CAIRO_ANTIALIAS_NONE);

  // Draw logo background
  cairo_set_source_rgb(ctx, wbg.r, wbg.g, wbg.b);
  cairo_rectangle(ctx, 5, 24, 54, 35);
  cairo_fill(ctx);

  // Re-enable antialiasing
  cairo_set_antialias(ctx, CAIRO_ANTIALIAS_BEST);

  // Debug
  //wqi = 0;

  // Set wifi logo colors
  if (wqi>ws5) {
    wfgs = wfgs5;
    wfge = wfge5;
    pgX = 1;
  } else if (wqi>ws4) {
    wfgs = wfgs4;
    wfge = wfge4;
    pgX = 0.99;
  } else if (wqi>ws3) {
    wfgs = wfgs3;
    wfge = wfge3;
    pgX = 0.76;
  } else if (wqi>ws2) {
    wfgs = wfgs2;
    wfge = wfge2;
    pgX = 0.53;
  } else if (wqi>0) {
    wfgs = wfgs1;
    wfge = wfge1;
    pgX = 0.3;
  } else {
    wfgs = wfgg;
    wfge = wfgg;
    pgX = 0;
   }

  // Create a radial gradient pattern
  pat = cairo_pattern_create_radial(wlX, wlY, 0, wlX, wlY, 33);
  cairo_pattern_add_color_stop_rgb(pat, 0, wfgs.r, wfgs.g, wfgs.b);
  cairo_pattern_add_color_stop_rgb(pat, pgX, wfge.r, wfge.g, wfge.b);
  cairo_pattern_add_color_stop_rgb(pat, pgX, wfgg.r, wfgg.g, wfgg.b);
  cairo_pattern_add_color_stop_rgb(pat, 1, wfgg.r, wfgg.g, wfgg.b);
  cairo_set_source(ctx, pat);

  // Draw the Wifi Logo, bar width:
  cairo_set_line_width(ctx, 5);  

  // Frorm bottom up... 
  cairo_move_to(ctx, wlX, wlY);
  cairo_arc(ctx, wlX, wlY, 9, wlRS, wlRE);  // ... Bar 1
  cairo_line_to(ctx, wlX, wlY);
  cairo_fill(ctx);
  cairo_arc(ctx, wlX, wlY, 14, wlRS, wlRE); // ... Bar 2
  cairo_stroke(ctx);
  cairo_arc(ctx, wlX, wlY, 21, wlRS, wlRE); // ... Bar 3
  cairo_stroke(ctx);
  cairo_arc(ctx, wlX, wlY, 28, wlRS, wlRE); // ... Bar 4
  cairo_stroke(ctx);

  // Clean up
  cairo_pattern_destroy(pat);
}


// Redraw borders which the Cairo drawing may overwrite
void drawBorders(cairo_t *ctx) {
    cairo_set_antialias(ctx, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx, 1);
    cairo_set_source_rgb(ctx, 0.81, 0.81, 0.81);
    cairo_move_to(ctx, 4, 21);
    cairo_line_to(ctx, 60, 21);
    cairo_stroke(ctx);
    cairo_set_source_rgb(ctx, 0.81, 0.81, 0.81);
    cairo_move_to(ctx, 60, 5);
    cairo_line_to(ctx, 60, 21);
    cairo_stroke(ctx);
    cairo_set_source_rgb(ctx, 0, 0, 0);
    cairo_move_to(ctx, 5, 4);
    cairo_line_to(ctx, 5, 20);
    cairo_stroke(ctx);
    cairo_set_source_rgb(ctx, 0, 0, 0);
    cairo_move_to(ctx, 4, 24);
    cairo_line_to(ctx, 60, 24);
    cairo_stroke(ctx);
}


// main  
int main(int argc, char *argv[]) {
  XEvent	event;
  int           n = 1000;
  PatCol	*wbgptr = &wbg, *ebgptr = &ebg, *efgptr = &efg, *wfggptr = &wfgg;
  PatCol	*wfgs1ptr = &wfgs1, *wfgs2ptr = &wfgs2, *wfgs3ptr = &wfgs3;
  PatCol	*wfgs4ptr = &wfgs4, *wfgs5ptr = &wfgs5;
  PatCol        *wfge1ptr = &wfge1, *wfge2ptr = &wfge2, *wfge3ptr = &wfge3;
  PatCol	*wfge4ptr = &wfge4, *wfge5ptr = &wfge5;

  cairo_surface_t	*sfc;
  cairo_t		*ctx;
  cairo_font_options_t	*fopts;

  // Default Colors
  strcpy(wbg.h, "#d2dae4");
  strcpy(ebg.h, "#d2dae4");
  strcpy(efg.h, "#181818");
  strcpy(wfgg.h, "#bbbbbb");
  strcpy(wfgs1.h, "#d43b3b");
  strcpy(wfgs2.h, "#d4991f");
  strcpy(wfgs3.h, "#c2ab00");
  strcpy(wfgs4.h, "#7da136");
  strcpy(wfgs5.h, "#1f8a21");
  strcpy(wfge1.h, "#e89494");
  strcpy(wfge2.h, "#ebc782");
  strcpy(wfge3.h, "#dbc200");
  strcpy(wfge4.h, "#bad687");
  strcpy(wfge5.h, "#70cc45");

  // Parse any command line arguments.
  ParseCMDLine(argc, argv);

  // Get initial Wifi data
  getWifiQ();

  // Set colors rgb elements
  h2dCol(wbgptr);
  h2dCol(ebgptr);
  h2dCol(efgptr);
  h2dCol(wfggptr);
  h2dCol(wfgs1ptr);
  h2dCol(wfgs2ptr);
  h2dCol(wfgs3ptr);
  h2dCol(wfgs4ptr);
  h2dCol(wfgs5ptr);
  h2dCol(wfge1ptr);
  h2dCol(wfge2ptr);
  h2dCol(wfge3ptr);
  h2dCol(wfge4ptr);
  h2dCol(wfge5ptr);

  // Init X window & Icon
  initXwindow(argc, argv);
  openXwindow(argc, argv, wmHWifi_master, wmHWifi_mask_bits, wmHWifi_mask_width, wmHWifi_mask_height);

  // Create a Cairo surface and context in the icon window 
  sfc = cairo_create_x11_surface0(winW, winH);
  ctx = cairo_create(sfc);

  // Set font options
  cairo_select_font_face(ctx, efont, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(ctx, fs);
  fopts = cairo_font_options_create();
  cairo_get_font_options(ctx, fopts);
  cairo_font_options_set_hint_style(fopts, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_antialias(fopts, CAIRO_ANTIALIAS_SUBPIXEL);
  cairo_font_options_set_subpixel_order(fopts, CAIRO_SUBPIXEL_ORDER_RGB);
  cairo_set_font_options(ctx, fopts);

  // Loop until we die
  while(1) {
    // Only recalculate every Nth loop
    if (n > 10) {
      n = 0;

      // Get Wifi quality and essid
      getWifiQ();

      // Draw the logo area
      drawLogo(ctx);

      // Draw ESSID if we're not scrolling, inside n loop for performance
      if (ss == 0)
        drawESSID(ctx);
     
    }

    // Draw the ESSID area if scrolling, outside n loop for speed
    if (ss == 1)
      drawESSID(ctx);

    // Redraw borders covered by above drawing
    drawBorders(ctx);

    // Increment counter
    ++n;

    // Keep track of click events. If Delay too long, set GotFirstClick's to False
    if (DblClkDelay > 1500) {
      DblClkDelay = 0;
      GotFirstClick1 = 0; GotDoubleClick1 = 0;
      GotFirstClick2 = 0; GotDoubleClick2 = 0;
      GotFirstClick3 = 0; GotDoubleClick3 = 0;

    } else {
      ++DblClkDelay;
    }

    // Process any pending X events.
    while(XPending(display)){
      XNextEvent(display, &event);
      switch(event.type){
        case ButtonPress:
          ButtonPressEvent(&event.xbutton);
          break;
        case ButtonRelease:
          break;
     }
   }

   // Redraw and wait for next update 
   RedrawWindow();
   usleep(DELAY);

 }
 //cairo_pattern_destroy(pat);
 cairo_destroy(ctx);
 cairo_close_x11_surface(sfc);
}

// Function to check a valid #000000 color is provided
void valid_color(char argv[10], char ccol[7]) {
  char tcol[6];
  if (strcmp(ccol, "missing") == 0 ||ccol[0] == '-' ) {
    fprintf(stderr, "ERROR: No color found following %s flag, must have quotes or escaped #.\n", argv);
    print_usage();
    exit(-1);
  }

  strcpy(tcol,ccol+1);
  if (strlen(ccol) != 7 || tcol[strspn(tcol, "0123456789abcdefABCDEF")] != 0) {
    fprintf(stderr, "ERROR: Invalid color following %s flag, should be valid hex \"#000000\" format.\n", argv);
    print_usage();
    exit(-1);
  }
}

// Parse command line arguments 
void ParseCMDLine(int argc, char *argv[]) {
  int  i;
  char ccol[1024] = "missing";

  for (i = 1; i < argc; i++) {
    if (argc > i+1 && strlen(argv[i+1])<=7)
      strcpy(ccol, argv[i+1]);

    if (!strcmp(argv[i], "-f")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No font found following -f flag.\n");
        print_usage();
        exit(-1);
      }
      strcpy(efont, argv[++i]);

    } else if (!strcmp(argv[i], "-fs")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No font size found following -fs flag.\n");
        print_usage();
        exit(-1);
      }
      fs = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-sf")) {
      st = 1;

    } else if (!strcmp(argv[i], "-sb")) {
      st = 0;

    } else if (!strcmp(argv[i], "-ls2")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No signal step percent provided after -ls2 flag.\n");
        print_usage();
        exit(-1);
      }
      ws2 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-ls3")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No signal step percent provided after -ls3 flag.\n");
        print_usage();
        exit(-1);
      }
      ws3 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-ls4")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No signal step percent provided after -ls4 flag.\n");
        print_usage();
        exit(-1);
      }
      ws4 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-ls5")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No signal step percent provided after -ls5 flag.\n");
        print_usage();
        exit(-1);
      }
      ws5 = strtol(argv[++i], NULL, 10);
    } else if (!strcmp(argv[i], "-eb")) {
      valid_color(argv[i], ccol);
      strcpy(ebg.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lb")) {
      valid_color(argv[i], ccol);
      strcpy(wbg.h, argv[++i]);

    } else if (!strcmp(argv[i], "-ef")) {
      valid_color(argv[i], ccol);
      strcpy(efg.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfg")) {
      valid_color(argv[i], ccol);
      strcpy(wfgg.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfs1")) {
      valid_color(argv[i], ccol);
      strcpy(wfgs1.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfs2")) {
      valid_color(argv[i], ccol);
      strcpy(wfgs2.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfs3")) {
      valid_color(argv[i], ccol);
      strcpy(wfgs3.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfs4")) {
      valid_color(argv[i], ccol);
      strcpy(wfgs4.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfs5")) {
      valid_color(argv[i], ccol);
      strcpy(wfgs5.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfe1")) {
      valid_color(argv[i], ccol);
      strcpy(wfge1.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfe2")) {
      valid_color(argv[i], ccol);
      strcpy(wfge2.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfe3")) {
      valid_color(argv[i], ccol);
      strcpy(wfge3.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfe4")) {
      valid_color(argv[i], ccol);
      strcpy(wfge4.h, argv[++i]);

    } else if (!strcmp(argv[i], "-lfe5")) {
      valid_color(argv[i], ccol);
      strcpy(wfge5.h, argv[++i]);

    } else if (!strcmp(argv[i], "-e")){
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No command found following -e flag.\n");
        print_usage();
        exit(-1);
      }
      strcpy(ExecuteCommand, argv[++i]);
      HasExecute = 1;

    } else if (!strcmp(argv[i], "-n")) {
      strcpy(wifiCmd,  "nmcli -t --fields \"ACTIVE,SIGNAL,SSID\" dev wifi |awk -F \":\" '/^yes/ {print $2 \" \" $3}'");

    } else if (!strcmp(argv[i], "-w")) {
      strcpy(wifiCmd, "wicd-cli -i |awk '/^Connected to / {gsub(\"%\", \"\"); print $5 \" \" $3}'");

    } else if (!strcmp(argv[i], "-c")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No command found following -c flag.\n");
        print_usage();
        exit(-1);
      }

      if (geteuid() == 0) {
        fprintf(stderr, "ERROR: wmHWifi has been run as root, -c flag not allowed.\n");
        exit(1);

      } else {
        strcpy(wifiCmd, argv[++i]);
      }
    } else if (!strcmp(argv[i], "-h")) {
        print_usage();
        exit(-1);

    } else {
      fprintf(stderr, "ERROR: Invalid command line option (%s), \"wmHWifi -h\" for usage.\n\n", argv[i]);
      exit(1);
    }

  }
}


// Print usage
void print_usage(){
    printf("\nwmHWifi version: %s\n", WMHWIFI_VERSION);
    printf("\nusage: wmHWifi [-f <font>] [-fs <size>] [-sb] [-sf] [-ls2 <percent>] [-ls3 <percent>]\n");
    printf("               [-ls4 <percent>] [-ls5 <percent] [-eb <Color>] [-ef <Color>]\n");
    printf("               [-lb <Color>] [-lfg <Color>] [-lfs1 <Color>] [-lfe1 <Color>]\n");
    printf("               [-lfs2 <Color>] [-lfe2 <Color>] [-lfs3 <Color>] [-lfe3 <Color>]\n");
    printf("               [-lfs4 <Color>] [-lfe4 <Color>] [-lfs5 <Color>] [-lfe5 <Color>]\n");
    printf("               [-e <Command>] [-n] [-w] [-c <Command>] [-h]\n\n");
    printf("   -f <font>       Cairo Font to use for ESSID (Default: Arial)\n");
    printf("   -fs <size>      Font size to use for ESSID (Default: 14)\n");
    printf("   -sb             Use \"bounce\" scrolling if ESSID text is too wide (Default)\n");
    printf("   -sf             Use \"full\" scrolling if the ESSID text is too wide\n");
    printf("   -ls2            Percent at which to change wifi logo color (Default: 20)\n");
    printf("   -ls3            Percent at which to change wifi logo color (Default: 40)\n");
    printf("   -ls4            Percent at which to change wifi logo color (Default: 60)\n");
    printf("   -ls5            Percent at which to change wifi logo color (Default: 80)\n");
    printf("   -eb <Color>     ESSID area background color (Default: #dsdae4)\n");
    printf("   -ef <Color>     ESSID area text color (Default: #181818)\n");
    printf("   -lb <Color>     Wifi logo area background color (Default: #d2dae4)\n");
    printf("   -lfg <Color>    Wifi logo area non-colored bar color (Default: #bbbbbb)\n");
    printf("   -lfs1 <Color>   Wifi logo gradient start color when signal <20%% (Default: #d43b3b)\n");
    printf("   -lfe1 <Color>   Wifi logo gradient end color when signal <20%% (Default: #e89494)\n");
    printf("   -lfs2 <Color>   Wifi logo gradient start color when signal <40%% (Default: #d4991f)\n");
    printf("   -lfe2 <Color>   Wifi logo gradient end color when signal <40%% (Default: #ebc782)\n");
    printf("   -lfs3 <Color>   Wifi logo gradient start color when signal <60%% (Default: #c2ab00)\n");
    printf("   -lfe3 <Color>   Wifi logo gradient end color when signal <60%% (Default: #dbc200)\n");
    printf("   -lfs4 <Color>   Wifi logo gradient start color when signal <80%% (Default: #7da136)\n");
    printf("   -lfe4 <Color>   Wifi logo gradient end color when signal <80%% (Default: #bad687)\n");
    printf("   -lfs5 <Color>   Wifi logo gradient start color when signal >80%% (Default: #1f8a21)\n");
    printf("   -lfe5 <Color>   Wifi logo gradient end color when signal >80%% (Default: #70cc45)\n");
    printf("   -e <Command>    Command to execute via double click of mouse button 1\n");
    printf("   -n              Use Network Manager (nmcli) to detrmine Wifi info (Default)\n");
    printf("   -w              Use wicd (wicd-cli) to detrmine Wifi info\n");
    printf("   -c <Command>    Command to run for Wifi quality (should return \"NN ESSID\" format)\n");
    printf("   -h              Display help screen\n");
    printf("\nExample: wmHWifi -eb #111111 -ef #000011 -e \"gnome-control-center network\"\n\n");

}




/*
 *  This routine handles button presses.
 *
 *   Double click on
 *              Mouse Button 1: Execute the command defined in the -e command-line option.
 *              Mouse Button 2: No action assigned.
 *              Mouse Button 3: No action assigned.
 */
void ButtonPressEvent(XButtonEvent *xev){

    char Command[512];


    if( HasExecute == 0) return; /* no command specified.  Ignore clicks. */
    DblClkDelay = 0;
    if ((xev->button == Button1) && (xev->type == ButtonPress)){
        if (GotFirstClick1) GotDoubleClick1 = 1;
        else GotFirstClick1 = 1;
    } else if ((xev->button == Button2) && (xev->type == ButtonPress)){
        if (GotFirstClick2) GotDoubleClick2 = 1;
        else GotFirstClick2 = 1;
    } else if ((xev->button == Button3) && (xev->type == ButtonPress)){
        if (GotFirstClick3) GotDoubleClick3 = 1;
        else GotFirstClick3 = 1;
    }


    /*
     *  We got a double click on Mouse Button1 (i.e. the left one)
     */
    if (GotDoubleClick1) {
        GotFirstClick1 = 0;
        GotDoubleClick1 = 0;
        sprintf(Command, "%s &", ExecuteCommand);
        system(Command);
    }


    /*
     *  We got a double click on Mouse Button2 (i.e. the left one)
     */
    if (GotDoubleClick2) {
        GotFirstClick2 = 0;
        GotDoubleClick2 = 0;
    }


    /*
     *  We got a double click on Mouse Button3 (i.e. the left one)
     */
    if (GotDoubleClick3) {
        GotFirstClick3 = 0;
        GotDoubleClick3 = 0;
    }



   return;

}

