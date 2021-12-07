all: inject hostapd

inject: inject.c
	gcc -Wall -O3 -o inject inject.c

hostapd:
	cd hostap/hostapd; $(MAKE) hostapd

clean:
	rm -f inject
