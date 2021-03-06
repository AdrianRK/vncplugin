//********************************************************************************//
// MIT License                                                                    //
//                                                                                //
// Copyright (c) 2019 TeamViewer Germany GmbH                                     //
//                                                                                //
// Permission is hereby granted, free of charge, to any person obtaining a copy   //
// of this software and associated documentation files (the "Software"), to deal  //
// in the Software without restriction, including without limitation the rights   //
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      //
// copies of the Software, and to permit persons to whom the Software is          //
// furnished to do so, subject to the following conditions:                       //
//                                                                                //
// The above copyright notice and this permission notice shall be included in all //
// copies or substantial portions of the Software.                                //
//                                                                                //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    //
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  //
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  //
// SOFTWARE.                                                                      //
//********************************************************************************//
#include "CommunicationChannel.h"
#include "logging.hpp"

#include <TVRemoteScreenSDKCommunication/ConnectivityService/IConnectivityServiceServer.h>
#include <TVRemoteScreenSDKCommunication/ConnectivityService/IConnectivityServiceClient.h>
#include <TVRemoteScreenSDKCommunication/ConnectivityService/ServiceFactory.h>

#include <TVRemoteScreenSDKCommunication/ImageService/IImageServiceClient.h>
#include <TVRemoteScreenSDKCommunication/ImageService/ServiceFactory.h>

#include <TVRemoteScreenSDKCommunication/ImageNotificationService/IImageNotificationServiceClient.h>
#include <TVRemoteScreenSDKCommunication/ImageNotificationService/ServiceFactory.h>

#include <TVRemoteScreenSDKCommunication/InputService/IInputServiceServer.h>
#include <TVRemoteScreenSDKCommunication/InputService/ServiceFactory.h>

#include <TVRemoteScreenSDKCommunication/RegistrationService/IRegistrationServiceClient.h>
#include <TVRemoteScreenSDKCommunication/RegistrationService/ServiceFactory.h>

#include <TVRemoteScreenSDKCommunication/SessionControlService/ISessionControlServiceClient.h>
#include <TVRemoteScreenSDKCommunication/SessionControlService/ServiceFactory.h>

#include <TVRemoteScreenSDKCommunication/SessionStatusService/ISessionStatusServiceServer.h>
#include <TVRemoteScreenSDKCommunication/SessionStatusService/ServiceFactory.h>

#include <boost/filesystem.hpp>

namespace
{
constexpr const char* ClientVersion = "1.0";
constexpr const char* SocketPrefix = "unix://";

const boost::filesystem::path ConnectivityLocation = "/tmp/TVQtRC/connectivity/";
const boost::filesystem::path InputLocation = "/tmp/TVQtRC/input/";
const boost::filesystem::path SessionStatusLocation = "/tmp/TVQtRC/sessionStatus/";


TVRemoteScreenSDKCommunication::ImageService::ColorFormat getSdkCommunicationColorFormat(const vncplugin::ColorFormat colorFormat)
{
	printLog("Entry");
	switch (colorFormat)
	{
	case vncplugin::ColorFormat::BGRA32: return TVRemoteScreenSDKCommunication::ImageService::ColorFormat::BGRA32;
	case vncplugin::ColorFormat::R5G6B5: return TVRemoteScreenSDKCommunication::ImageService::ColorFormat::R5G6B5;
	case vncplugin::ColorFormat::RGBA32: return TVRemoteScreenSDKCommunication::ImageService::ColorFormat::RGBA32;
	case vncplugin::ColorFormat::Unsupported: return TVRemoteScreenSDKCommunication::ImageService::ColorFormat::Unknown;
	}

	return TVRemoteScreenSDKCommunication::ImageService::ColorFormat::Unknown;
}

TVRemoteScreenSDKCommunication::SessionControlService::ControlMode getSdkCommunicationControlMode(vncplugin::ControlMode mode)
{
	printLog("Entry");
	switch (mode)
	{
		case vncplugin::ControlMode::Disabled:    return TVRemoteScreenSDKCommunication::SessionControlService::ControlMode::Disable;
		case vncplugin::ControlMode::ViewOnly:    return TVRemoteScreenSDKCommunication::SessionControlService::ControlMode::ScreenSharing;
		case vncplugin::ControlMode::FullControl: return TVRemoteScreenSDKCommunication::SessionControlService::ControlMode::FullControl;
	}

	return TVRemoteScreenSDKCommunication::SessionControlService::ControlMode::Unknown;
}

template<typename FactoryFuncT, class ClientT>
bool setupClient(const std::string& serviceLocation, FactoryFuncT factory, typename std::unique_ptr<ClientT>& client, std::mutex& mutex)
{
	printLog("Entry");
	if (serviceLocation.empty())
	{
		return false;
	}

	std::lock_guard<std::mutex> clientLock(mutex);
	client = factory();
	client->StartClient(serviceLocation);

	return true;
}

template<class ClientT>
void teardownClient(typename std::unique_ptr<ClientT>& client, std::mutex& mutex)
{
	printLog("Entry");
	std::lock_guard<std::mutex> clientLock(mutex);
	if (client)
	{
		client->StopClient();
		client.reset();
	}
}

template<class ServerT>
void teardownServer(typename std::unique_ptr<ServerT>& client, std::mutex& mutex)
{
	printLog("Entry");
	std::lock_guard<std::mutex> clientLock(mutex);
	if (client)
	{
		client->StopServer();
		client.reset();
	}
}

bool setupDir()
{
	printLog("Entry");
	if (!boost::filesystem::is_directory(ConnectivityLocation))
	{
		printLog("1");
		if (!boost::filesystem::create_directories(ConnectivityLocation))
		{
			printLog("2");
			return false;
		}
	}
	printLog("3");
	if (!boost::filesystem::is_directory(InputLocation))
	{
		printLog("4");
		if (!boost::filesystem::create_directories(InputLocation))
		{
			printLog("5");
			return false;
		}
	}
	printLog("6");

	if (!boost::filesystem::is_directory(SessionStatusLocation))
	{
		printLog("7");
		if (!boost::filesystem::create_directories(SessionStatusLocation))
		{
			printLog("8");
			return false;
		}
	}
	printLog("9");
	return true;
}

} // namespace

namespace vncplugin
{

std::shared_ptr<CommunicationChannel> CommunicationChannel::Create(
	const std::string& registrationSocket)
{
	printLog("Entry");
	const std::shared_ptr<CommunicationChannel> communicationChannel (new CommunicationChannel(registrationSocket));

	communicationChannel->m_weakThis = communicationChannel;
	return communicationChannel;
}

CommunicationChannel::CommunicationChannel(
	const std::string& registrationSocket)
	: m_registrationSocket(registrationSocket)
	, m_disconnectCondition(std::make_unique<Condition>())
	, m_shutdownCondition(std::make_unique<Condition>())
{
	printLog("Entry");
}

void CommunicationChannel::setStartServerConnection(const StartServerConnectionCB& cb)
{
	m_sessionStarted = cb;
}

void CommunicationChannel::setMouseMovementConnection(const MouseMovementCB& cb)
{
	m_mouseMoved = cb;
}


void CommunicationChannel::setStartRemoteConnection(const StartRemoteConnectionCB& cb)
{
	m_RCsessionStarted = cb;
}

CommunicationChannel::~CommunicationChannel()
{
	printLog("Entry");
	shutdownInternal();
}

void CommunicationChannel::startup()
{
	printLog("Entry");
	std::lock_guard<std::mutex> shutdownLock(m_shutdownCondition->mutex);
	if (m_isRunning)
	{
		return;
	}

	if (setupDir() == false)
	{
		printLog("[Communication Channel] Directory setup failed.");
		return;
	}

	m_isRunning = true;

	m_thread = std::thread([this]()
	{
		constexpr uint8_t ShutdownRetryTime = 1; //seconds

		while (m_isRunning)
		{
			if (establishConnection())
			{
				if (setupClientAndServer())
				{
					printLog("[Communication Channel] Setup successful.");
					m_sessionStarted();
					startPing();
				}
			}

			sendDisconnect();
			tearDown();

			std::unique_lock<std::mutex> shutdownLockLoop(m_shutdownCondition->mutex);
			m_shutdownCondition->condition.wait_for(
				 shutdownLockLoop, std::chrono::seconds(ShutdownRetryTime));
		}
	});
}

void CommunicationChannel::shutdown()
{
	printLog("Entry");
	shutdownInternal();
}

void CommunicationChannel::shutdownInternal()
{
	printLog("Entry");
	sendDisconnect();

	m_isRunning = false;

	{
		std::lock_guard<std::mutex> lock(m_shutdownCondition->mutex);
		m_shutdownCondition->condition.notify_all();
	}

	if (m_thread.joinable())
	{
		m_thread.join();
	}

	tearDown();
}


void CommunicationChannel::sendStop()
{
	printLog("Entry");
	std::lock_guard<std::mutex> lock(m_sessionControlServiceClientMutex);
	if (m_sessionControlServiceClient)
	{
		const TVRemoteScreenSDKCommunication::CallStatus status =
			m_sessionControlServiceClient->CloseRc(m_communicationId);

		if (status.IsOk() == false)
		{
			printError("[Communication Channel] Error in closing the remote control. Reason: " + status.errorMessage);
		}
	}
	else
	{
		printError("[CommunicationChannel] Session control service client not available.");
	}
}

void CommunicationChannel::sendControlMode(ControlMode mode)
{
	printLog("Entry");
	std::lock_guard<std::mutex> lock(m_sessionControlServiceClientMutex);
	if (m_sessionControlServiceClient)
	{
		const TVRemoteScreenSDKCommunication::SessionControlService::ControlMode requestedControlMode = getSdkCommunicationControlMode(mode);

		const TVRemoteScreenSDKCommunication::CallStatus status =
			m_sessionControlServiceClient->ChangeControlMode(
				m_communicationId,
				requestedControlMode);

		if (status.IsOk() == false)
		{
			const std::string errorMsg =
				"[CommunicationChannel] Disabling of remote control failed. Reason: " + status.errorMessage;
			printError(errorMsg.c_str());
		}
	}
	else
	{
		printError("[CommunicationChannel] Session control service client not available.");
	}
}

void CommunicationChannel::sendScreenGrabResult(int x, int y, int w, int h, const std::string &pictureData) const
{
	//printLog("Entry");
	TVRemoteScreenSDKCommunication::CallStatus callStatus;

	std::lock_guard<std::mutex> lock(m_imageServiceClientMutex);
	if (m_imageServiceClient)
	{
		callStatus = m_imageServiceClient->UpdateImage(
			m_communicationId,
			x,
			y,
			w,
			h,
			pictureData);

		if (!callStatus.IsOk())
		{
			printError("[Communication Channel] Image update failed: ", callStatus.errorMessage);
		}
	}
	else
	{
		printError("[Communication Channel] Client not available for image service");
	}
}

void CommunicationChannel::sendImageDefinitionForGrabResult(const std::string& title,
		size_t width,
		size_t height,
		double dpi,
		ColorFormat colorFormat) const
{
	printLog("Entry");
	std::lock_guard<std::mutex> lock(m_imageServiceClientMutex);
	if (m_imageServiceClient)
	{
		const TVRemoteScreenSDKCommunication::CallStatus response =
			m_imageServiceClient->UpdateImageDefinition(
				m_communicationId,
				title,
				width,
				height,
				getSdkCommunicationColorFormat(colorFormat),
				dpi);

		if (response.IsOk() == false)
		{
			printError("[CommunicationChannel] Sending image definition failed. Reason: ", response.errorMessage);
		}
	}
	else
	{
		printError("[CommunicationChannel] Image service client not available.");
	}
}

std::string CommunicationChannel::getServerLocation(const TVRemoteScreenSDKCommunication::ServiceType serviceType) const
{
	printLog("Entry");
	for (const TVRemoteScreenSDKCommunication::RegistrationService::ServiceInformation& serviceInformation : m_serviceInformations)
	{
		if (serviceInformation.type == serviceType)
		{
			return serviceInformation.location;
		}
	}

	return std::string();
}

bool CommunicationChannel::establishConnection()
{
	printLog("Entry");
	using namespace TVRemoteScreenSDKCommunication::RegistrationService;

	std::lock_guard<std::mutex> lock(m_registrationServiceClientMutex);
	m_registrationServiceClient = TVRemoteScreenSDKCommunication::RegistrationService::ServiceFactory::CreateClient();
	m_registrationServiceClient->StartClient(m_registrationSocket);

	const IRegistrationServiceClient::ExchangeVersionResponse exchangeResponse =
		m_registrationServiceClient->ExchangeVersion(ClientVersion);

	if (exchangeResponse.callStatus.IsOk() == false)
	{
		const std::string errorMsg =
			"[CommunicationChannel] Exchange of version failed. Reason: " + exchangeResponse.callStatus.errorMessage;
		printError(errorMsg.c_str());
		return false;
	}

	const std::string ServerVersion = exchangeResponse.versionNumber;
	std::string version = ServerVersion;

	if (std::strtof(ClientVersion, nullptr) < std::strtof(ServerVersion.c_str(), nullptr))
	{
		version = ClientVersion;
	}

	const IRegistrationServiceClient::DiscoverResponse discoverResponse = m_registrationServiceClient->Discover(version);

	if (!discoverResponse.callStatus.IsOk())
	{
		const std::string errorMsg =
			"[CommunicationChannel] Version Handshake failed. Reason: " + exchangeResponse.callStatus.errorMessage;
		printError(errorMsg.c_str());
		return false;
	}

	m_communicationId = discoverResponse.communicationId;
	m_serviceInformations = discoverResponse.services;

	return true;
}

bool CommunicationChannel::setupClientAndServer()
{
	printLog("Entry");
	return setupConnectivityClient() &&
		setupImageClient() &&
		setupImageNotificationClient() &&
		setupSessionControlClient() &&
		setupSessionStatusService() &&
		setupInputService() &&
		setupConnectivityService() &&
		registerServices();
}

bool CommunicationChannel::setupConnectivityService()
{
	printLog("Entry");
	using namespace TVRemoteScreenSDKCommunication::ConnectivityService;

	std::lock_guard<std::mutex> lock(m_connectivityServiceServerMutex);
	m_connectivityServiceServer = ServiceFactory::CreateServer();

	if (m_connectivityServiceServer->StartServer(std::string(SocketPrefix) + ConnectivityLocation.string() + m_communicationId) == false)
	{
		return false;
	}

	auto availableCallback = [communicationId{m_communicationId}](
		const std::string& comId, const IConnectivityServiceServer::IsAvailableResponseCallback& response)
	{
		if (comId == communicationId)
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_connectivityServiceServer->SetIsAvailableCallback(availableCallback);

	std::weak_ptr<CommunicationChannel> weakThis = m_weakThis;
	auto disconnectCallback = [weakThis](
		const std::string& comId, const IConnectivityServiceServer::IsAvailableResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			const std::lock_guard<std::mutex> disconnectLock(communicationChannel->m_disconnectCondition->mutex);
			communicationChannel->m_disconnectCondition->condition.notify_all();
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_connectivityServiceServer->SetDisconnectCallback(disconnectCallback);

	return true;
}

bool CommunicationChannel::setupInputService()
{
	printLog("Entry");
	std::lock_guard<std::mutex> lock(m_inputServiceServerMutex);
	m_inputServiceServer = TVRemoteScreenSDKCommunication::InputService::ServiceFactory::CreateServer();

	if (!m_inputServiceServer->StartServer(std::string(SocketPrefix) + InputLocation.string() + m_communicationId))
	{
		return false;
	}

	std::weak_ptr<CommunicationChannel> weakThis = m_weakThis;

	auto receivedKeyCallback = [weakThis](
		const std::string& comId,
		TVRemoteScreenSDKCommunication::InputService::KeyState keyState,
		uint32_t xkbSymbol,
		uint32_t unicodeCharacter,
		uint32_t xkbModifiers,
		const TVRemoteScreenSDKCommunication::InputService::IInputServiceServer::KeyResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			switch(keyState)
			{
			case TVRemoteScreenSDKCommunication::InputService::KeyState::Down:
				communicationChannel->m_keyboardKeyEvent(xkbSymbol, unicodeCharacter, xkbModifiers, false);
				break;
			case TVRemoteScreenSDKCommunication::InputService::KeyState::Up:
				communicationChannel->m_keyboardKeyEvent(xkbSymbol, unicodeCharacter, xkbModifiers, true);
				break;
			case TVRemoteScreenSDKCommunication::InputService::KeyState::Unknown:
				break;
			}

			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_inputServiceServer->SetReceivedKeyCallback(receivedKeyCallback);

	auto mouseMoveCallback = [weakThis](
		const std::string& comId,
		int32_t posX,
		int32_t posY,
		const TVRemoteScreenSDKCommunication::InputService::IInputServiceServer::MouseMoveResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			communicationChannel->m_mouseMoved(posX, posY);
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_inputServiceServer->SetMouseMoveCallback(mouseMoveCallback);

	auto mousePressReleaseCallback = [weakThis](
		const std::string& comId,
		TVRemoteScreenSDKCommunication::InputService::KeyState keyState,
		int32_t posX,
		int32_t posY,
		TVRemoteScreenSDKCommunication::InputService::MouseButton button,
		const TVRemoteScreenSDKCommunication::InputService::IInputServiceServer::MousePressReleaseResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			switch(button)
			{
			case TVRemoteScreenSDKCommunication::InputService::MouseButton::Left:
				communicationChannel->m_mouseClick(posX, posY, 1);
				break;
			case TVRemoteScreenSDKCommunication::InputService::MouseButton::Middle:
				communicationChannel->m_mouseClick(posX, posY, 2);
				break;
			case TVRemoteScreenSDKCommunication::InputService::MouseButton::Right:
				communicationChannel->m_mouseClick(posX, posY, 3);
				break;
			default:
				response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
				return;
			}

			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_inputServiceServer->SetMousePressReleaseCallback(mousePressReleaseCallback);

	auto mouseWheelCallback = [weakThis](
		const std::string& comId,
		int32_t posX,
		int32_t posY,
		int32_t angle,
		const TVRemoteScreenSDKCommunication::InputService::IInputServiceServer::MouseWheelResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_inputServiceServer->SetMouseWheelCallback(mouseWheelCallback);

	return true;
}

bool CommunicationChannel::setupSessionStatusService()
{
	printLog("Entry");
	using namespace TVRemoteScreenSDKCommunication::SessionStatusService;

	std::lock_guard<std::mutex> lock(m_sessionStatusServiceServerMutex);
	m_sessionStatusServiceServer = ServiceFactory::CreateServer();

	if (m_sessionStatusServiceServer->StartServer(std::string(SocketPrefix) + SessionStatusLocation.string() + m_communicationId) == false)
	{
		return false;
	}

	std::weak_ptr<CommunicationChannel> weakThis = m_weakThis;
	auto remoteControlStarted = [weakThis](
		const std::string& comId,
		const GrabStrategy strategy,
		const ISessionStatusServiceServer::RemoteControllStartedResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));

			communicationChannel->m_RCsessionStarted();
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_sessionStatusServiceServer->SetRemoteControlStartedCallback(remoteControlStarted);

	auto remoteControlStopped = [weakThis](
		const std::string& comId,
		const ISessionStatusServiceServer::RemoteControllStartedResponseCallback& response)
	{
		printLog("Entry");
		const std::shared_ptr<CommunicationChannel> communicationChannel = weakThis.lock();
		if (communicationChannel && (communicationChannel->m_communicationId == comId))
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Ok));
			communicationChannel->m_RCsessionStopped();
		}
		else
		{
			response(TVRemoteScreenSDKCommunication::CallStatus(TVRemoteScreenSDKCommunication::CallState::Failed));
		}
	};
	m_sessionStatusServiceServer->SetRemoteControlStoppedCallback(remoteControlStopped);

	return true;
}

bool CommunicationChannel::setupConnectivityClient()
{
	printLog("Entry");
	const std::string ServiceLocation = getServerLocation(TVRemoteScreenSDKCommunication::ServiceType::Connectivity);
	return setupClient(
		ServiceLocation,
		TVRemoteScreenSDKCommunication::ConnectivityService::ServiceFactory::CreateClient,
		m_connectivityServiceClient,
		m_connectivityServiceClientMutex);
}

bool CommunicationChannel::setupImageClient()
{
	printLog("Entry");
	const std::string ServiceLocation = getServerLocation(TVRemoteScreenSDKCommunication::ServiceType::Image);
	return setupClient(
		ServiceLocation,
		TVRemoteScreenSDKCommunication::ImageService::ServiceFactory::CreateClient,
		m_imageServiceClient,
		m_imageServiceClientMutex);
}

bool CommunicationChannel::setupImageNotificationClient()
{
	printLog("Entry");
	const std::string ServiceLocation = getServerLocation(TVRemoteScreenSDKCommunication::ServiceType::ImageNotification);
	return setupClient(
		ServiceLocation,
		TVRemoteScreenSDKCommunication::ImageNotificationService::ServiceFactory::CreateClient,
		m_imageNotificationServiceClient,
		m_imageNotificationServiceClientMutex);
}

bool CommunicationChannel::setupSessionControlClient()
{
	printLog("Entry");
	const std::string ServiceLocation = getServerLocation(TVRemoteScreenSDKCommunication::ServiceType::SessionControl);
	return setupClient(
		ServiceLocation,
		TVRemoteScreenSDKCommunication::SessionControlService::ServiceFactory::CreateClient,
		m_sessionControlServiceClient,
		m_sessionControlServiceClientMutex);
}

bool CommunicationChannel::registerServices() const
{
	printLog("Entry");
	TVRemoteScreenSDKCommunication::CallStatus status;

	std::lock_guard<std::mutex> lock(m_registrationServiceClientMutex);

	status = m_registrationServiceClient->RegisterService(
		m_communicationId,
		TVRemoteScreenSDKCommunication::ServiceType::Connectivity,
		std::string(SocketPrefix) + ConnectivityLocation.string() + m_communicationId);

	if (!status.IsOk())
	{
		printLog("Error registering service ", ConnectivityLocation.string());
		return false;
	}

	status = m_registrationServiceClient->RegisterService(
		m_communicationId,
		TVRemoteScreenSDKCommunication::ServiceType::Input,
		std::string(SocketPrefix) + InputLocation.string() + m_communicationId);

	if (!status.IsOk())
	{
		printLog("Error registering service ", InputLocation.string());
		return false;
	}

	status = m_registrationServiceClient->RegisterService(
		m_communicationId,
		TVRemoteScreenSDKCommunication::ServiceType::SessionStatus,
		std::string(SocketPrefix) + SessionStatusLocation.string() + m_communicationId);

	if (!status.IsOk())
	{
		printLog("Error registering service ", SessionStatusLocation.string());
		return false;
	}

	return true;
}

void CommunicationChannel::sendDisconnect() const
{
	printLog("Entry");
	std::lock_guard<std::mutex> connectivityServiceClientLock(m_connectivityServiceClientMutex);
	if (m_connectivityServiceClient)
	{
		m_connectivityServiceClient->Disconnect(m_communicationId);
	}

	std::lock_guard<std::mutex> disconnectConditionLock(m_disconnectCondition->mutex);
	m_disconnectCondition->condition.notify_all();
}

void CommunicationChannel::startPing()
{
	printLog("Entry");
	constexpr uint8_t PingTimeout = 5; //seconds

	std::unique_lock<std::mutex> lock(m_disconnectCondition->mutex);
	while (m_isRunning &&
		   m_disconnectCondition->condition.wait_for(lock, std::chrono::seconds(PingTimeout)) == std::cv_status::timeout)
	{
		std::unique_lock<std::mutex> connectivityServiceClientLock(m_connectivityServiceClientMutex);
		if (m_connectivityServiceClient &&
			m_connectivityServiceClient->IsAvailable(m_communicationId).IsOk())
		{
			connectivityServiceClientLock.unlock();
		}
		else
		{
			break;
		}
	}
}

void CommunicationChannel::tearDown()
{
	printLog("Entry");
	teardownServer(m_connectivityServiceServer,  m_connectivityServiceServerMutex);
	teardownServer(m_inputServiceServer,         m_inputServiceServerMutex);
	teardownServer(m_sessionStatusServiceServer, m_sessionStatusServiceServerMutex);

	teardownClient(m_connectivityServiceClient,      m_connectivityServiceClientMutex);
	teardownClient(m_imageNotificationServiceClient, m_imageNotificationServiceClientMutex);
	teardownClient(m_imageServiceClient,             m_imageServiceClientMutex);
	teardownClient(m_registrationServiceClient,      m_registrationServiceClientMutex);
	teardownClient(m_sessionControlServiceClient,    m_sessionControlServiceClientMutex);
}

} // namespace vncplugin
