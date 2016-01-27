SERVER_NAME = bogus.bionicmutton.org
CONTENT_LENGTH = 18  # Irrelevant with JSON-parsing
BIN_DIR = ../../../build/crank
BIN_NAME = crank
CGI_SOCKET = /tmp/crank

check-bin:
	@test -d $(BIN_DIR) && test -x $(BIN_DIR)/$(BIN_NAME)

CMD = SERVER_NAME=$(SERVER_NAME) CONTENT_LENGTH=$(CONTENT_LENGTH) cgi-fcgi -connect $(CGI_SOCKET) $(BIN_DIR)/$(BIN_NAME)

start:
	-cat json-connect | $(CMD)

stop:
	-cat json-stop | $(CMD)

serverinfo:
	-cat json-serverinfo | $(CMD)

search:
	-cat json-search | $(CMD)

update:
	-cat json-update | $(CMD)

bogus:
	-echo "" | $(CMD)
