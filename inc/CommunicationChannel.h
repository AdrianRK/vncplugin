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
#pragma once

#include <TVRemoteScreenSDKCommunication/CommunicationLayerBase/ServiceType.h>

#include <TVRemoteScreenSDKCommunication/ImageService/ColorFormat.h>

#include <TVRemoteScreenSDKCommunication/SessionStatusService/GrabStrategy.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>

namespace TVRemoteScreenSDKCommunication
{
namespace ConnectivityService
{
class IConnectivityServiceServer;
class IConnectivityServiceClient;
} // namespace ConnectivityService

namespace ImageService
{
class IImageServiceClient;
} // namespace ImageService

namespace ImageNotificationService
{
class IImageNotificationServiceClient;
} // ImageNotificationService

namespace InputService
{
class IInputServiceServer;
} // namespace InputService

namespace RegistrationService
{
class IRegistrationServiceClient;
struct ServiceInformation;
} // namespace RegistrationService

namespace SessionControlService
{
class ISessionControlServiceClient;
} // namespace SessionControlService

namespace SessionStatusService
{
class ISessionStatusServiceServer;
} // namespace SessionStatusService

} // namespace TVRemoteScreenSDKCommunication

enum class ControlMode
{
	Disabled,
	ViewOnly,
	FullControl,
};

class CommunicationChannel
{
public:
	static std::shared_ptr<CommunicationChannel> Create(const std::string& registrationSocket);
	~CommunicationChannel();

public:
	void startup();
	void shutdown();

	void sendStop();

	void sendControlMode(ControlMode mode);

	void sendScreenGrabResult(int x, int y, int w, int h, const std::string &pictureData) const;
	//void sendImageDefinitionForGrabResult(
	//	const QString& title,
	//	QSize size,
	//	double dpi,
	//	ColorFormat colorFormat) const;

	void sendGrabRequest(int x, int y, int w, int h) const;
	//void sendImageDefinitionForGrabRequest(
		//const QString& title,
		//QSize size) const;

private:
	CommunicationChannel(const std::string& registrationSocket);

	void shutdownInternal();

	std::string getServerLocation(const TVRemoteScreenSDKCommunication::ServiceType serviceType) const;

	bool establishConnection();

	bool setupClientAndServer();

	bool setupConnectivityService();
	bool setupInputService();
	bool setupSessionStatusService();

	bool setupConnectivityClient();
	bool setupImageClient();
	bool setupImageNotificationClient();
	bool setupSessionControlClient();

	bool registerServices() const;
	void sendDisconnect() const;
	void startPing();

	void tearDown();

	struct Condition
	{
		std::condition_variable condition;
		std::mutex mutex;
	};

	const std::string m_registrationSocket;
	const std::unique_ptr<Condition> m_disconnectCondition;
	const std::unique_ptr<Condition> m_shutdownCondition;

	std::mutex m_inputServiceServerMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::InputService::IInputServiceServer> m_inputServiceServer;

	std::mutex m_connectivityServiceServerMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::ConnectivityService::IConnectivityServiceServer> m_connectivityServiceServer;

	std::mutex m_sessionStatusServiceServerMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::SessionStatusService::ISessionStatusServiceServer> m_sessionStatusServiceServer;

	mutable std::mutex m_imageServiceClientMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::ImageService::IImageServiceClient> m_imageServiceClient;

	mutable std::mutex m_connectivityServiceClientMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::ConnectivityService::IConnectivityServiceClient> m_connectivityServiceClient;

	mutable std::mutex m_registrationServiceClientMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::RegistrationService::IRegistrationServiceClient> m_registrationServiceClient;

	std::mutex m_sessionControlServiceClientMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::SessionControlService::ISessionControlServiceClient> m_sessionControlServiceClient;

	mutable std::mutex m_imageNotificationServiceClientMutex;
	std::unique_ptr<TVRemoteScreenSDKCommunication::ImageNotificationService::IImageNotificationServiceClient> m_imageNotificationServiceClient;

	std::string m_communicationId;
	std::vector<TVRemoteScreenSDKCommunication::RegistrationService::ServiceInformation> m_serviceInformations;

	std::thread m_thread;

	std::atomic_bool m_isRunning{false};

	std::weak_ptr<CommunicationChannel> m_weakThis;
};

