RONN=	/usr/local/bin/ronn

HTML_DIR=	./html.d
MAN_DIR=	./man.d/man8

SRC=	crank.md \
	pulley.md

docs: html man

# Copy to build-dir and then remove the sources,
# since ronn doesn't understand output paths.
html: html-dir $(SRC)
	cp $(SRC) $(HTML_DIR)
	( cd $(HTML_DIR) && $(RONN) --html $(SRC) && rm $(SRC) )

html-dir:
	test -d $(HTML_DIR) || mkdir $(HTML_DIR)

man: man-dir $(SRC)
	cp $(SRC) $(MAN_DIR)
	( cd $(MAN_DIR) && $(RONN) --roff --manual="SteamWorks" $(SRC) && rm $(SRC) )
	( cd $(MAN_DIR) && for f in $(SRC) ; do F=`basename $$f .md` ; mv $$F $$F.8 ; done )

man-dir:
	test -d $(MAN_DIR) || mkdir -p $(MAN_DIR)
