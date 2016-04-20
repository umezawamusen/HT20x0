CC = arm-linux-gnueabi-gcc
CFLAGS = -I/usr/arm-linux/include
LDFLAGS = -L/usr/arm-linux/lib

TARGET = ht2030_sample

all: $(TARGET)

ht2030_sample: ht2030_sample.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f *.o *~ core *.bak $(TARGET)
