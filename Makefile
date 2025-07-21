BUILD_DIR = build
DEMO_DATA_DIR = src/demo/data

all: demo

COPY_DEMO_FILES := $(patsubst $(DEMO_DATA_DIR)/%,build/%,$(wildcard $(DEMO_DATA_DIR)/*))
$(BUILD_DIR)/%: $(DEMO_DATA_DIR)/%
	cp -f $< $@

demo: $(wildcard *.c) $(COPY_DEMO_FILES)
	g++ -g -I src src/demo/main.c -lX11 -lXext -DWAYLAND -o $(BUILD_DIR)/demo

clean:
	rm -f build/*
