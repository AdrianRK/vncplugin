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
	instance->init();
	instance->m_weakThis = instance;
	return instance;
}

VNCMediator::VNCMediator(
	const std::shared_ptr<ICommunicationChannel>& comm,
	const std::shared_ptr<IVNCServerWrapper>& vncserver)
	: m_comm(comm),
	  m_vncserver(vncserver)
{
}

void VNCMediator::init()
{
	auto mouseButtonClick = [weakThis = m_weakThis] (int x, int y, int button)
	{
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
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

	auto startConnection = []()
	{};

	auto newSesstionStarted = []()
	{};

	auto sesstionSopped = []()
	{};

	auto mouseMove = [weakThis = m_weakThis](int x, int y)
	{
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			printLog("mouseMove X = ", x, " Y = ", y);
			vncmediator->m_vncserver->sendMouseEvent(x, y, 0);
		}
	};

	auto keyboardPress = [weakThis = m_weakThis](int symbol, int unicodeCharacter, int xkbModifiers, bool state)
	{
		std::shared_ptr<VNCMediator> vncmediator = weakThis.lock();
		if (vncmediator)
		{
			printLog("Key press ", symbol, " unicodeCharacter ", unicodeCharacter, " xkbModifiers ", xkbModifiers, " state ", state);

			vncmediator->m_vncserver->sendKeyEvent(symbol, state);
		}
	};

	auto setImageDefinition = [](const std::string& title, int width, int height, double dpi, int bytesperpixel)
	{
		printLog("Entry");
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
		return UpdateType::FullBufferUpdate;
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
	}
}

void VNCMediator::start()
{
	if (m_comm)
	{
		m_comm->startup();
	}

	if (m_vncserver)
	{
		m_vncserver->start();
	}
}

void VNCMediator::stop()
{
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
