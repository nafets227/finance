FROM alpine:3.21.3 AS builder
LABEL Description="Finance Build Container for aqbanking"

# install prerequisited
RUN \
	set -x && \
	apk add \
		autoconf \
		automake \
		bash \
		build-base \
		bzip2 \
		curl \
		gettext-dev \
		glib-dev \
		git \
		gmp-dev \
		gnutls-dev \
		intltool \
		libffi-dev \
		libgcrypt-dev \
		libtool \
		libxslt-dev \
		libxml2-dev \
		mysql-dev \
		xmlsec-dev
	#----- end for alpine

# gwenhywfar
RUN \
	set -x && \
	git clone https://git.aquamaniac.de/git/gwenhywfar.git && \
	cd gwenhywfar && \
	git checkout tags/5.12.0 && \
	sed -i 's:i18n_libs="$LIBS":i18n_libs="$LIBS -lintl":' configure.ac && \
	make -f Makefile.cvs && \
	./configure \
		CFLAGS=-Wno-error=deprecated-declarations \
		--with-guis="cpp" \
		--enable-error-on-warning \
		--disable-network-checks && \
	make && \
	make install && \
	make DESTDIR=$PWD/dist install

# aqbanking
RUN \
	set -x && \
	git clone https://git.aquamaniac.de/git/aqbanking.git && \
	cd aqbanking && \
	git checkout tags/6.6.0 && \
	sed -i 's:i18n_libs="$LIBS":i18n_libs="$LIBS -lintl":' configure.ac && \
	ACLOCAL_FLAGS="-I /usr/local/share/aclocal" make -f Makefile.cvs && \
	./configure && \
	make typedefs && \
	make typefiles && \
	make && \
	make install && \
	make DESTDIR=$PWD/dist install

# pxlib, a paradox DB library
RUN \
	set -x && \
	git clone https://github.com/steinm/pxlib.git && \
	cd pxlib && \
	touch config.rpath && \
	git checkout 781a234 && \
	rm -rf debian && \
	sed -i 's:automake-1.16:automake-1.17:' autogen.sh && \
	./autogen.sh && \
	#autoupdate && \
	#autoconf && \
	# autoreconf && \
	./configure \
		--prefix=/usr/local \
		--with-gsf \
		--disable-static && \
	make && \
	make install && \
	make DESTDIR=$PWD/dist install

# fntxt2sql
RUN \
	mkdir /fntxt2sql
COPY fntxt2sql/* /fntxt2sql/
RUN \
	set -x && \
	cd fntxt2sql && \
	make clean && \
	make && \
	mkdir -p dist/usr/local/bin && \
	cp -a fntxt2sql  dist/usr/local/bin/

##############################################################################
FROM alpine:3.21.3

LABEL org.opencontainers.image.authors="Stefan Schallenberg aka nafets227 <infos@nafets.de>"
LABEL Description="Finance Container"

VOLUME /finance

ARG DEBUG=0
RUN \
	set -x && \
	apk add --no-cache --update \
		bash \
		ca-certificates \
		findutils \
		gettext \
		grep \
		iputils \
		mariadb-client \
		mariadb-connector-c \
		s-nail \
		xmlsec \
		gmp \
		gnutls \
		bind-tools \
		&& \
	if [ "$DEBUG" == "1" ] ; then \
		echo deleting files not needed: && \
		find \
			/var/cache/apk \
			/usr/share/man \
			/tmp \
			/var/tmp \
			-type f \
		; \
	fi && \
	rm -rf \
		/var/cache/apk/* \
		/usr/share/man/* \
		/tmp/* \
		/var/tmp/*

COPY --from=builder /gwenhywfar/dist/usr/local /usr/local
COPY --from=builder /aqbanking/dist /
COPY --from=builder /pxlib/dist /
COPY --from=builder /fntxt2sql/dist /

# copy and install additional scripts
COPY finance-root-wrapper finance-entrypoint /usr/local/bin/

RUN \
	set -x && \
	adduser -D -h /finance finance finance && \
	chown root:root /usr/local/bin/* && \
	chmod 755 /usr/local/bin/*

ENTRYPOINT [ "/usr/local/bin/finance-root-wrapper" ]
