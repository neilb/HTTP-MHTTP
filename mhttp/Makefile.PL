use ExtUtils::MakeMaker;
#$Verbose = 1;
WriteMakefile(
     NAME   => 'libmhttp',
     SKIP   => [qw(all static static_lib dynamic dynamic_lib)],
     clean  => {'FILES' => 'libmhttp$(LIBEEXT)'},
             );

sub MY::top_targets {
       '
all :: static

pure_all :: static

static ::       libmhttp$(LIB_EXT)

libmhttp$(LIB_EXT): $(O_FILES)
	$(AR) cr libmhttp$(LIB_EXT) $(O_FILES)
	$(RANLIB) libmhttp$(LIB_EXT)

          ';
}
