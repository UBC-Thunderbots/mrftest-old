	; asmsyntax=pic

ifdef EXTERNDEF
#undefine EXTERNDEF
endif

ifdef IMPL
#define EXTERNDEF global
else
#define EXTERNDEF extern
endif

