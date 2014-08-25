ARDUINO_VERS=104
# GLFAG = 
OFLAG = -Os

#AVRDUDECONFIG = 

LIBNAME=sprinklers_avr
VERSION=1.0.0

EXTRAFLAGS = -DUSB_VID=null -DUSB_PID=null
SKETCHBOOK = /home/rzimmerman/Arduino
ARDUINO_DIR = /home/rzimmerman/arduino-1.0.4
ARDUINO = $(ARDUINO_DIR)
AVR_TOOLS_DIR=$(ARDUINO_DIR)/hardware/tools/avr
PATH += $(ARDUINO)/hardware/tools/avr/bin
include $(SKETCHBOOK)/Arduino.mk

package: clean
	@mkdir -p /tmp/$(LIBNAME)-$(VERSION)
	@cp -rf * /tmp/$(LIBNAME)-$(VERSION)
	@rm -rf /tmp/$(LIBNAME)-$(VERSION)/web/json
	@rm -rf /tmp/$(LIBNAME)-$(VERSION)/web/bin
	tar -C/tmp -czvhf $(LIBNAME)-$(VERSION).tar.gz $(LIBNAME)-$(VERSION)
	rm -rf /tmp/$(LIBNAME)-$(VERSION)

