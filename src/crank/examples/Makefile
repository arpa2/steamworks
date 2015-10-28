SERVER_NAME = bogus.bionicmutton.org
CONTENT_LENGTH = 18  # Irrelevant with JSON-parsing
BIN_DIR = ../../../build/crank
CGI_SOCKET = /tmp/crank

check-bin:
	@test -d $(BIN_DIR) && test -x $(BIN_DIR)/crank

CMD = SERVER_NAME=$(SERVER_NAME) CONTENT_LENGTH=$(CONTENT_LENGTH) cgi-fcgi -connect $(CGI_SOCKET) $(BIN_DIR)/crank

start:
	-cat json-connect | $(CMD)

stop:
	-cat json-stop | $(CMD)

search:
	-cat json-search | $(CMD)

bogus:
	-echo "" | $(CMD)
