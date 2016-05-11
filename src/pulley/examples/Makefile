SERVER_NAME = bogus.bionicmutton.org
CONTENT_LENGTH = 18  # Irrelevant with JSON-parsing
BIN_DIR = ../../../build/pulley
BIN_NAME = pulley
CGI_SOCKET = /tmp/pulley

check-bin:
	@test -d $(BIN_DIR) && test -x $(BIN_DIR)/$(BIN_NAME)

CMD = SERVER_NAME=$(SERVER_NAME) CONTENT_LENGTH=$(CONTENT_LENGTH) cgi-fcgi -connect $(CGI_SOCKET) $(BIN_DIR)/$(BIN_NAME)

start:
	-cat json-connect | $(CMD)

stop:
	-cat json-stop | $(CMD)

serverinfo:
	-cat json-serverinfo | $(CMD)


follow:
	-cat json-follow0 | $(CMD)

follow-all:
	-cat json-follow1 | $(CMD)

unfollow:
	-cat json-unfollow | $(CMD)

### Test edge cases (not actual commands)
#
#
empty:
	-echo "" | $(CMD)

bogus:
	-echo '{"verb":"flabberghast"}' | $(CMD)

broken:
	-echo '{{"' | $(CMD)



### Convenience targets
#
#
build:
	( cd ../../../build && $(MAKE) )
