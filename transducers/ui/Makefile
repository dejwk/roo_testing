ROOTDIR = ../../..
FLTK_DIR=~/fltk-1.3.4-2
ARDUINO_LIBS = esp-display esp-tft-font-FinalFrontier-25
CC = clang++
IDIR =
CFLAGS=-g -std=c++11 -I. -I $(FLTK_DIR)/ -I ../devices/ -I ../../esp-tft-font-FinalFrontier-25 -I ./Arduino -I .. -I ../../../src/ 
LFLAGS=-L ./fltk-1.3.4-2/lib
LIBS = -lfltk_images -lfltk_png -lfltk_z -lfltk_jpeg -lfltk -lpthread -ldl -lm -lX11
#DEPS = emulator.cpp Arduino/Serial.cpp Arduino/Print.cpp Arduino/hal.cpp Arduino/WString.cpp Arduino/stdlib_noniso.cpp ../smooth_font.cpp ../../../src/benchmark.cpp ../smooth_font.cpp ../fixed_glcd_8_5.cpp ../../esp-tft-font-FinalFrontier-25/esp-tft-font-FinalFrontier-25.cpp
LDIR = $(FLTK_DIR)/lib
ODIR = obj
DEFINES = -D ESP_DISPLAY_EMULATOR

_OBJFILES=${shell (cd $(ROOTDIR); for i in $(ARDUINO_LIBS); do find lib/$$i -name "*.cpp"; done; find src/*.cpp) | sed 's/.cpp/.o/'}
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJFILES))

$(ODIR)/%.o: $(ROOTDIR)/%.cpp #$(DEPS)
	mkdir -p $(shell dirname $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFINES)

$(ODIR)/%.o: ./%.cpp #$(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFINES)

emulator: $(OBJ)
	$(CC) -o emulator $(OBJ) -L $(LDIR) -lfltk -lX11 -lm -ldl -lpthread

fontgen: fontgen.cpp
	clang++ -std=c++11 fontgen.cpp -o fontgen

clean:
	rm -rf $(ODIR)

test:
#echo $(_OBJFILES)
	echo $(shell (cd $(ROOTDIR); for i in $(ARDUINO_LIBS); do echo lib/$$i; done))

#LFLAGS = -L fltk-1.3.4-2/lib
