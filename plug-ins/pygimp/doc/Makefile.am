
noinst_DATA = pygimp.html

EXTRA_DIST = pygimp.sgml
#	pygimp.html \
#	structure-of-plugin.html \
#	procedural-database.html \
#	gimp-module-procedures.html \
#	gimp-objects.html \
#	support-modules.html \
#	end-note.html

printed: pygimp.ps pygimp.pdf

clean:
	rm -f *.html pygimp.ps pygimp.pdf


pygimp.ps: pygimp.sgml
	(db2ps $(srcdir)/pygimp.sgml || docbook2ps $(srcdir)/pygimp.sgml)

pygimp.pdf: pygimp.sgml
	(db2pdf $(srcdir)/pygimp.sgml || docbook2pdf $(srcdir)/pygimp.sgml)

pygimp.html: pygimp.sgml
	-cd $(srcdir) && (db2html pygimp.sgml || docbook2html pygimp.sgml)
	-cd $(srcdir) && mv pygimp/*.html .
	-rm -rf $(srcdir)/pygimp

.PHONY: printed clean

