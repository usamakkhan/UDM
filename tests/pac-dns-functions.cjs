'use strict';
module.exports = [
  ['literal', 'dnsResolve("127.0.0.1")', '127.0.0.1'],
  ['localhost', 'dnsResolve("localhost")', '127.0.0.1'],
  ['resolvable', 'isResolvable("localhost")', true],
  ['nul-rejected', 'dnsResolve("localhost\\0.example.test")', null],
  ['empty-rejected', 'dnsResolve("")', null],
  ['long-rejected', 'dnsResolve("a".repeat(256))', null],
  ['convert-zero', 'convert_addr("0.0.0.0")', 0],
  ['convert-loopback', 'convert_addr("127.0.0.1")', 2130706433],
  ['convert-signed-word', 'convert_addr("255.255.255.255")', -1],
  ['convert-octet-overflow', 'convert_addr("256.0.0.1")', null],
  ['convert-missing-octet', 'convert_addr("127.1")', null],
  ['convert-empty-octet', 'convert_addr("127..0.1")', null],
  ['convert-trailing-junk', 'convert_addr("127.0.0.1junk")', null],
  ['subnet-match', 'isInNet("192.0.2.172", "192.0.2.0", "255.255.255.0")', true],
  ['subnet-mismatch', 'isInNet("192.0.3.172", "192.0.2.0", "255.255.255.0")', false],
  ['subnet-exact', 'isInNet("192.0.2.172", "192.0.2.172", "255.255.255.255")', true],
  ['subnet-resolved', 'isInNet("localhost", "127.0.0.0", "255.0.0.0")', true],
  ['subnet-zero-mask', 'isInNet("203.0.113.9", "0.0.0.0", "0.0.0.0")', true],
  ['subnet-bad-mask', 'isInNet("127.0.0.1", "127.0.0.0", "999.0.0.0")', false],
].map(([name,expression,expected])=>({
  name:'dns-'+name,
  source:'function FindProxyForURL(){return String('+expression+')}',
  value:String(expected)
})).concat([{
  name:'dns-lookup-budget',
  source:'function FindProxyForURL(){for(var i=0;i<33;++i)dnsResolve("127.0.0.1");return "DIRECT"}',
  error:/lookup limit/
}]);
