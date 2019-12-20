CC=gcc

.PHONY: clean

klink: klink.c
	$(CC) -o '$@' '$<'
#	$(STRIP) klink

clean:
	rm klink
