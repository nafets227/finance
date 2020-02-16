FROM gcc:latest AS builder
MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Build Container for aqbanking"

# install prerequisited
RUN \
	set -x && \
	apt-get update && \
	apt-get install -y --no-install-recommends \
		intltool \
		libgcrypt20-dev \
		libgnutls28-dev \
		libxmlsec1-dev

# gwenhywfar
RUN \
	set -x && \
	git clone https://github.com/aqbanking/gwenhywfar && \
	cd gwenhywfar && \
	git checkout tags/5.2.0 && \
	make -f Makefile.cvs && \
	./configure \
		--with-guis="cpp" \
		--enable-error-on-warning \
		--disable-network-checks && \
	make && \
	make install && \
	make DESTDIR=$PWD/dist install

# aqbanking
RUN \
	export LD_LIBRARY_PATH=/usr/local/lib && \
	set -x && \
	git clone https://github.com/aqbanking/aqbanking && \
	cd aqbanking && \
	git checkout tags/6.1.0 && \
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
	curl -L http://downloads.sourceforge.net/sourceforge/pxlib/pxlib-0.6.8.tar.gz | tar xvz && \
	cd pxlib-0.6.8 && \
	touch config.rpath && \
	autoreconf && \
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
FROM archlinux/base

MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Container"

VOLUME /finance

ARG DEBUG=0
RUN \
	set -x && \
	pacman -Suy --needed --noconfirm \
		bind-tools \
		gettext \
		grep \
		iputils \
		mariadb-clients \
		s-nail \
		xmlsec \
		&& \
	if [ "$DEBUG" == "1" ] ; then \
		echo deleting files not needed: && \
		find \
			/var/cache/pacman \
			/usr/share/man \
			/tmp \
			/var/tmp \
			-type f \
		; \
	fi && \
	rm -rf \
		/var/cache/pacman \
		/usr/share/man/* \
		/tmp/* \
		/var/tmp/*

COPY --from=builder /gwenhywfar/dist/usr/local /usr/local
COPY --from=builder /aqbanking/dist /
COPY --from=builder /pxlib-0.6.8/dist /
COPY --from=builder /fntxt2sql/dist /

# copy and install additional scripts
COPY finance-root-wrapper finance-entrypoint /usr/local/bin/

RUN \
	set -x && \
	useradd -d /finance -U finance && \
	echo /usr/local/lib >/etc/ld.so.conf.d/finance.conf && \
	ldconfig && \
	chown root:root /usr/local/bin/* && \
	chmod 755 /usr/local/bin/*

ENTRYPOINT [ "/usr/local/bin/finance-root-wrapper" ]
