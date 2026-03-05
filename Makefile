# Makefile – Coffee Toolkit

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter \
           -I/boot/system/develop/headers/private
LIBS     = -lbe -lroot -ltranslation -ltracker -lshared
TARGET   = coffee_toolkit

SRCS = main.cpp \
       MainWindow.cpp \
       BrewRatioWindow.cpp \
       ExtractionWindow.cpp \
       RoastColorWindow.cpp \
       DetailWindow.cpp

OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

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
	rm -f $(OBJS) $(TARGET)
