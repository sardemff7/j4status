EXTRA_DIST += \
	src/config.ent.in \
	$(man1_MANS:.1=.xml) \
	$(man5_MANS:.5=.xml) \
	$(null)

CLEANFILES += \
	src/config.ent \
	$(man1_MANS) \
	$(man5_MANS) \
	$(null)

MAN_GEN_RULE = $(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
	$(XSLTPROC) \
	-o $(dir $@) \
	$(AM_XSLTPROCFLAGS) \
	--stringparam profile.condition '${AM_DOCBOOK_CONDITIONS}' \
	$(XSLTPROCFLAGS) \
	http://docbook.sourceforge.net/release/xsl/current/manpages/profile-docbook.xsl \
	$<

$(man1_MANS): %.1: %.xml $(NKUTILS_MANFILES) src/config.ent
	$(MAN_GEN_RULE)

$(man5_MANS): %.5: %.xml $(NKUTILS_MANFILES) src/config.ent
	$(MAN_GEN_RULE)

src/config.ent: src/config.ent.in Makefile
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
		$(SED) \
		-e 's:[@]sysconfdir[@]:$(sysconfdir):g' \
		-e 's:[@]datadir[@]:$(datadir):g' \
		-e 's:[@]PACKAGE_NAME[@]:$(PACKAGE_NAME):g' \
		-e 's:[@]PACKAGE_VERSION[@]:$(PACKAGE_VERSION):g' \
		< $< > $@ || rm $@
