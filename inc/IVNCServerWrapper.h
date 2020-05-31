/*
 * =====================================================================================
 *
 *       Filename:  IVNCServerWrapper.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  29/05/2020 19:50:14
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef VNC_CLIENT_IVNC_SERVER_WRAPPER_HEADER_
#define VNC_CLIENT_IVNC_SERVER_WRAPPER_HEADER_

#include <functional>

namespace vncplugin
{

enum class UpdateType
{
	FullBufferUpdate,
	PartialBufferUpdate,
	NoBufferUpdate
};

using SetImageDefinitionCB = std::function<void(const std::string&, int, int, double, int)>;
using UpdateBufferCB = std::function<void(const unsigned char*, int, int, int, int, int)>;
using GetUpdateTypeCB = std::function <UpdateType(void)>;
using VNCServerDisconnectCB = std::function <void(bool)>;

class IVNCServerWrapper
{
public:
	virtual ~IVNCServerWrapper(){}
	virtual void setPassword(const std::string&) = 0;
	virtual void setUser(const std::string&) = 0;

	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void setSetImageDefinition(const SetImageDefinitionCB&) = 0;
	virtual void setUpdateBuffer(const UpdateBufferCB&) = 0;
	virtual void setGetUpdateType(const GetUpdateTypeCB&) = 0;
	virtual void setVNCServerDisconnect(const VNCServerDisconnectCB&) = 0;

	virtual bool isRunning() const  = 0;
	virtual void sendKeyEvent(int, bool) = 0;
	virtual void sendMouseEvent(int, int, int) = 0;

};
} // vncplugin

#endif // VNC_CLIENT_IVNC_SERVER_WRAPPER_HEADER_
