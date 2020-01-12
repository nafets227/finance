FROM nafets227/archbuildpkg:latest AS builder
MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Build Container"

RUN \
	set -x && \
	mkdir /finance && \
	chown jenkins /finance

RUN \
	set -x && \
	pacman -Suy --needed --noconfirm \
		mariadb-clients

USER jenkins
# download, compile and install pxlib, a paradox DB library
RUN \
	set -x && \
	cd /finance && \
	curl -L http://downloads.sourceforge.net/sourceforge/pxlib/pxlib-0.6.6.tar.gz | tar xvz && \
	cd pxlib-0.6.6 && \
	touch config.rpath && \
	autoreconf && \
	./configure \
		--prefix=/usr/local \
		--with-gsf \
		--disable-static && \
	make
USER root
RUN \
	set -x && \
	cd /finance/pxlib-0.6.6 && \
	sudo make install && \
	tar cv \
		--exclude /usr/local/include \
		-f /finance/pxlib-installed.tar \
		/usr/local

USER jenkins
# download, compile and install aqbanking and gwenhywfar git versions
# standard Arch Linux versions are too old for PSD2
RUN \
	set -x && \
	cd /finance && \
	/usr/bin/git clone https://aur.archlinux.org/gwenhywfar-git.git  && \
	\
	cd gwenhywfar-git && \
	makepkg --sync --install --rmdeps --clean --noconfirm

RUN \
	set -x && \
	cd /finance && \
	/usr/bin/git clone https://aur.archlinux.org/aqbanking-git.git && \
	cd aqbanking-git && \
	makepkg --sync --install --rmdeps --clean --noconfirm

# copy, compile and install fntxt2sql
RUN \
	mkdir /finance/fntxt2sql

COPY fntxt2sql/* /finance/fntxt2sql/

RUN \
	cd /finance/fntxt2sql && \
	make

RUN \
	if [ "$DEBUG" == "1" ] ; then \
		ls -lA \
			/finance \
			/finance/gwenhywfar-git \
			/finance/aqbanking-git \
			/finance/fntxt2sql \
		; \
	fi

##############################################################################
FROM archlinux/base

MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Container"

VOLUME /finance

RUN \
	set -x && \
	pacman -Suy --needed --noconfirm \
		bind-tools \
		gettext \
		grep \
		iputils \
		mariadb-clients \
		s-nail \
		sudo \
		tar \
		&& \
	if [ "$DEBUG" == "1" ] ; then \
		echo deleting files not needed: && \
		find \
			/var/lib/pacman \
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

RUN \
	set -x && \
	useradd -d /finance -U finance && \
	echo /usr/local/lib >/etc/ld.so.conf.d/finance.conf

COPY --from=builder /finance/pxlib-installed.tar /
COPY --from=builder /finance/gwenhywfar-git/*.pkg.tar.* /
COPY --from=builder /finance/aqbanking-git/*.pkg.tar.* /
COPY --from=builder /finance/fntxt2sql/fntxt2sql /usr/local/bin

RUN \
	tar xvf /pxlib-installed.tar && \
	ldconfig && \
	pacman -U --needed --noconfirm /*.pkg.tar.*

# copy and install additional scripts
COPY finance-root-wrapper finance-entrypoint /usr/local/bin/
RUN \
	chown root:root /usr/local/bin/* && \
	chmod 755 /usr/local/bin/*

ENTRYPOINT [ "/usr/local/bin/finance-root-wrapper" ]

