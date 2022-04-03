#undef fopen
#undef fclose
#undef fwrite
#undef fseek
#undef ftell

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmc.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <malloc.h>
#include <sjis.h>
#include <romfs_io.h>

#include "ps2_specific.h"
#include "globals.h"

// taken from ps2sdk mc_example
void CreateSave(std::string tuxdir, std::string name)
{
	FILE* mc_fd;
	FILE* icon_fd;
	size_t icon_size;
	char* icon_buffer;
	char str[256];
	char dirpath[128];
	mcIcon icon_sys;

	sprintf(dirpath, "mc0:SUPERTUX%s", tuxdir.c_str());
	mkdir(dirpath, 0755);

	static iconIVECTOR bgcolor[4] = {
		{  81, 107, 194,  0 }, // top left
		{  81, 107, 194,  0 }, // top right
		{ 157, 250, 255,  0 }, // bottom left
		{ 157, 250, 255,  0 }, // bottom right
	};

	static iconFVECTOR lightdir[3] = {
		{ 0.5, 0.5, 0.5, 0.0 },
		{ 0.0,-0.4,-0.1, 0.0 },
		{-0.5,-0.5, 0.5, 0.0 },
	};

	static iconFVECTOR lightcol[3] = {
		{ 0.3, 0.3, 0.3, 0.00 },
		{ 0.4, 0.4, 0.4, 0.00 },
		{ 0.5, 0.5, 0.5, 0.00 },
	};

	static iconFVECTOR ambient = { 1.00, 1.00, 1.00, 0.00 };

	// Set up icon.sys. This is the file which controls how our memory card save looks
	// in the PS2 browser screen. It contains info on the bg colour, lighting, save name
	// and icon filenames. Please note that the save name is sjis encoded.

	memset(&icon_sys, 0, sizeof(mcIcon));
	strcpy((char*)icon_sys.head, "PS2D");
	//strcpy_sjis((short *)&icon_sys.title, "\n");
	sprintf(str, "SuperTux\n%s", name.c_str());
	strcpy_sjis((short *)&icon_sys.title, str);
	icon_sys.nlOffset = 16;
	icon_sys.trans = 0x60;
	memcpy(icon_sys.bgCol, bgcolor, sizeof(bgcolor));
	memcpy(icon_sys.lightDir, lightdir, sizeof(lightdir));
	memcpy(icon_sys.lightCol, lightcol, sizeof(lightcol));
	memcpy(icon_sys.lightAmbient, ambient, sizeof(ambient));
	strcpy((char*)icon_sys.view, "tux.icn"); // these filenames are relative to the directory
	strcpy((char*)icon_sys.copy, "tux.icn"); // in which icon.sys resides.
	strcpy((char*)icon_sys.del, "tux.icn");

	// Write icon.sys to the memory card (Note that this filename is fixed)
	sprintf(str, "%s/icon.sys", dirpath);
	mc_fd = fopen(str, "w");
	if(!mc_fd) return;

	fwrite(&icon_sys, 1, sizeof(icon_sys), mc_fd);
	fclose(mc_fd);
	printf("icon.sys written sucessfully.\n");

	// Write icon file to the memory card.
	// Note: The icon file was created with my bmp2icon tool, available for download at
	//       http://www.ps2dev.org
	icon_fd = ropen("data/tux.icn", "r");
	if(!icon_fd) return;

	rseek(icon_fd,0,SEEK_END);
	icon_size = rtell(icon_fd);
	rseek(icon_fd,0,SEEK_SET);

	icon_buffer = (char*)malloc(icon_size);
	if(icon_buffer == NULL) return;
	if(rread(icon_buffer, 1, icon_size, icon_fd) != icon_size) return;
	rclose(icon_fd);

	sprintf(str, "%s/tux.icn", dirpath);
	icon_fd = fopen(str, "w");
	if(!icon_fd) return;

	fwrite(icon_buffer, 1, icon_size, icon_fd);
	fclose(icon_fd);
	printf("tux.icn written sucessfully.\n");
}
