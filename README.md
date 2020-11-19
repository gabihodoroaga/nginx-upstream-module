# nginx-upstream-module

This is the a startup project for nginx custom upstream module and is part of the [Nginx - Custom upstream module](https://www.hodo.dev/posts/post-17-nginx-custom-upstream-module/) tutorial. See the tutorial for details.

## Build and run

Download all the required source files

```bash
# Clone the repositoty
git clone https://github.com/gabihodoroaga/nginx-upstream-module.git

# download nginx
curl -OL http://nginx.org/download/nginx-1.19.3.tar.gz
tar -xvzf nginx-1.19.3.tar.gz && rm nginx-1.19.3.tar.gz

# download PCRE library
curl -OL https://ftp.pcre.org/pub/pcre/pcre-8.44.tar.gz
tar -xvzf pcre-8.44.tar.gz && rm pcre-8.44.tar.gz

# download OpenSSL
curl -OL https://www.openssl.org/source/openssl-1.1.1h.tar.gz
tar -xvzf openssl-1.1.1h.tar.gz && rm openssl-1.1.1h.tar.gz 

# download zlib
curl -OL https://zlib.net/zlib-1.2.11.tar.gz
tar -xvzf zlib-1.2.11.tar.gz && rm zlib-1.2.11.tar.gz

```

Configure the nginx build

```bash
cd nginx-1.19.3/

./configure --with-debug \
            --prefix= \
            --conf-path=conf/nginx.conf \
            --pid-path=logs/nginx.pid \
            --http-log-path=logs/access.log \
            --error-log-path=logs/error.log \
            --http-client-body-temp-path=temp/client_body_temp \
            --http-proxy-temp-path=temp/proxy_temp \
            --http-fastcgi-temp-path=temp/fastcgi_temp \
            --http-scgi-temp-path=temp/scgi_temp \
            --http-uwsgi-temp-path=temp/uwsgi_temp \
            --with-pcre=../pcre-8.44 \
            --with-zlib=../zlib-1.2.11 \
            --with-http_v2_module \
            --with-http_realip_module \
            --with-http_addition_module \
            --with-http_sub_module \
            --with-http_dav_module \
            --with-http_stub_status_module \
            --with-http_flv_module \
            --with-http_mp4_module \
            --with-http_gunzip_module \
            --with-http_gzip_static_module \
            --with-http_auth_request_module \
            --with-http_random_index_module \
            --with-http_secure_link_module \
            --with-http_slice_module \
            --with-mail \
            --with-stream \
            --with-openssl=../openssl-1.1.1h \
            --with-http_ssl_module \
            --with-mail_ssl_module \
            --with-stream_ssl_module \
            --add-module=../nginx-upstream-module
```

Build nginx with the custom module

```bash
cd nginx-1.19.3/
make
```

Create your test folder 

```bash
# create the test folder 
mkdir nginx-test
mkdir nginx-test/logs
mkdir nginx-test/conf
mkdir nginx-test/temp
mkdir nginx-test/run
# copy the nginx file
cp nginx-1.19.3/objs/nginx nginx-test/
# copy the nginx sample configuration file
cp nginx-upstream-module/example/nginx.conf nginx-test/conf/
```

Run your custom nginx build

```bash
cd nginx-test
./nginx
```
