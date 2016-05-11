RONN=	/usr/local/bin/ronn

HTML_DIR=	./html
MAN_DIR=	./man

SRC=	crank.md \
	pulley.md

docs: html

# Copy to build-dir and then remove the sources,
# since ronn doesn't understand output paths.
html: html-dir $(SRC)
	cp $(SRC) $(HTML_DIR)
	( cd $(HTML_DIR) && $(RONN) --html $(SRC) && rm $(SRC) )

html-dir:
	test -d $(HTML_DIR) || mkdir $(HTML_DIR)

