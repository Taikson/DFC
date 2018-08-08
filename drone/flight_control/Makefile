LD_PRELOAD=

TARGET = imu

SOURCE = $(TARGET).c 
#LIBS   = -liio -lpthread -lxml2 -lz -lrt
LIBS   =  -lpthread  -lz -lrt -lgcrypt -lgpg-error -lpcap  -lnl-3 -lnl-genl-3
export STAGING_DIR=/home/tycoon/openwrt/lede_vocore2/staging_dir
export LD_LIBRARY_PATH=
TOOLCHAIN_DIR=$(STAGING_DIR)/toolchain-mipsel_24kc_gcc-5.5.0_musl

INCLUDE_TARGET=$(STAGING_DIR)/target-mipsel_24kc_musl/usr/include
LIBRARY_TARGET=$(STAGING_DIR)/target-mipsel_24kc_musl/usr/lib
INCLUDE_TCHAIN=$(TOOLCHAIN_DIR)/include
LIBRARY_TCHAIN=$(TOOLCHAIN_DIR)/lib
INCLUDE_LIBNL=$(STAGING_DIR)/target-mipsel_24kc_musl/usr/include/libnl3
LIBRARY_USR=$(TOOLCHAIN_DIR)/usr/lib
CC=mipsel-openwrt-linux-gcc
#-std=c99
CFLAGS ?=  -O3 -Wall -I$(INCLUDE_TARGET) -I$(INCLUDE_TCHAIN) -I $(INCLUDE_LIBNL) -L$(LIBRARY_USR) -L$(LIBRARY_TCHAIN) -L$(LIBRARY_TARGET) 

all:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o deltatime.o -c deltatime.c $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o gy953.o -c gy953.c $(LIBS) 
	$(CC) $(CFLAGS) $(CPPFLAGS) -o pid.o -c pid.c $(LIBS) 
	$(CC) $(CFLAGS) $(CPPFLAGS) -o pwm.o -c pwm.c $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o adc.o -c adc.c $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o udpserver.o -c udpserver.c   $(LIBS) 		
	$(CC) $(CFLAGS) $(CPPFLAGS) -o 80211beacons.o -c 80211beacons.c   $(LIBS) 
	
	$(CC) $(CFLAGS) $(CPPFLAGS) -o imu.o -c $(SOURCE)   $(LIBS) 
	
	$(CC) $(CFLAGS) $(CPPFLAGS) -o imu imu.o gy953.o pid.o deltatime.o pwm.o adc.o udpserver.o 80211beacons.o  $(LIBS) 

#debug:
#	$(CC) $(CFLAGS) $(CPPFLAGS) -DDEBUG -o $(TARGET) $(SOURCE) $(LIBS) 



clean:
	rm -f $(TARGET)
