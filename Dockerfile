FROM pritunl/archlinux

MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Container"

RUN \
    pacman -S --needed --noconfirm \
        aqbanking \
        autoconf \
        automake \
        gcc \
        intltool \
        make \
        mariadb-clients \
        && \
    paccache -r -k0 && \
    rm -rf /usr/share/man/* && \
    rm -rf /tmp/* && \
    rm -rf /var/tmp/*

RUN \
	mkdir /finance /finance/fntxt2sql

# download, compile and install pxlib, a paradox DB library
RUN \
	cd /finance && \
	curl -L http://downloads.sourceforge.net/sourceforge/pxlib/pxlib-0.6.6.tar.gz | tar xvz && \
	cd pxlib-0.6.6 && \
	touch config.rpath && \
	autoreconf && \
	./configure --prefix=/usr/local \
              --with-gsf \
              --disable-static && \
	make install

# copy, compile and install fntxt2sql	
COPY fntxt2sql/* /finance/fntxt2sql/
RUN \
	cd /finance/fntxt2sql && \ 
	make && \
	cp -a fntxt2sql /usr/local/bin/fntxt2sql

# copy and install additional scripts
COPY finance finance-entrypoint mail.sh /usr/local/bin/
RUN \
    chown root:root /usr/local/bin/* && \
    chmod 755 /usr/local/bin/*

ENTRYPOINT [ "/usr/local/bin/finance-entrypoint" ]

