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
#include <csignal>
#include "logging.hpp"
#include "CommunicationChannel.h"
#include "VNCServerWrapper.h"
#include "VNCMediator.h"

constexpr const char* RegistrationServiceLocation =
	"unix:///tmp/teamviewer-iot-agent-services/remoteScreen/registrationService";

std::shared_ptr<vncplugin::VNCMediator> vncmediator;

void handleSig(int sig)
{
	printLog ("Received signal ", sig);
	if (vncmediator)
	{
		vncmediator->stop();
	}
	exit(1);
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


	std::shared_ptr<vncplugin::CommunicationChannel> comm = vncplugin::CommunicationChannel::Create(RegistrationServiceLocation);

	std::shared_ptr<vncplugin::VNCServerWrapper> wncserver = vncplugin::VNCServerWrapper::create(argv,argc);

	vncmediator = vncplugin::VNCMediator::create(comm, wncserver);

	vncmediator->start();

	while (1);

	return 0;
}

