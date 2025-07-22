
# default configuration
CC 					?= gcc
CFLAGS 			:= $(CFLAGS) -g -I src 
LDFLAGS 		:= $(LDFLAGS) -lm
LDFLAGS_GFX := -lX11 -lXext

BUILD_DIR := build
DEMO_OUT	:= demo

all: demos

# 
# targets
# 
demos: balls

# balls
BALLS_DEST := $(BUILD_DIR)/balls/data
BALLS_SRC := src/demos/balls/data
BALLS_FILES := $(patsubst $(BALLS_SRC)/%,$(BALLS_DEST)/%,$(wildcard $(BALLS_SRC)/*))
$(BALLS_DEST):
	mkdir -p $(BALLS_DEST)
$(BALLS_DEST)/%: $(BALLS_SRC)/%
	cp -f $< $@
balls: $(BALLS_DEST) $(BALLS_FILES)
	$(CC) $(LDFLAGS) $(LDFLAGS_GFX) $(CFLAGS) src/demos/balls/main.c -DWAYLAND -o $(BUILD_DIR)/balls/$(DEMO_OUT)

clean:
	rm -f build/*

# 
# preset configurations
#
emcc: CC:=emcc
emcc: LDFLAGS:=$(LDFLAGS)
emcc: LDFLAGS_GFX:=-sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS
emcc: DEMO_OUT:=demo.html
emcc: CFLAGS:=$(CFLAGS) -pthread -sINITIAL_MEMORY=1024mb -sALLOW_MEMORY_GROWTH=1 -sTOTAL_STACK=512mb --embed-file ./$(BALLS_SRC)@data
emcc: demos
