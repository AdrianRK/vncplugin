/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  VNC local solution
 *
 *        Version:  1.0
 *        Created:  14/05/2020 09:39:10
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <cstdlib>
#include <rfb/rfbclient.h>
#include "logging.hpp"

static rfbBool resize (rfbClient *client) 
{
	return true;
}

static void update (rfbClient *cl, int x, int y, int w, int h) 
{
	printLog("Received update message of size ", x, y, w, h);
}

static void got_cut_text (rfbClient *cl, const char *text, int textlen)
{
	printLog("Received cut text of size ", textlen);
}

const char * password = "hellbell200";

static char* get_password (rfbClient *client)
{
	printLog("Requesting password");	
	char* pss = nullptr;
	pss = (char*)malloc(sizeof(char) * (strlen(password) + 1));
	if (pss)
	{
		memcpy(pss, password, sizeof(char) * (strlen(password) + 1));
	}
	return pss;
}

static void kbd_leds (rfbClient *cl, int value, int pad) 
{
    printLog("Led State = ", value);
}

static void text_chat (rfbClient *cl, int value, char *text) 
{
	printLog("received chat message");
}

static void log_info(const char *format, ...)
{
	if (!rfbEnableClientLogging)
	{
		return;
	}
	
	va_list args;
	time_t clk;
	char buff[512] = {0,};
	char tmbuff[256] ={0,};
	time(&clk);
	
	strftime(tmbuff, 255, "%d/%m/%Y %x", localtime(&clk));
	snprintf(buff, 512, format, args);
	log(logger, tmbuff, " ", buff);

	va_start(args, format);
	
	va_end(args);
}

int main (int argc, char **argv)
{
    if (argc < 2)
    {
        printError("Plugin requires the address of the vnc server and the screen number (localhost:1)");
    }
	printLog("Start application");
	
	rfbClientLog=rfbClientErr=log_info;

	rfbClient *cl;

	cl = rfbGetClient (8, 3, 4);

	cl->MallocFrameBuffer = resize;
	cl->canHandleNewFBSize = TRUE;
	cl->GotFrameBufferUpdate = update;
	cl->GotXCutText = got_cut_text;
	cl->HandleKeyboardLedState = kbd_leds;
	cl->HandleTextChat = text_chat;
	cl->GetPassword = get_password;

	if(!rfbInitClient(cl,&argc,argv))
	{
		printError("Failed to connect");
		cl = NULL; /* rfbInitClient has already freed the client struct */
		rfbClientCleanup(cl);
		return 1;
	}
	//int i = 1;
	while (1)
	{
		
	}	

	return 0;
}

