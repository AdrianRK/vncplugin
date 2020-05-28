####################################################################################################
#                                     Make file VNC plugin 	                                   #
####################################################################################################
INC:= -I../inc/ -I../CommunicationLayer/lib/include 
INC:= $(INC) -DDISABLE_LOG
CFLAGS:= -c -Wall $(INC) --std=c++14 -lz
SOURCES:= CommunicationChannel.cpp \
	main.cpp

OBJDIR:=../obj
BINDIR:=../bin

LDFLAGS:= -L../CommunicationLayer/lib/lib -L/usr/local/lib -Wl,-v -Wl,--copy-dt-needed-entries

OBJECTS:=$(SOURCES:.cpp=.o)

CC:=g++
EXECUTABLE:=vncplugin
LIBRARIES:= -lvncclient \
	-pthread \
	-lboost_filesystem \
	-lboost_system \
	-l:libz.a \
	-l:libConnectivityService.a \
	-l:libImageNotificationService.a \
	-l:libImageService.a \
	-l:libInputService.a \
	-l:libRegistrationService.a \
	-l:libServiceBase.a \
	-l:libSessionControlService.a \
	-l:libSessionStatusService.a \
	-l:libprotoc.a \
	-l:libgrpc.a \
	-l:libgrpc++.a \
	-l:libgrpc++_reflection.a \
	-l:libgrpc_unsecure.a \
	-l:libgrpc++_unsecure.a \
	-l:libprotoc.a \
	-l:libprotobuf.a

all: cln MAKEDIR $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(addprefix $(OBJDIR)/, $(OBJECTS)) -o $(BINDIR)/$@ $(LIBRARIES)
	
.cpp.o:
	$(CC) $(CFLAGS) $< -o $(OBJDIR)/$@

cln:
	rm -rf $(BINDIR)/$(EXECUTABLE)
	
clean:
	rm -rf $(BINDIR)/$(EXECUTABLE) $(addprefix $(OBJDIR)/, $(OBJECTS)) $(BINDIR) $(OBJDIR)

MAKEDIR:
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)
