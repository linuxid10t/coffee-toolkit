# Makefile – Coffee Toolkit

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter \
           -I/boot/system/develop/headers/private
LIBS     = -lbe -lroot -ltranslation -ltracker -lshared
RC       = rc
TARGET   = coffee_toolkit
RDEF     = coffee_toolkit.rdef
RSRC     = coffee_toolkit.rsrc

SRCS = main.cpp \
       MainWindow.cpp \
       BrewRatioWindow.cpp \
       ExtractionWindow.cpp \
       RoastColorWindow.cpp \
       DetailWindow.cpp

OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) $(RSRC)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)
	xres -o $@ $(RSRC)

$(RSRC): $(RDEF)
	$(RC) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Header dependencies
main.o:              main.cpp MainWindow.h
MainWindow.o:        MainWindow.cpp MainWindow.h Constants.h \
                     BrewRatioWindow.h ExtractionWindow.h \
                     RoastColorWindow.h DetailWindow.h
BrewRatioWindow.o:   BrewRatioWindow.cpp BrewRatioWindow.h Constants.h
ExtractionWindow.o:  ExtractionWindow.cpp ExtractionWindow.h Constants.h
RoastColorWindow.o:  RoastColorWindow.cpp RoastColorWindow.h Constants.h
DetailWindow.o:      DetailWindow.cpp DetailWindow.h Constants.h

clean:
	rm -f $(OBJS) $(TARGET) $(RSRC)
