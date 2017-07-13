SERVER_NAME = bogus.bionicmutton.org
CONTENT_LENGTH = 18  # Irrelevant with JSON-parsing
BIN_DIR = ../../../build/src/crank
BIN_NAME = crank
CGI_SOCKET_DIR?= /tmp/steamworks
CGI_SOCKET = $(CGI_SOCKET_DIR)/crank

# Add -Z if the LDAP server requires TLS
LDAPFLAGS = -h db -D cn=Manager,dc=example,dc=com -w secret

check-bin:
	@test -d $(BIN_DIR) && test -x $(BIN_DIR)/$(BIN_NAME)
	@test -d $(CGI_SOCKET_DIR) || mkdir -p $(CGI_SOCKET_DIR)

CMD = SERVER_NAME=$(SERVER_NAME) CONTENT_LENGTH=$(CONTENT_LENGTH) cgi-fcgi -connect $(CGI_SOCKET) $(BIN_DIR)/$(BIN_NAME)

start:
	-cat json-connect | $(CMD)

auth:
	-cat json-connect-auth | $(CMD)

stop:
	-cat json-stop | $(CMD)

serverinfo:
	-cat json-serverinfo | $(CMD)

serverstatus:
	-cat json-serverstatus | $(CMD)

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

ldap-add-persons:
	ldapadd $(LDAPFLAGS) < ldif-add-persons

ldap-remove-persons:
	ldapmodify $(LDAPFLAGS) < ldif-remove-persons

ldap-update-persons0:
	ldapmodify $(LDAPFLAGS) < ldif-update-persons0

ldap-update-persons1:
	ldapmodify $(LDAPFLAGS) < ldif-update-persons1

ldap-update-persons2:
	ldapmodify $(LDAPFLAGS) < ldif-update-persons2

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
