MAKEFLGS = --no-print-directory
SRC=src

all : 
	@echo enter platform type, eg.
	@echo make djgpp             -- for djgpp dos
	@echo make unix              -- for unix (linux etc)

djgpp :
	make $(MAKEFLGS) build EXTENSION=".exe" 

unix :
	make $(MAKEFLGS) build EXTENSION=

# defines 
linux : unix
dos : djgpp

#############################################
# build

build :
	make $(MAKEFLGS) -C $(SRC)/swantbls EXE=../../swantbls$(EXTENSION)
	make $(MAKEFLGS) -C $(SRC)/add_fs EXE=../../add_fs$(EXTENSION)
