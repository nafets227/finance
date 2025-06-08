FROM alpine:3.22.0 AS builder
LABEL description="Finance Build Container for aqbanking"

# install prerequisited
# hadolint ignore=DL3018
RUN \
	set -x && \
	apk add --no-cache \
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

SHELL [ "/bin/bash", "-c" ]

# gwenhywfar
WORKDIR /gwenhywfar
RUN \
	set -x && \
	git clone https://git.aquamaniac.de/git/gwenhywfar.git . && \
	git checkout tags/5.12.0 && \
	sed -i "s:i18n_libs=\"\$LTLIBINTL\":i18n_libs=\"\$LTLIBINTL -lintl\":" \
		configure.ac && \
	sed -i "s:AM_GNU_GETTEXT:AM_GLIB_GNU_GETTEXT:" configure.ac && \
	make -f Makefile.cvs && \
	./configure \
		CFLAGS=-Wno-error=deprecated-declarations \
		--with-guis="cpp" \
		--enable-error-on-warning \
		--disable-network-checks && \
	make && \
	make install && \
	make DESTDIR="$PWD"/dist install

# aqbanking
WORKDIR /aqbanking
RUN \
	set -x && \
	git clone https://git.aquamaniac.de/git/aqbanking.git . && \
	git checkout tags/6.6.0 && \
	sed -i "s:i18n_libs=\"\$LIBS\":i18n_libs=\"\$LIBS -lintl\":" configure.ac && \
	ACLOCAL_FLAGS="-I /usr/local/share/aclocal" make -f Makefile.cvs && \
	./configure && \
	make typedefs && \
	make typefiles && \
	make && \
	make install && \
	make DESTDIR="$PWD"/dist install

# pxlib, a paradox DB library
WORKDIR /pxlib
RUN \
	set -x && \
	git clone https://github.com/steinm/pxlib.git . && \
	touch config.rpath && \
	git checkout 781a234 && \
	rm -rf debian && \
	sed -i 's:automake-1.16:automake-1.17:' autogen.sh && \
	sed -i 's:aclocal:aclocal --aclocal-path /usr/share/gettext/m4:' \
		autogen.sh && \
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
	make DESTDIR="$PWD"/dist install

# fntxt2sql
WORKDIR /fntxt2sql
COPY fntxt2sql/* /fntxt2sql/
RUN \
	set -x && \
	make clean && \
	make && \
	mkdir -p dist/usr/local/bin && \
	cp -a fntxt2sql  dist/usr/local/bin/

##############################################################################
FROM alpine:3.22.0

LABEL org.opencontainers.image.authors="Stefan Schallenberg aka \
	nafets227 <infos@nafets.de>"
LABEL description="Finance Container"

VOLUME /finance

ARG DEBUG=0
# hadolint ignore=DL3018
RUN \
	set -x && \
	apk add --no-cache --update \
		bash \
		ca-certificates \
		findutils \
		gettext \
		grep \
		iputils \
		libgcrypt \
		mariadb-client \
		mariadb-connector-c \
		s-nail \
		xmlsec \
		gmp \
		gnutls \
		bind-tools \
		&& \
	if [ "$DEBUG" = "1" ] ; then \
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

SHELL [ "/bin/bash", "-c" ]

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

#checkov:skip=CKV_DOCKER_3:root user needed here.
