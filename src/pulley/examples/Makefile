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
	-cat json-follow1 | $(CMD)

follow-all:
	-cat json-follow0 | $(CMD)

follow-people:
	-cat json-follow2 | $(CMD)

unfollow:
	-cat json-unfollow | $(CMD)

dump:
	-cat json-dump | $(CMD)

### Test edge cases (not actual commands)
#
#
empty:
	-echo "" | $(CMD)

bogus:
	-echo '{"verb":"flabberghast"}' | $(CMD)

broken:
	-echo '{{"' | $(CMD)

### Larger-scale tests
#
# 
# This sets up a pulley following a particular subtree,
# and in parallel runs two updates via the crank (which,
# in the examples provided, connects to the same LDAP
# server which is upstream to the pulley). This should
# show two updates showing up in the pulley after the
# initial sync.
#
sync-update:
	$(MAKE) start follow & ( cd ../../crank/examples && $(MAKE) start auth && sleep 1 && $(MAKE) update0 && sleep 1 && $(MAKE) update1 stop )


update0 update1:
	( cd ../../crank/examples && $(MAKE) $@ )

persons0 persons1 persons2:
	( cd ../../crank/examples && $(MAKE) ldap-update-$@ )


### Convenience targets
#
#
build:
	( cd ../../../build && $(MAKE) )
