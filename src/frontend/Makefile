CGI_SOCKET_DIR?= /tmp/steamworks

demo-crank:
	-mkdir -p $(CGI_SOCKET_DIR)
	cp -R webroot/crank/ $(CGI_SOCKET_DIR)
	nginx -c  `pwd`/steamworks-crank.conf 
	( cd ../crank/examples ; $(MAKE) CGI_SOCKET_DIR=$(CGI_SOCKET_DIR) start auth )
