/*
 * =====================================================================================
 *
 *       Filename:  vncMediator.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  29/05/2020 17:02:18
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "logging.hpp"
#include "VNCMediator.h"
#include "ICommunicationChannel.h"
#include "IVNCServerWrapper.h"

namespace vncplugin
{

std::shared_ptr<VNCMediator> VNCMediator::create(
	const std::shared_ptr<ICommunicationChannel>& comm,
	const std::shared_ptr<IVNCServerWrapper>& vncserver)
{
	std::shared_ptr<VNCMediator> instance {new VNCMediator(comm, vncserver)};
	instance->m_weakThis = instance;
	instance->init();
	return instance;
}

VNCMediator::VNCMediator(
	const std::shared_ptr<ICommunicationChannel>& comm,
	const std::shared_ptr<IVNCServerWrapper>& vncserver)
	: m_comm(comm),
	  m_vncserver(vncserver)
{
}

VNCMediator::~VNCMediator()
{
	stop();
}

void VNCMediator::init()
{
	auto mouseButtonClick = [weakThis = m_weakThis] (int x, int y, int button)
	{
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			printLog("Entry");
			switch (button)
			{
			case 1:
				vncmediator->m_vncserver->sendMouseEvent(x, y, 1);
				break;
			case 2:
				vncmediator->m_vncserver->sendMouseEvent(x, y, 2);
				break;
			case 3:
				vncmediator->m_vncserver->sendMouseEvent(x, y, 3);
				break;
			default:
				return;
			}
		}
	};

	auto startConnection = [weakThis = m_weakThis]()
	{
		printLog("Entry");
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			vncmediator->m_comm->sendControlMode(ControlMode::FullControl);
		}
	};

	auto newSesstionStarted = [weakThis = m_weakThis]()
	{
		printLog("Entry");
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			vncmediator->m_comm->sendControlMode(ControlMode::FullControl);
			vncmediator->m_vncserver->start();
		}
	};

	auto sesstionSopped = [weakThis = m_weakThis]()
	{
		printLog("Entry");
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			vncmediator->m_vncserver->stop();
		}
	};

	auto mouseMove = [weakThis = m_weakThis](int x, int y)
	{
		printLog("Entry");
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			printLog("mouseMove X = ", x, " Y = ", y);
			vncmediator->m_vncserver->sendMouseEvent(x, y, 0);
		}
	};

	auto keyboardPress = [weakThis = m_weakThis](int symbol, int unicodeCharacter, int xkbModifiers, bool state)
	{
		printLog("Key press ", symbol, " unicodeCharacter ", unicodeCharacter, " xkbModifiers ", xkbModifiers, " state ", state);
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator && vncmediator->m_vncserver && vncmediator->m_vncserver->isRunning())
		{
			vncmediator->m_vncserver->sendKeyEvent(symbol, state);
		}
	};

	auto setImageDefinition = [weakThis = m_weakThis](const std::string& title, int width, int height, double dpi, int bytesperpixel)
	{
		printLog("Entry");
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			if (bytesperpixel == 4)
			{
				vncmediator->m_comm->sendImageDefinitionForGrabResult(title, width, height, dpi, ColorFormat::RGBA32);
			}
			else
			{
				printError("Unknown color format ", bytesperpixel);
			}

		}
	};

	auto updateBuffer = [weakThis = m_weakThis](const unsigned char* buffer, int screensize, int x, int y, int w, int h)
	{
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator && buffer)
		{
			std::string str(reinterpret_cast<const char*>(buffer), screensize);
			vncmediator->m_comm->sendScreenGrabResult(x, y, w, h, str);
		}
	};

	auto GetUpdateTypeCB = []() -> UpdateType
	{
		return UpdateType::PartialBufferUpdate;
	};

	auto vncServerDisconnect = [weakThis = m_weakThis](bool normalTermination)
	{
		if (!normalTermination)
		{
			std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
			if (vncmediator)
			{
				vncmediator->m_comm->sendStop();
			}
		}
	};


	if (m_comm)
	{
		m_comm->setStartServerConnection(startConnection);
		m_comm->setStartRemoteConnection(newSesstionStarted);
		m_comm->setMouseClickCB(mouseButtonClick);
		m_comm->setMouseMovementConnection(mouseMove);
		m_comm->setStopRemoteConnectionCB(sesstionSopped);
		m_comm->setKeyboardKeyEventCB(keyboardPress);
	}

	if (m_vncserver)
	{
		m_vncserver->setSetImageDefinition(setImageDefinition);
		m_vncserver->setUpdateBuffer(updateBuffer);
		m_vncserver->setGetUpdateType(GetUpdateTypeCB);
		m_vncserver->setVNCServerDisconnect(vncServerDisconnect);
	}
}

void VNCMediator::start()
{
	printLog("Entry");
	if (m_comm)
	{
		m_comm->startup();
	}
}

void VNCMediator::stop()
{
	printLog("Entry");
	if (m_comm)
	{
		m_comm->sendStop();
		m_comm->shutdown();
	}

	if (m_vncserver)
	{
		m_vncserver->stop();
	}
}

} // namespace vncplugin
