JAVA_HOME ?= /usr/lib/jvm/default

CXXFLAGS += -std=c++14 -O2 -pedantic -Wall -Wextra -Wno-unused-parameter
INCLUDE += -I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/linux

SRC_FILES = src/agent.cpp

all: build build_example

build:
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(SRC_FILES) -shared -fpic -o bin/sync_jvmti.so

build_example:
	cd java; $(JAVA_HOME)/bin/javac -g HelloWorld.java

