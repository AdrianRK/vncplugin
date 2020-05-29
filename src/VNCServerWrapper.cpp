/*
 * =====================================================================================
 *
 *       Filename:  VNCServerWrapper.cpp
 *
 *    Description:  VNC Server wrapper
 *
 *        Version:  1.0
 *        Created:  28/05/2020 22:25:27
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "logging.hpp"
#include "VNCServerWrapper.h"

#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace
{

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
#ifndef DISABLE_LOG
	log(logger, tmbuff, " ", buff);
#endif

	va_start(args, format);

	va_end(args);
}

//const ClientData g_vnclugin = ClientData::WrapperPointer;
void vncGetWrapperPointer(){}

std::atomic <bool> sendFullBuffer {false};
std::atomic <bool> sendBuffer {false};

}


namespace vncplugin
{

std::shared_ptr<VNCServerWrapper> VNCServerWrapper::create(char** arguments, int count)
{
	std::shared_ptr<VNCServerWrapper> instance (new VNCServerWrapper);
	instance->init(arguments, count);
	instance->m_weakThis = instance;
	return instance;
}

void VNCServerWrapper::sendKeyEvent(int symbol, bool down)
{
	if (m_isRunning)
	{
		SendKeyEvent(m_cl, symbol, down);
	}
}

void VNCServerWrapper::sendMouseEvent(int x, int y, int state)
{
	if (m_isRunning)
	{
		switch (state)
		{
		case 1:
			SendPointerEvent(m_cl, x, y, rfbButton1Mask);
			break;
		case 2:
			SendPointerEvent(m_cl, x, y, rfbButton2Mask);
			break;
		case 3:
			SendPointerEvent(m_cl, x, y, rfbButton3Mask);
			break;
		default:
			break;
		}
	}
}

VNCServerWrapper::VNCServerWrapper()
{}

void VNCServerWrapper::init(char** arguments, int count)
{
	m_arguments = arguments;
	m_count = count;

	if (m_cl == nullptr)
	{
		auto allocateClient = [] (rfbClient *client) -> rfbBool
		{
			printLog("Received resize message");
			if (client->frameBuffer == nullptr)
			{
				client->updateRect.x = client->updateRect.y = 0;
				client->updateRect.w = client->width;
				client->updateRect.h = client->height;

				client->frameBuffer = new uint8_t [client->width * client->height * (client->format.bitsPerPixel/8)];

				VNCServerWrapper * ptr = reinterpret_cast<VNCServerWrapper*>(rfbClientGetClientData(client, reinterpret_cast<void*>(vncGetWrapperPointer)));

				if (ptr && ptr->m_setImageDefinition)
				{
					ptr->m_setImageDefinition(client->desktopName, client->width, client->height, 96, client->format.bitsPerPixel/8);
				}
			}
			return true;
		};

		auto forwardBuffer = [](rfbClient *cl, int x, int y, int w, int h)
		{

			if (!sendBuffer)
			{
				return;
			}

			if (!sendFullBuffer)
			{
				unsigned char * buffer = new  unsigned char[w * h * (cl->format.bitsPerPixel/8)];
				memset(buffer, 0, w * h * (cl->format.bitsPerPixel/8));
				size_t offset = 0;

				for (int i = y * (cl->width * (cl->format.bitsPerPixel/8)); i < (y + h) * (cl->width * (cl->format.bitsPerPixel/8)); i += (cl->width * (cl->format.bitsPerPixel/8)))
				{
					memcpy(buffer + offset, cl->frameBuffer + i + x * (cl->format.bitsPerPixel/8), w * (cl->format.bitsPerPixel/8));
					offset += (w * (cl->format.bitsPerPixel/8));
				}

				VNCServerWrapper * ptr = reinterpret_cast<VNCServerWrapper*>(rfbClientGetClientData(cl, reinterpret_cast<void*>(vncGetWrapperPointer)));

				if (ptr && ptr->m_updateBuffer)
				{
					ptr->m_updateBuffer(buffer, offset, x, y, w, h);
				}
			}
			else
			{
				VNCServerWrapper * ptr = reinterpret_cast<VNCServerWrapper*>(rfbClientGetClientData(cl, reinterpret_cast<void*>(vncGetWrapperPointer)));

				if (ptr && ptr->m_updateBuffer)
				{
					ptr->m_updateBuffer(cl->frameBuffer, cl->width * cl->height * (cl->format.bitsPerPixel/8), 0, 0, cl->width, cl->height);
				}

				sendFullBuffer = false;
			}
		};

		auto cache = [](rfbClient *cl, const char *text, int textlen)
		{
			std::string str(text, textlen);
			printLog("Cut text ", str);
		};


		auto  getUserAndPassword = [](rfbClient* cl, int credentialType) -> rfbCredential*
		{
			printLog("Asking for credentials");

			rfbCredential *Credentials = (rfbCredential*)malloc(sizeof(rfbCredential));
			Credentials->userCredential.username = (char*)malloc(RFB_BUF_SIZE);
			Credentials->userCredential.password = (char*)malloc(RFB_BUF_SIZE);

			if(credentialType != rfbCredentialTypeUser)
			{
				return NULL;
			}

			VNCServerWrapper * ptr = reinterpret_cast<VNCServerWrapper*>(rfbClientGetClientData(cl, reinterpret_cast<void*>(vncGetWrapperPointer)));

			if (ptr)
			{
				if (ptr->m_user.size() == 0 || ptr->m_password.size() == 0)
				{
					rfbClientLog("Username and password required for authentication!\n");
					printf("user: ");
					fgets(Credentials->userCredential.username, RFB_BUF_SIZE, stdin);

					char *buff = getpass("password:");

					memcpy(Credentials->userCredential.password, buff, (strlen(buff) < RFB_BUF_SIZE) ? strlen(buff) + 1 : RFB_BUF_SIZE);

					ptr->m_user = std::string(Credentials->userCredential.username, strlen(Credentials->userCredential.username));
					ptr->m_password = std::string(buff, strlen(buff));
				}
				else
				{
					Credentials->userCredential.username = strdup(ptr->m_user.c_str());
					Credentials->userCredential.password = strdup(ptr->m_password.c_str());
				}

				/* remove trailing newlines */
				Credentials->userCredential.username[strcspn(Credentials->userCredential.username, "\n")] = 0;
				Credentials->userCredential.password[strcspn(Credentials->userCredential.password, "\n")] = 0;
			}
			return Credentials;
		};

		auto  getPassword = [](rfbClient* cl) -> char*
		{
			printLog("Asking for password");

			VNCServerWrapper * ptr = reinterpret_cast<VNCServerWrapper*>(rfbClientGetClientData(cl, reinterpret_cast<void*>(vncGetWrapperPointer)));

			if (ptr)
			{
				if (ptr->m_password.size() == 0)
				{
					char *password = (char*)malloc(RFB_BUF_SIZE);

					rfbClientLog("Password required for authentication!\n");

					char *buff = getpass("password:");

					memcpy(password, buff, (strlen(buff) < RFB_BUF_SIZE) ? strlen(buff) + 1 : RFB_BUF_SIZE);
					ptr->m_password = std::string(buff, strlen(buff));

					return password;
				}
				else
				{
					return strdup(ptr->m_password.c_str());
				}
			}
			else
			{
				return nullptr;
			}
		};

		rfbClientLog=rfbClientErr=log_info;

		m_cl = rfbGetClient (8, 3, 4);

		m_cl->MallocFrameBuffer = allocateClient;
		m_cl->GotFrameBufferUpdate = forwardBuffer;
		m_cl->GotXCutText = cache;
		m_cl->GetCredential = getUserAndPassword;
		m_cl->GetPassword = getPassword;
		m_cl->frameBuffer = nullptr;

		rfbClientSetClientData(m_cl, reinterpret_cast<void*>(vncGetWrapperPointer), this);
	}
}

VNCServerWrapper::~VNCServerWrapper()
{
	stop();
}

void VNCServerWrapper::cleanup()
{
	if (m_cl != nullptr)
	{

		if (m_cl->frameBuffer != nullptr)
		{
			delete[] m_cl->frameBuffer;
			m_cl->frameBuffer = nullptr;
		}
		rfbClientCleanup(m_cl);
		m_cl = nullptr;
	}
}

void VNCServerWrapper::start()
{
	if (!m_isRunning)
	{
		if (m_cl == nullptr)
		{
			init(m_arguments, m_count);
		}
		std::weak_ptr<VNCServerWrapper> weakThis = m_weakThis;

		m_thread = std::thread([weakThis]()
				{
			sleep(1);
			if (std::shared_ptr<VNCServerWrapper> instance = weakThis.lock())
			{

				while (instance->m_isRunning)
				{
					sleep(1);
					if (!rfbInitClient(instance->m_cl, &instance->m_count, instance->m_arguments))
					{
						printError("Failed to connect");
						break;
					}
					int i = 0;
					while (instance->m_isRunning)
					{
						i = WaitForMessage(instance->m_cl,500);
						if(i<0)
						{
							break;
						}
						if(i)
						{
							if(!HandleRFBServerMessage(instance->m_cl))
							{
								break;
							}
						}
					}
				}
				instance->cleanup();
			}
				});

		m_isRunning = true;
	}
}

void VNCServerWrapper::stop()
{
	if (m_isRunning)
	{
		m_isRunning = false;
		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}
	cleanup();
}

}
