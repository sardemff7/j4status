EXTRA_DIST += \
	$(systemduserunit_DATA:=.in) \
	$(null)

CLEANFILES += \
	$(systemduserunit_DATA) \
	$(null)

$(systemduserunit_DATA): %: %.in src/config.h
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
		$(SED) \
		-e 's:[@]bindir[@]:$(bindir):g' \
		-e 's:[@]PACKAGE_NAME[@]:$(PACKAGE_NAME):g' \
		< $< > $@ || rm $@

