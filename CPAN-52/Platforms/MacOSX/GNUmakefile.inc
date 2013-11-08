DEFAULT := $(shell sed -n '/^DEFAULT = /s///p' $(PERLVERSIONS))
KNOWNVERSIONS := $(shell grep -v '^DEFAULT' $(PERLVERSIONS))
BOOTSTRAPPERL =

MY_ARCHS := $(or $(RC_ARCHS),$(MY_HOST))
MY_PREFER_32_BIT := yes

EXTRAMERGE := mergeoss mergeman
EXTRAVERSIONERFLAGS = -DFORCE_TWO_NUMBER_VERSIONS