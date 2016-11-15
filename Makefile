all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

makefiles:
	cd src && opp_makemake -f --deep --make-so -o lte -O out -I$$\(INET_PROJ\)/src -I$$\(INET_PROJ\)/src/inet/networklayer/contract/ipv4 -I$$\(INET_PROJ\)/src/inet/common -I$$\(INET_PROJ\)/src/inet/mobility/static -I$$\(INET_PROJ\)/src/inet/networklayer/ipv4 -I$$\(INET_PROJ\)/src/inet/common/geometry/common -I$$\(INET_PROJ\)/src/inet/mobility/contract -I$$\(INET_PROJ\)/src/inet/networklayer/common -I$$\(INET_PROJ\)/src/inet/transportlayer/contract/udp -I$$\(INET_PROJ\)/src/inet/common/queue -I$$\(INET_PROJ\)/src/inet/transportlayer/tcp_common -I$$\(INET_PROJ\)/src/inet/transportlayer/udp -I$$\(INET_PROJ\)/src/inet/networklayer/configurator/ipv4 -I$$\(INET_PROJ\)/src/inet/transportlayer/sctp -I$$\(INET_PROJ\)/src/inet/applications/sctpapp -I$$\(INET_PROJ\)/src/inet/transportlayer/contract/sctp -L../../inet/out/$$\(CONFIGNAME\)/src -lINET -DINET_IMPORT -KINET_PROJ=../../inet


checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
