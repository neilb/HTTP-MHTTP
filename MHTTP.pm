package HTTP::MHTTP;
use strict;
require 5.005;
use Carp;
require DynaLoader;
require Exporter;
use MIME::Base64 qw(encode_base64);
use vars qw(@ISA $VERSION @EXPORT_OK);
$VERSION = '0.07';
@ISA = qw(DynaLoader Exporter);


sub dl_load_flags { 0x01 }
HTTP::MHTTP->bootstrap($VERSION);


#  the supported request headers
  my $headers = {
                   'Accept-Encoding' => '0',
                   'Accept-Language' => '1',
                   'Connection'      => '2',
                   'Cookie'          => '3',
                   'Host'            => '4',
                   'User-Agent'      => '5',
                   'Authorization'   => '6',
                   'Accept'          => '7',
                   'SOAPAction'      => '8',
                   'Content-Type'    => '9',
                   'Cache-control'   => '10',
                   'Cache-Control'   => '10',
                   'Accept-Charset'  => '11',
                   'Pragma'          => '12',
                   'Referrer'        => '13',
                   'Referer'         => '13',
                   'Keep-Alive'      => '14',
                   'If-Modified-Since' => '15',
                   'Content-type'    => '16',
		 };



=head1 NAME

HTTP::MHTTP - this library provides reasonably low level access to the HTTP protocol, for perl.  This does not replace LWP (what possibly could :-) but is a cut for speed.
It also supports all of HTTP 1.0, so you have GET, POST, PUT, HEAD, and DELETE.

=head1 SYNOPSIS

 use HTTP::MHTTP;
 
 http_init();
 
 http_add_headers(
               'User-Agent' => 'DVSGHTTP1/1',
               'Accept-Language' => 'en-gb',
               'Connection' => 'Keep-Alive',
                   );
 if (http_call("GET", "http://localhost")){
   if (http_status() == 200 ){
     print http_response();
   } else {
     print "MSG: ".http_reason();
   }
 } else {
   print "call failed \n";
 }


=head1 DESCRIPTION

A way faster http access library that uses C extension based on mhttp
to do the calls.

=head2 http_init()

initialise the mhttp library - must be called once to reset all internals,
use http_reset() if you don't need to reset your headers before the next call.


=head2 http_reset()

reset the library internals for everything except the headers specified 
previously, and the debug switch.  Call http_init() if you need to reset
everything.


=head2 switch_debug()

  switch_debug(<0 || 1>)

Toggle the internal debugging on and off by passing either > 1 or 0.


=head2 http_add_headers()

  http_add_headers(
                 'User-Agent' => 'HTTP-MHTTP1/0',
                 'Host' => 'localhost',
                 'Accept-Language' => 'en-gb',
                );

pass in header/value pairs that will be set on the next http_call().


=head2 http_body()

  http_body("this is the body");

Set the body of the next request via http_call().


=head2 http_call()

  my $rc = http_call("GET", "http://localhost");

Do an http request.  Returns either 0 or 1 depending on whether the call was 
successful - remember to still check the http_status() code though.


=head2 http_status()

Returns the last status code.


=head2 http_reason()

Returns the last reason code.


=head2 http_headers()

Returns the headers of the last call, as a single string.


=head2 http_split_headers()

Returns the split out hash of headers of the last call.  Returns the 
hash reference.


=head2 http_response_length()

Returns the length of the body of the last call.


=head2 http_response()

Returns the body of the last call.


=head2 basic_authorization()

  my $pass = basic_authorization($user, $password);

Construct the basic authorization value to be passed in an "Authorization"
header.


=head1 COPYRIGHT

Copyright (c) 2003, Piers Harding. All Rights Reserved.
This module is free software. It may be used, redistributed
and/or modified under the same terms as Perl itself.

=head1 AUTHOR

Piers Harding, piers@ompa.net.


=head1 SEE ALSO

perl(1)

=cut


# export the open command, and initialise http::mhttp
my @export_ok = ("http_reset", "http_init", "http_add_headers", "http_status", "http_reason", "http_call", "http_headers", "http_split_headers", "http_body", "http_response", "basic_authorization", "switch_debug", "http_response_length" );
sub import {

  my ( $caller ) = caller;

  my ($me, $debug) = @_;

  no strict 'refs';
  foreach my $sub ( @export_ok ){
    *{"${caller}::${sub}"} = \&{$sub};
  }

}


sub http_add_headers {
  my $hdrs = { @_ };
  foreach my $header ( keys %$hdrs ){
    if ( exists $headers->{$header} ){
      add_header($header.": ".$hdrs->{$header});
    } else {
      warn "Invalid header specified: $header - $hdrs->{$header} \n";
    }
  }
}


sub http_split_headers {

  my $headers = {};
  foreach my $h (split(/\n/,http_headers())){
    next unless $h =~ /:/;
    my ($hdr,$val) = $h =~ /^(.*?):\s(.*?)$/;
    $headers->{$hdr} = $val;
  }
  return $headers;

}


sub basic_authorization{
  my ( $user, $passwd ) = @_;
  return "Basic ".encode_base64( $user.':'.$passwd, "" );
}

1;
