FROM nafets227/archbuildpkg:latest AS aqbanking-builder
MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Build Container for aqbanking"

RUN \
	set -x && \
	pacman -Suy --needed --noconfirm

USER jenkins

# download, compile and install aqbanking and gwenhywfar git versions
# standard Arch Linux versions are too old for PSD2
RUN \
	set -x && \
	cd /home/jenkins && \
	/usr/bin/git clone https://aur.archlinux.org/gwenhywfar-git.git  && \
	\
	cd gwenhywfar-git && \
	makepkg --sync --install --rmdeps --clean --noconfirm

RUN \
	set -x && \
	cd /home/jenkins && \
	/usr/bin/git clone https://aur.archlinux.org/aqbanking-git.git && \
	cd aqbanking-git && \
	makepkg --sync --install --rmdeps --clean --noconfirm

##############################################################################
FROM nafets227/archbuildpkg:latest AS finance-builder
MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Build Container"

RUN \
	set -x && \
	pacman -Suy --needed --noconfirm \
		mariadb-clients

USER jenkins
# download, compile and install pxlib, a paradox DB library
RUN \
	set -x && \
	cd /home/jenkins && \
	curl -L http://downloads.sourceforge.net/sourceforge/pxlib/pxlib-0.6.6.tar.gz | tar xvz && \
	cd pxlib-0.6.6 && \
	touch config.rpath && \
	autoreconf && \
	./configure \
		--prefix=/usr/local \
		--exec-prefix=/home/jenkins/pxlib.install \
		--with-gsf \
		--disable-static && \
	make

USER root
RUN \
	set -x && \
	cd /home/jenkins/pxlib-0.6.6 && \
	sudo make install && \
	cp -ar /home/jenkins/pxlib.install/* /usr/local

USER jenkins

# copy, compile and install fntxt2sql
RUN \
	mkdir /home/jenkins/fntxt2sql

COPY fntxt2sql/* /home/jenkins/fntxt2sql/

RUN \
	cd /home/jenkins/fntxt2sql && \
	make

ARG DEBUG=0
RUN \
	if [ "$DEBUG" == "1" ] ; then \
		ls -lA \
			/home/jenkins \
			/home/jenkins/pxlib.install \
			/home/jenkins/fntxt2sql \
		; \
	fi

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

RUN \
	set -x && \
	useradd -d /finance -U finance && \
	echo /usr/local/lib >/etc/ld.so.conf.d/finance.conf

COPY --from=aqbanking-builder /home/jenkins/gwenhywfar-git/*.pkg.tar.* /
COPY --from=aqbanking-builder /home/jenkins/aqbanking-git/*.pkg.tar.* /
COPY --from=finance-builder /home/jenkins/pxlib.install /usr/local
COPY --from=finance-builder /home/jenkins/fntxt2sql/fntxt2sql /usr/local/bin

RUN \
	ldconfig && \
	pacman -U --needed --noconfirm /*.pkg.tar.*

# copy and install additional scripts
COPY finance-root-wrapper finance-entrypoint /usr/local/bin/
RUN \
	chown root:root /usr/local/bin/* && \
	chmod 755 /usr/local/bin/*

ENTRYPOINT [ "/usr/local/bin/finance-root-wrapper" ]

