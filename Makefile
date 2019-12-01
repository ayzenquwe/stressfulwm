CFLAGS?=-Os -pedantic -Wall

all:
	$(CC) $(CFLAGS) stressfulwm.c -lX11 -o stressfulwm

clean:
	rm -f stressfulwm

