use ExtUtils::MakeMaker;

WriteMakefile(
    clean => { 'FILES'	=> '*~ *.inl core test*.txt *.tar.gz _Inline ' },
    dist => { 'TARFLAGS'	=> 'cvf',
              'COMPRESS'        => 'gzip -9f',
    		'SUFFIX'	=> '.tgz'},
    'NAME'	=> 'HTTP::MHTTP',
    'VERSION_FROM' => 'MHTTP.pm',
    'MYEXTLIB' => 'mhttp/libmhttp$(LIB_EXT)',
    'DEFINE'	=> "",
    'INC'	=> "-Imhttp",
);

sub MY::postamble {
'
$(MYEXTLIB): mhttp/Makefile
	cd mhttp && $(MAKE) $(PASSTHRU)
';
}

