.PHONY : help doc doc-internal doc-public
help :
	@echo This directory does not have any build products.
	@echo However, you can build some documentation with “make doc”.

doc : doc-internal doc-public

doc-internal :
	doxygen Doxyfile.internal

doc-public :
	doxygen Doxyfile.public
