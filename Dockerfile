FROM archlinux/base

MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Container"

VOLUME /finance

RUN \
	set -x && \
	pacman -Sy --noconfirm && \
	pacman -S --needed --noconfirm libarchive && \
	pacman -S --needed --noconfirm archlinux-keyring && \
	pacman -Su --noconfirm && \
	pacman -S --needed --noconfirm \
		bind-tools \
		gettext \
		grep \
		iputils \
		mariadb-clients \
		s-nail \
		sudo \
		&& \
	if [ "$DEBUG" == "1" ] ; then \
		echo deleting files not needed: && \
		find \
			/var/lib/pacman \
			/var/cache/pacman \
			/usr/share/man \
			/tmp \
			/var/tmp \
			-type f ; \
	fi && \
	rm -rf \
		/var/cache/pacman \
		/usr/share/man/* \
		/tmp/* \
		/var/tmp/*

RUN \
	echo /usr/local/lib >/etc/ld.so.conf.d/finance.conf
	
RUN \
	set -x && \
	useradd -d /finance -U finance && \
	useradd -d /var/tmp/aur -U aur && \
	echo "aur ALL = NOPASSWD: /usr/bin/pacman" \
		>/etc/sudoers.d/nafetsde-aur

# download, compile and install pxlib, a paradox DB library
RUN \
	set -x && \
	pacman -Sy --noconfirm && \
	pacman -S --needed --noconfirm \
		autoconf \
		automake \
		gcc \
		intltool \
		make \
		&& \
	cd /finance && \
	curl -L http://downloads.sourceforge.net/sourceforge/pxlib/pxlib-0.6.6.tar.gz | tar xvz && \
	cd pxlib-0.6.6 && \
	touch config.rpath && \
	autoreconf && \
	./configure \
		--prefix=/usr/local \
		--with-gsf \
		--disable-static && \
	make install && \
	ldconfig && \
	cd /finance && \
	rm -rf /finance/pxlib-0.6.6 && \
	pacman -R --nosave --recursive --noconfirm \
		autoconf \
		automake \
		gcc \
		make && \
	rm -rf \
		/var/cache/pacman \
		/usr/share/man/* \
		/tmp/* \
		/var/tmp/*

# download, compile and install aqbanking and gwenhywfar git versions
# standard Arch Linux versions are too old for PSD2
USER aur
RUN \
	set -x && \
	sudo pacman -Sy --noconfirm && \
	sudo pacman -S --needed --noconfirm base-devel git && \
	/usr/bin/git clone https://aur.archlinux.org/gwenhywfar-git.git /var/tmp/aur/gwenhywfar-git && \
	cd /var/tmp/aur/gwenhywfar-git && makepkg --sync --install --rmdeps --clean --noconfirm && \
	/usr/bin/git clone https://aur.archlinux.org/aqbanking-git.git /var/tmp/aur/aqbanking-git && \
	cd /var/tmp/aur/aqbanking-git && makepkg --sync --install --rmdeps --clean --noconfirm
	# sudo pacman -R --nosave --recursive --noconfirm base-devel git --ignore findutils --ignore sudo && \
USER root

# copy, compile and install fntxt2sql	
RUN \
	mkdir /tmp/fntxt2sql

COPY fntxt2sql/* /tmp/fntxt2sql/

RUN \
	cd /tmp/fntxt2sql && \ 
	make && \
	cp -a fntxt2sql /usr/local/bin/fntxt2sql && \ 
	rm -rf /tmp/fntxt2sql

# copy and install additional scripts
COPY finance-root-wrapper finance-entrypoint /usr/local/bin/
RUN \
	chown root:root /usr/local/bin/* && \
	chmod 755 /usr/local/bin/*

ENTRYPOINT [ "/usr/local/bin/finance-root-wrapper" ]

