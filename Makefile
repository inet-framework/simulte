all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

makefiles:
	cd src && opp_makemake -f --deep --make-so -o simulte -O out -I$$\(INET_PROJ\)/src/networklayer/common -I$$\(INET_PROJ\)/src -I$$\(INET_PROJ\)/src/world/obstacles -I$$\(INET_PROJ\)/src/mobility/models -I$$\(INET_PROJ\)/src/networklayer/contract -I$$\(INET_PROJ\)/src/mobility -I$$\(INET_PROJ\)/src/util -I$$\(INET_PROJ\)/src/transport/contract -I$$\(INET_PROJ\)/src/linklayer/radio/propagation -I$$\(INET_PROJ\)/src/world/powercontrol -I$$\(INET_PROJ\)/src/linklayer/radio -I$$\(INET_PROJ\)/src/util/headerserializers/tcp -I$$\(INET_PROJ\)/src/networklayer/ipv4 -I$$\(INET_PROJ\)/src/util/headerserializers/ipv4 -I$$\(INET_PROJ\)/src/base -I$$\(INET_PROJ\)/src/util/headerserializers -I$$\(INET_PROJ\)/src/world/radio -I$$\(INET_PROJ\)/src/util/headerserializers/udp -I$$\(INET_PROJ\)/src/battery/models -I$$\(INET_PROJ\)/src/applications/pingapp -I$$\(INET_PROJ\)/src/linklayer/contract -I$$\(INET_PROJ\)/src/transport/tcp_common -I$$\(INET_PROJ\)/src/networklayer/arp -I$$\(INET_PROJ\)/src/transport/udp -I$$\(INET_PROJ\)/src/applications/udpapp -L$$\(INET_PROJ\)/out/$$\(CONFIGNAME\)/src -linet -DINET_IMPORT -KINET_PROJ=../../inet

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
