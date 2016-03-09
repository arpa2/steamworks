SERVER_NAME = bogus.bionicmutton.org
CONTENT_LENGTH = 18  # Irrelevant with JSON-parsing
BIN_DIR = ../../../build/shaft
BIN_NAME = shaft
CGI_SOCKET = /tmp/shaft

check-bin:
	@test -d $(BIN_DIR) && test -x $(BIN_DIR)/$(BIN_NAME)

CMD = SERVER_NAME=$(SERVER_NAME) CONTENT_LENGTH=$(CONTENT_LENGTH) cgi-fcgi -connect $(CGI_SOCKET) $(BIN_DIR)/$(BIN_NAME)

start:
	-cat json-connect | $(CMD)

stop:
	-cat json-stop | $(CMD)

serverinfo:
	-cat json-serverinfo | $(CMD)

upstream:
	-cat json-upstream | $(CMD)

bogus:
	-echo "" | $(CMD)
