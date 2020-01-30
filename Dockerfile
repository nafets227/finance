FROM alpine:edge AS builder
MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Build Container for aqbanking"

RUN \
	apk add --no-cache --update \
		aqbanking-dev \
		gwenhywfar-dev \
		mysql-dev \
		build-base

# fntxt2sql
RUN \
	mkdir /fntxt2sql
COPY fntxt2sql/* /fntxt2sql/
RUN \
	cd fntxt2sql && \
	make clean && \
	make CONF_HBCIPX=0 && \
	mkdir -p dist/usr/local/bin && \
	cp -a fntxt2sql  dist/usr/local/bin/

# list results
RUN \
	echo "Results in Builder container:" && \
	ls -lR /*/dist

##############################################################################
FROM alpine:edge

MAINTAINER Stefan Schallenberg aka nafets227 <infos@nafets.de>
LABEL Description="Finance Container"

VOLUME /finance

ARG DEBUG=0
RUN \
	set -x && \
	apk add --no-cache --update \
		bash \
		aqbanking \
		findutils \
		gettext \
		grep \
		iputils \
		mariadb-client \
		mariadb-connector-c \
		s-nail \
		xmlsec \
		&& \
	if [ "$DEBUG" == "1" ] ; then \
		echo deleting files not needed: && \
		find \
			/usr/share/man \
			/tmp \
			/var/tmp \
			-type f \
		; \
	fi && \
	rm -rf \
		/usr/share/man/* \
		/tmp/* \
		/var/tmp/*

COPY --from=builder /fntxt2sql/dist /

# copy and install additional scripts
COPY finance-root-wrapper finance-entrypoint /usr/local/bin/

	# echo /usr/local/lib >/etc/ld.so.conf.d/finance.conf && \
	# ldconfig && \
RUN \
	set -x && \
	adduser -D -h /finance finance finance && \
	chown root:root /usr/local/bin/* && \
	chmod 755 /usr/local/bin/*

# list results
RUN \
	echo "Installed following files to /usr/local: " && \
	ls -lR /usr/local

ENTRYPOINT [ "/usr/local/bin/finance-root-wrapper" ]
