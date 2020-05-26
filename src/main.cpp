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
#include <csignal>
#include "logging.hpp"
#include "CommunicationChannel.h"

#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

constexpr const char* RegistrationServiceLocation =
	"unix:///tmp/teamviewer-iot-agent-services/remoteScreen/registrationService";

const std::shared_ptr<CommunicationChannel> comm = CommunicationChannel::Create(RegistrationServiceLocation);

bool framebuffer_allocated = false;
uint8_t * surface = nullptr;
size_t screensize = 0;
size_t bytesperpixel = 0;
size_t width  = 0;
size_t height = 0;
size_t depth  = 0;
rfbClient *cl = NULL;

static void cleanup(rfbClient* cl);

void log_info(const char *format, ...)
{
	if (!rfbEnableClientLogging)
	{
		return;
	}

	va_list args;
	time_t clk;
	char buff[512] = {0,};
	char tmbuff[256] = {0,};
	time(&clk);

	strftime(tmbuff, 255, "%d/%m/%Y %x", localtime(&clk));
	snprintf(buff, 512, format, args);
	log(logger, tmbuff, " ", buff);

	va_start(args, format);

	va_end(args);
}

uint32_t get(rfbClient *cl, int x, int y)
{
	if (cl)
	{
        switch (cl->format.bitsPerPixel)
        {
        case 1: return ((uint8_t *)cl->frameBuffer)[x + y * cl->width];
        case 2: return ((uint16_t *)cl->frameBuffer)[x + y * cl->width];
        case 4: return ((uint32_t *)cl->frameBuffer)[x + y * cl->width];
        default:
                printError("Unknown bytes/pixel: %d", cl->format.bitsPerPixel);
                cleanup(cl);
                exit(1);
        }
	}
	return 0;
}

static rfbBool resize (rfbClient *client) 
{
	printLog("Received resize message");
	if (framebuffer_allocated == false)
	{

		width  = client->width;
		height = client->height;
		depth  = client->format.bitsPerPixel;

		bytesperpixel = client->format.bitsPerPixel / 8;

		client->updateRect.x = client->updateRect.y = 0;
		client->updateRect.w = width;
		client->updateRect.h = height;

		screensize = width * height * (depth/8);

		surface = new uint8_t [width * height * (depth/8)];

		client->frameBuffer = surface;
		framebuffer_allocated = true;

		comm->setImageDefinition("Title", width, height, 96, ColorFormat::RGBA32);
		comm->sendImageDefinitionForGrabResult();
		//comm->setBufferSize(width * height * bytesperpixel);
		//comm->updateBufferSize(surface, width * height * bytesperpixel, 0);
	}
	return true;
}

static void update (rfbClient *cl, int x, int y, int w, int h) 
{
	//printLog("Received update message of size x=", x, " y=",  y, " w=", w, " h=", h);

	/*unsigned char * buffer = new  unsigned char[w * h * bytesperpixel];
	size_t offset = 0;

	for (int j = y; j < y + h; j++)
	{
		memcpy(buffer + offset, surface + (x + y * width) * bytesperpixel, w * bytesperpixel);
		offset += w * bytesperpixel;
	}

	std::string str (reinterpret_cast<char*>(buffer), offset);

	comm->sendScreenGrabResult(x, y, w, h, str);*/

	std::string str (reinterpret_cast<char*>(surface), screensize);

	comm->sendScreenGrabResult(0, 0, width, height, str);
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

static void cleanup(rfbClient* cl)
{
	if (cl != NULL)
	{
		rfbClientCleanup(cl);
	}
	if (surface != nullptr)
	{
		delete[] surface;
		surface = nullptr;
	}
}

void handleSig(int sig)
{
	printLog ("Received signal ", sig);
	cleanup(cl);
	exit(1);
}

void mouseMove(int x, int y)
{
	printLog("mouseMove X = ", x, " Y = ", y);
	SendPointerEvent(cl, x, y, 0);
}

int main (int argc, char **argv)
{
    if (argc < 2)
    {
        printError("Plugin requires the address of the vnc server and the screen number (localhost:1)");
        return 1;
    }
	printLog("Start application");

	signal(SIGTERM, handleSig);
	signal(SIGINT, handleSig);

	rfbClientLog=rfbClientErr=log_info;

	cl = rfbGetClient (8, 3, 4);

	cl->MallocFrameBuffer = resize;
	cl->canHandleNewFBSize = TRUE;
	cl->GotFrameBufferUpdate = update;
	cl->GotXCutText = got_cut_text;
	cl->HandleKeyboardLedState = kbd_leds;
	cl->HandleTextChat = text_chat;
	cl->GetPassword = get_password;

	std::atomic<bool> start {false};

	auto startConnection = [&]()
	{
		start = true;
	};

	comm->setStartServerConnection(startConnection);

	comm->startup();

	while(!start);

	comm->sendControlMode(ControlMode::FullControl);

	if (!rfbInitClient(cl,&argc,argv))
	{
		printError("Failed to connect");
		cleanup(cl);
		cl = NULL; /* rfbInitClient has already freed the client struct */
		return 1;
	}
	int i = 0;
	while (1)
	{
		i = WaitForMessage(cl,500);
		if(i<0)
		{
			cleanup(cl);
			cl = NULL; /* rfbInitClient has already freed the client struct */
			break;
		}
		if(i)
		{
			if(!HandleRFBServerMessage(cl))
			{
				cleanup(cl);
				cl = NULL; /* rfbInitClient has already freed the client struct */
				break;
			}
		}
	}

	return 0;
}

