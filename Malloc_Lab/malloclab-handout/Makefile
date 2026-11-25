#
# Students' Makefile for the Malloc Lab
#
TEAM = bovik
VERSION = 1
HANDINDIR = /afs/cs.cmu.edu/academic/class/15213-f01/malloclab/handin

CC = gcc
CFLAGS = -Wall -O2 -m64

BUILD_DIR = .build64
OBJS = $(addprefix $(BUILD_DIR)/,mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o)

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

-include $(OBJS:.o=.d)

handin:
	cp mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

clean:
	rm -rf $(BUILD_DIR)
	rm -f mdriver


