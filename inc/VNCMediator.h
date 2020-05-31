/*
 * =====================================================================================
 *
 *       Filename:  vncMediator.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  29/05/2020 17:02:05
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef VNC_CLIENT_VNC_MEDIATOR_HEADER_
#define VNC_CLIENT_VNC_MEDIATOR_HEADER_

#include <memory>

namespace vncplugin
{

class ICommunicationChannel;
class IVNCServerWrapper;

class VNCMediator
{
public:
	static std::shared_ptr<VNCMediator> create(const std::shared_ptr<ICommunicationChannel>&, const std::shared_ptr<IVNCServerWrapper>&);
	~VNCMediator();

	void start();
	void stop();

private:
	VNCMediator(const std::shared_ptr<ICommunicationChannel>&, const std::shared_ptr<IVNCServerWrapper>&);
	void init();

	VNCMediator() = delete;
	VNCMediator(const VNCMediator&) = delete;

	const std::shared_ptr<ICommunicationChannel> m_comm;
	const std::shared_ptr<IVNCServerWrapper> m_vncserver;

	std::weak_ptr<VNCMediator> m_weakThis;
};

} // vncplugin

#endif // VNC_CLIENT_VNC_MEDIATOR_HEADER_
