/*
 * =====================================================================================
 *
 *       Filename:  VNCServerWrapper.h
 *
 *    Description:  VNC Server wrapper
 *
 *        Version:  1.0
 *        Created:  28/05/2020 22:09:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef VNC_CLIENT_VNC_SERVER_WRAPPER_HEADER_
#define VNC_CLIENT_VNC_SERVER_WRAPPER_HEADER_

#include "IVNCServerWrapper.h"
#include <thread>
#include <string>
#include <atomic>
#include <rfb/rfbclient.h>

namespace vncplugin
{

class VNCServerWrapper: public IVNCServerWrapper
{
public:

	static std::shared_ptr<VNCServerWrapper> create(char** arguments, int count);

	virtual ~VNCServerWrapper();

	void setPassword(const std::string& password) override {m_password = password;}
	void setUser(const std::string& user) override {m_user = user;}

	void start() override;
	void stop() override;

	void setSetImageDefinition(const SetImageDefinitionCB& cb) override
	{
		m_setImageDefinition = cb;
	}

	void setUpdateBuffer(const UpdateBufferCB& cb) override
	{
		m_updateBuffer = cb;
	}

	void setGetUpdateType(const GetUpdateTypeCB& cb) override
	{
		m_getUpdateType = cb;
	}

	bool isRunning() const override
	{
		return m_isRunning;
	}

	void sendKeyEvent(int, bool) override;
	void sendMouseEvent(int, int, int) override;

private:

	VNCServerWrapper();
	VNCServerWrapper(const VNCServerWrapper&) = delete;
	VNCServerWrapper(VNCServerWrapper&&) = delete;

	void init(char** arguments, int count);
	void setupServer();
	void cleanup();

	char** m_arguments = nullptr;
	int m_count = 0;
	rfbClient* m_cl = nullptr;
	std::thread m_thread;
	std::string m_password {};
	std::string m_user {};
	std::atomic<bool> m_isRunning {false};

	std::weak_ptr<VNCServerWrapper> m_weakThis;

	SetImageDefinitionCB m_setImageDefinition;
	UpdateBufferCB m_updateBuffer;
	GetUpdateTypeCB m_getUpdateType;
};

} // namespace vncplugin

#endif // VNC_CLIENT_VNC_SERVER_WRAPPER_HEADER_
