PROJ_NAME = Image1NORTool

BUILD_ROOT = build

OPTIONS += -DPROJ_NAME=\"$(PROJ_NAME)\"
CFLAGS += -arch x86_64 -arch arm64
CFLAGS += -Iinclude
CFLAGS += -MMD

SOURCES = main.c utils.c image1_nor.c

TARGET = $(BUILD_ROOT)/$(PROJ_NAME)

all: $(TARGET)

$(TARGET): $(SOURCES) Makefile
	$(CC) $(CFLAGS) $(OPTIONS) $(SOURCES) -o $@

-include $(TARGET).d

.PHONY: clean

clean:
	rm -rf $(BUILD_ROOT)
