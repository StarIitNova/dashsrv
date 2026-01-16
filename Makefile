MONGOOSE_C=vendor/mongoose/mongoose.c

CC=gcc
CXX=g++

CFLAGS=-Ivendor/mongoose -Ivendor -Wextra -Wall -Iinclude -O2 -g
CXXFLAGS:=$(CFLAGS) -std=gnu++23
LDFLAGS=

CFLAGS += -std=gnu23

BIN=bin
TARGET=dashsrv

SOURCES=source/Dashsrv.cpp source/Basic.cpp source/MGClient.cpp source/Hardware.cpp \
	source/Server/ServiceHandler.cpp source/Server/Routes.cpp \
	source/Server/Core/HTTP.cpp source/Server/Core/Rand.cpp source/Server/Core/Server.cpp source/Server/Config.cpp \
	source/Minecraft/MCPacket.cpp source/Minecraft/MCQuery.cpp source/Minecraft/Status.cpp source/Minecraft/String.cpp \
	$(MONGOOSE_C)

OBJECTS=$(patsubst %.c,$(BIN)/%.o,$(patsubst %.cpp,$(BIN)/%.o,$(SOURCES)))
DEPS=$(patsubst %.o,%.d,$(OBJECTS))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BIN)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BIN) $(TARGET)

.PHONY: all clean
