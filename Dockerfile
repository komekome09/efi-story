FROM alpine:latest

ENV GNUEFI_VERSION 3.0.9

WORKDIR /var

RUN apk --update add --virtual build-dependences \
        build-base \
        musl-dev \
        wget \
        unzip \
        curl \
    && set -x \
    && curl -sSL "https://sourceforge.net/projects/gnu-efi/files/gnu-efi-${GNUEFI_VERSION}.tar.bz2/download" -o gnu-efi.tar.bz2 \
    && tar xvf gnu-efi.tar.bz2 \
    && ls \
    && cd gnu-efi-${GNUEFI_VERSION} \
    && make \
    && make install \
    && mkdir /var/gnuefi/ \
    && cd /var/gnuefi

WORKDIR /var/gnuefi
CMD ["make"]
