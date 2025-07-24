
# default configuration
CC 			?= gcc
CFLAGS 		:= $(CFLAGS) -g -I src 
LDFLAGS 	:= $(LDFLAGS) -lm
LDFLAGS_GFX := -lX11 -lXext

BUILD_DIR 	:= build
BUILD_EXT   := 

all: demos

#
# data files
#
DATA_DIR := data
%: $(DATA_DIR)/% 
	mkdir -p $(BUILD_DIR)/$(DATA_DIR)
	cp -f $< $(BUILD_DIR)/$(DATA_DIR)/$@

# 
# targets
# 
demos: $(DST_DATA_DIR) balls hanging_boxes

balls: sphere.obj
	$(CC) $(CFLAGS) src/demos/$@/main.c $(LDFLAGS) $(LDFLAGS_GFX) -o $(BUILD_DIR)/$@$(BUILD_EXT)
hanging_boxes: cube.obj
	$(CC) $(CFLAGS) src/demos/$@/main.c $(LDFLAGS) $(LDFLAGS_GFX) -o $(BUILD_DIR)/$@$(BUILD_EXT)

clean:
	rm -rf build/*

# 
# preset configurations
#
emcc: CC:=emcc
emcc: LDFLAGS:=$(LDFLAGS)
emcc: LDFLAGS_GFX:=-sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS
emcc: BUILD_EXT:=.html
emcc: CFLAGS:=$(CFLAGS) -pthread -sINITIAL_MEMORY=1024mb -sALLOW_MEMORY_GROWTH=1 -sTOTAL_STACK=512mb
emcc: all
