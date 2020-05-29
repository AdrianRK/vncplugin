/*
 * =====================================================================================
 *
 *       Filename:  ICommunicationChannel.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  29/05/2020 19:50:28
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once
#include <functional>

namespace vncplugin
{

enum class ControlMode
{
	Disabled,
	ViewOnly,
	FullControl,
};

enum class ColorFormat
{
	Unsupported,
	BGRA32,
	RGBA32,
	R5G6B5,
};

using StartServerConnectionCB = std::function<void(void)>;
using StartRemoteConnectionCB = std::function<void(void)>;
using StopRemoteConnectionCB = std::function<void(void)>;
using MouseMovementCB = std::function<void(int, int)>;
using KeyboardKeyEventCB = std::function<void(int, int, int, bool)>;
using MouseClickCB = std::function<void(int, int, int)>;

class ICommunicationChannel
{
public:
	virtual ~ICommunicationChannel(){}
	virtual void startup() = 0;

	virtual void shutdown() = 0;

	virtual void sendStop() = 0;

	virtual void sendControlMode(ControlMode) = 0;

	virtual void sendScreenGrabResult(
			int,
			int,
			int,
			int,
			const std::string &) const = 0;

	virtual void setImageDefinition(const std::string&,
			size_t,
			size_t,
			double,
			ColorFormat) = 0;

	virtual void sendImageDefinitionForGrabResult() const  = 0;

	virtual void setStartServerConnection(const StartServerConnectionCB&) = 0;
	virtual void setMouseMovementConnection(const MouseMovementCB&) = 0;
	virtual void setStartRemoteConnection(const StartRemoteConnectionCB&) = 0;
	virtual void setMouseClickCB(const MouseClickCB&) = 0;
	virtual void setStopRemoteConnectionCB(const StopRemoteConnectionCB&) = 0;
	virtual void setKeyboardKeyEventCB(const KeyboardKeyEventCB&) = 0;
};

} // namespace vncplugin
