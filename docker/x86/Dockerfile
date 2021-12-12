FROM alpine:latest as builder

ARG AMULE_VERSION=2.3.2
ENV AMULE_VERSION ${AMULE_VERSION}
ENV UPNP_VERSION 1.6.22
ENV CRYPTOPP_VERSION CRYPTOPP_5_6_5
ENV BOOST_VERSION=1.76.0
ENV BOOST_VERSION_=1_76_0
ENV BOOST_ROOT=/usr/include/boost

WORKDIR /tmp

RUN echo ${AMULE_VERSION}

# Upgrade required packages (build)
RUN apk --update add gd geoip libpng libwebp pwgen sudo wxgtk zlib bash && \
    apk --update add --virtual build-dependencies alpine-sdk automake \
                               autoconf bison g++ gcc gd-dev geoip-dev \
                               gettext gettext-dev git libpng-dev libwebp-dev \
                               libtool libsm-dev make musl-dev wget \
                               wxgtk-dev zlib-dev

# Get boost headers
RUN mkdir -p ${BOOST_ROOT} \
    && wget "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_}.tar.gz" \
    && tar zxf boost_${BOOST_VERSION_}.tar.gz -C ${BOOST_ROOT} --strip-components=1

# Build libupnp
RUN mkdir -p /build \
    && wget "http://downloads.sourceforge.net/sourceforge/pupnp/libupnp-${UPNP_VERSION}.tar.bz2" \
    && tar xfj libupnp*.tar.bz2 \
    && cd libupnp* \
    && ./configure --prefix=/usr >/dev/null \
    && make -j$(nproc) >/dev/null \
    && make install \
    && make DESTDIR=/build install

# Build crypto++
RUN mkdir -p /build \
    && git clone --branch ${CRYPTOPP_VERSION} --single-branch "https://github.com/weidai11/cryptopp"  \
    && cd cryptopp* \
    && make CXXFLAGS="${CXXFLAGS} -DNDEBUG -fPIC" -j$(nproc) -f GNUmakefile dynamic >/dev/null \
    && make PREFIX="/usr" install \
    && make DESTDIR=/build PREFIX="/usr" install

# Build amule from source
RUN mkdir -p /build \
    && git clone --branch ${AMULE_VERSION} --single-branch "https://github.com/amule-project/amule" \
    && cd amule* \
    && ./autogen.sh >/dev/null \
    && ./configure \
        --disable-gui \
        --disable-amule-gui \
        --disable-wxcas \
        --disable-alc \
        --disable-plasmamule \
        --disable-kde-in-home \
        --prefix=/usr \
        --mandir=/usr/share/man \
        --enable-unicode \
        --without-subdirs \
        --without-expat \
        --enable-amule-daemon \
        --enable-amulecmd \
        --enable-webserver \
        --enable-cas \
        --enable-alcc \
        --enable-fileview \
        --enable-geoip \
        --enable-mmap \
        --enable-optimize \
        --enable-upnp \
        --disable-debug \
        --with-boost=${BOOST_ROOT} \
        >/dev/null  \
    && make -j$(nproc) >/dev/null \
    && make DESTDIR=/build install

# Install a nicer web ui
RUN cd /build/usr/share/amule/webserver \
    && git clone https://github.com/MatteoRagni/AmuleWebUI-Reloaded \
    && rm -rf AmuleWebUI-Reloaded/.git AmuleWebUI-Reloaded/doc-images

#################################

FROM alpine:latest

COPY --from=builder /build /

# Add startup script
ADD amule.sh /home/amule/amule.sh

# Final cleanup & upgrade required packages (run)
RUN chmod a+x /home/amule/amule.sh \
    && apk --update --no-cache add \
          sudo bash musl gd geoip wxgtk \
          gettext libpng libwebp pwgen zlib \
    && rm -rf /var/cache/apk/*

EXPOSE 4711/tcp 4712/tcp 4672/udp 4665/udp 4662/tcp 4661/tcp

ENTRYPOINT ["/home/amule/amule.sh"]
