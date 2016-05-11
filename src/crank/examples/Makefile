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

auth:
	-cat json-connect-auth | $(CMD)

stop:
	-cat json-stop | $(CMD)

serverinfo:
	-cat json-serverinfo | $(CMD)

search:
	-cat json-search | $(CMD)

search-all:
	-cat json-search-all | $(CMD)

typeinfo:
	-cat json-typeinfo | $(CMD)

update: update0 update1

update0:
	-cat json-update0 | $(CMD)

update1:
	-cat json-update1 | $(CMD)

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
