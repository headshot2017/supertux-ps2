//  $Id: supertux.cpp 997 2004-05-05 16:07:20Z rmcruz $
// 
//  SuperTux
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include <sys/types.h>
#include <ctype.h>
#include <cstring>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <tamtypes.h>
#include <sifrpc.h>
#include <kernel.h>
#include <loadfile.h>
#include <errno.h>
#include <iopheap.h>
#include <debug.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <libmc.h>

#include "defines.h"
#include "globals.h"
#include "setup.h"
#include "intro.h"
#include "title.h"
#include "gameloop.h"
#include "leveleditor.h"
#include "screen.h"
#include "sound.h"
#include "worldmap.h"
#include "resources.h"
#include "texture.h"
#include "tile.h"

int test_read()
{
        const char *filename = "mass:TEST.TXT";
        char text[81];
        int fd;
        int size;

        printf("read test:\n");

        fd = open(filename, O_RDONLY);
        if (fd <= 0) {
                printf("  Cannot open file '%s'\n", filename);
		return -1;
        }

        while ((size = read(fd, text, 10)) >= 1) {
                text[size] = 0;
                printf("read%d: %s\n", size, text);
        }

        close(fd);
        return 0;
}


#define IRX_DEFINE(mod) \
extern unsigned char mod##_irx[]; \
extern unsigned int size_##mod##_irx

#define IRX_LOAD(mod) \
    if (SifExecModuleBuffer(mod##_irx, size_##mod##_irx, 0, NULL, NULL) < 0) \
        printf("Could not load '%s'\n", #mod); \
    else printf("Loaded %s\n", #mod);

IRX_DEFINE(usbd);
IRX_DEFINE(usbhdfsd);
//IRX_DEFINE(usbmass_bd_mini);


#define	MASS_USB_ID	0x500C0F1
static SifRpcClientData_t client __attribute__((aligned(64)));
int usb_mass_bindRpc() {
	int ret;
	int retryCount = 0x1000;

	while(retryCount--) {
	        ret = SifBindRpc( &client, MASS_USB_ID, 0);
        	if ( ret  < 0)  {
	           return -1;
	        }
	        if (client.server != 0) break;

	        // short delay 
	      	ret = 0x10000;
	    	while(ret--) asm("nop\nnop\nnop\nnop");
	}
	return retryCount;
}

int main(int argc, char * argv[])
{
	if (SDL_Init(SDL_INIT_TIMER))
	{
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return -1;
	}
    SDL_Delay(1000);

// change priority to make SDL audio thread run properly
    int main_id = GetThreadId();
    ChangeThreadPriority(main_id, 72);

    // Initialize and connect to all SIF services on the IOP.
    SifInitRpc(0);
    while (!SifIopReset("", 0)); // clean previous loading of irx by apps like ulaunchElf. Comment this line to get cout on ps2link
    while (!SifIopSync());
    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();

    // Apply the SBV LMB patch to allow modules to be loaded from a buffer in EE RAM.
    sbv_patch_enable_lmb();

  st_video_setup();
  st_joystick_setup();

  //sleep(3);

  int ret = 0;

    ret = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
    if (ret < 0) {
        //exit(-1);
    }

    ret = SifLoadModule("rom0:XPADMAN", 0, NULL);
    if (ret < 0) {
        //exit(-1);
    }

    ret = SifLoadModule("rom0:XMCMAN", 0, NULL);
    if (ret < 0) {
        //exit(-1);
    }
    ret = SifLoadModule("rom0:XMCSERV", 0, NULL);
    if (ret < 0) {
        printf("Failed to load module: XMCSERV");
    }

    if(mcInit(MC_TYPE_XMC) < 0)
        printf("Failed to initialise memcard server!\n");

    //FILE *fd = fopen("mc0:/tux.txt", "w");
    //if (fd) fprintf(fd, "loading usb\n");
  IRX_LOAD(usbd);
  IRX_LOAD(usbhdfsd);
  //IRX_LOAD(usbmass_bd_mini);
    //if (fd) fprintf(fd, "loaded\n");

ret = usb_mass_bindRpc();
	if (ret < 0 ) {
	        printf("\nSifBindRpc failed: %d !!!!\n", ret);
	}else {
		printf("ok\n");
	}
  //sleep(2);

/*
	fd = fopen("mc0:/SUPERTUX/icon.sys", "r");
	if(fd ==NULL)
	{
		printf("\nNo previous save exists, creating...\n");
		//if((ret = CreateSave()) < 0)
			//printf("Failed to create save! Errorno: %d\n",ret);
	}
	else
		fclose(fd);
*/

  st_directory_setup();
  parseargs(argc, argv);

  st_general_setup();
  fadeout("Initalizing audio");
  st_audio_setup();
  fadeout("Menus");
  st_menu();
  fadeout("Shared assets");
  loadshared();

  SDL_ShowCursor(false);

    //if (fd) {fprintf(fd, "entered title\n"); fclose(fd);};
  title();

  // exit game
  clearscreen(0, 0, 0);
  updatescreen();
  SDL_Quit();

  unloadshared();
  st_general_free();
  TileManager::destroy_instance();
#ifdef DEBUG
  Surface::debug_check();
#endif
  st_shutdown();

  return 0;
}
