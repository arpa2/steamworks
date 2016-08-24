CGI_SOCKET_DIR?= /tmp/steamworks
LOG_DIR?= /tmp/steamworks/logs  # Also in the nginx config

deploy: setup-dir-crank setup-app-crank

demo-crank: setup-dir-crank setup-nginx-crank setup-fcgi-crank setup-app-crank

setup-dir-crank:
	-mkdir -p $(CGI_SOCKET_DIR) $(LOG_DIR)

setup-app-crank: setup-app-common
	cp -R webroot/crank/* $(CGI_SOCKET_DIR)

setup-nginx-crank:
	nginx -c  `pwd`/steamworks-crank.conf 

setup-fcgi-crank:
	( cd ../crank/examples ; $(MAKE) CGI_SOCKET_DIR=$(CGI_SOCKET_DIR) serverstatus )

setup-app-common:
	cp -R webroot/common/* $(CGI_SOCKET_DIR)
