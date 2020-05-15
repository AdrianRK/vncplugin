####################################################################################################
#                                     Make file VNC plugin 	                                   #
####################################################################################################
INC:= -I../libvncserver/ -I./inc/
CFLAGS:= -c -Wall $(INC) --std=c++11
SOURCES:= main.cpp 

OBJDIR:=./obj
BINDIR:=./bin
SRCDIR:=./src

LDFLAGS:= -L../libvncserver/build

OBJECTS:=$(SOURCES:.cpp=.o)

CC:=g++
EXECUTABLE:=vncplugin
LIBRARIES:= -lvncclient

all: cln MAKEDIR $(EXECUTABLE) 

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(addprefix $(OBJDIR)/, $(OBJECTS)) -o $(BINDIR)/$@ $(LIBRARIES)
#	cp $(BINDIR)/$@ ./vncplugin/

$(OBJECTS): $(SRCDIR)/$(SOURCES)
	$(CC) $(CFLAGS) $^ -o $(OBJDIR)/$@

cln:
	rm -rf $(BINDIR)/$(EXECUTABLE)
	
clean:
	rm -rf $(BINDIR)/$(EXECUTABLE) $(addprefix $(OBJDIR)/, $(OBJECTS)) $(BINDIR) $(OBJDIR)

MAKEDIR:
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)
#	mkdir -p ./vncplugin
