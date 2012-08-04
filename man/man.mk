EXTRA_DIST += \
	man/config.dtd.in \
	$(man1_MANS:.1=.xml) \
	$(man5_MANS:.5=.xml)

CLEANFILES += \
	man/config.dtd \
	$(man1_MANS) \
	$(man5_MANS)

man/%.1 man/%.5: man/%.xml man/config.dtd
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
		$(XSLTPROC) \
		-o $(dir $@) \
		$(AM_XSLTPROCFLAGS) \
		http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl \
		$<

man/config.dtd: man/config.dtd.in Makefile
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
		$(SED) \
		-e 's:[@]sysconfdir[@]:$(sysconfdir):g' \
		-e 's:[@]datadir[@]:$(datadir):g' \
		-e 's:[@]PACKAGE_NAME[@]:$(PACKAGE_NAME):g' \
		-e 's:[@]PACKAGE_VERSION[@]:$(PACKAGE_VERSION):g' \
		< $< > $@ || rm $@
