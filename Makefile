MAJOR := 0
MINOR := 1
NAME := kms_egl
VERSION := $(MAJOR).$(MINOR)

OBJS := \
	kms_egl_cdc.o \
	kms_helper.o


CFLAGS += -MMD -fPIC -shared 
CFLAGS += $(shell pkg-config --cflags libdrm)
ALL_LDFLAGS += $(LDFLAGS) -Wl,--no-undefined -shared -ldrm -lkms


#
# Rules
#

.PHONY:
all : lib$(NAME).so

lib$(NAME).so : lib$(NAME).so.$(MAJOR)
	ln -s $^ $@

lib$(NAME).so.$(MAJOR) : lib$(NAME).so.$(VERSION)
	ln -s $^ $@

lib$(NAME).so.$(VERSION) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-soname,lib$(NAME).so.$(MAJOR) -o $@ $^

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $^ $(ALL_LDFLAGS)

.PHONY:
clean:
	rm -f *.o *.d lib$(NAME).so*

.PHONY:
deploy: $(TARGET)
	cp kms_egl.h $(SDKTARGETSYSROOT)/usr/include/
	cp lib$(NAME).so.$(VERSION) $(SDKTARGETSYSROOT)/usr/lib/
	scp lib$(NAME).so.$(VERSION) root@$(BOARD_IP):/usr/lib/
