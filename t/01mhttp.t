use strict;
use Test;
BEGIN { plan tests => 9 }

use HTTP::MHTTP;

ok(1);
ok(test2());
ok(test3());
ok(test4());
ok(test5());
ok(test6());
ok(test7());
ok(test8());
ok(test9());


sub test2 {
  http_init();
  switch_debug(1) if $ENV{'DEBUG'};
  return 1;
}

sub test3 {
  http_add_headers(
                   'User-Agent' => 'DVSGHTTP1/0',
                   'Host' => 'localhost',
                   'Accept-Language' => 'en-gb',
                   'Connection' => 'Keep-Alive',
                 );
  return 1;
}

sub test4 {
  return http_call('GET', 'http://www.piersharding.com/');
}

sub test5 {
  #warn "status: ".http_status()."\n";
  return http_status() == 200 ? 1 : 0;
}

sub test6 {
  #warn "response: ".http_response()."\n";
  my @a = split(/\n/,http_response());
  return @a > 0 ? 1 : 0;
}

sub test7 {
  #warn "reason: ".http_reason()."\n";
  return length(http_reason()) > 0 ? 1 : 0;
}

sub test8 {
  #warn "headers: ".http_headers()."\n";
  my @a = split(/\n/,http_headers());
  return @a > 0 ? 1 : 0;
}

sub test9 {
  http_init();
  switch_debug(1) if $ENV{'DEBUG'};
  http_set_protocol(1);
  http_add_headers(
                    'User-Agent' => 'MHTTP1/0',
                    'Host' => 'www.piersharding.com',
                    'Accept-Language' => 'en-gb',
                    'Connection' => 'Keep-Alive',
                  );
  for (1..3){
    http_reset();
    my $rc = http_call('GET', 'http://www.piersharding.com/');
    return 0 unless $rc;
    #warn "Status: ".http_status()."\n";
  }
  return 1;
}
