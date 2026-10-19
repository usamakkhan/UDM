#pragma once
namespace udm {
// Original implementation of the portable hostname/string PAC helpers.
// Evaluate in the same bounded runtime as the supplied PAC script.
inline constexpr char pacStringFunctions[] = R"UDMPAC(
function isPlainHostName(host) {
  return String(host).indexOf('.') === -1;
}
function dnsDomainIs(host, domain) {
  return String(host).endsWith(String(domain));
}
function localHostOrDomainIs(host, hostDom) {
  host = String(host); hostDom = String(hostDom);
  return host === hostDom ||
    (isPlainHostName(host) && hostDom.startsWith(host + '.'));
}
function dnsDomainLevels(host) {
  host = String(host);
  var count = 0;
  for (var i = 0; i < host.length; ++i) if (host[i] === '.') ++count;
  return count;
}
function convert_addr(address) {
  var parts = String(address).split('.');
  if (parts.length !== 4) return null;
  var value = 0;
  for (var i = 0; i < 4; ++i) {
    if (!/^[0-9]{1,3}$/.test(parts[i])) return null;
    var octet = Number(parts[i]);
    if (octet > 255) return null;
    value = (value << 8) | octet;
  }
  return value;
}
function isResolvable(host) {
  return dnsResolve(host) !== null;
}
function isInNet(host, pattern, mask) {
  var network = convert_addr(pattern), bits = convert_addr(mask);
  if (network === null || bits === null) return false;
  var address = convert_addr(host);
  if (address === null) {
    var resolved = dnsResolve(host);
    if (resolved === null) return false;
    address = convert_addr(resolved);
  }
  return address !== null && (address & bits) === (network & bits);
}
function shExpMatch(text, pattern) {
  text = String(text); pattern = String(pattern);
  // '*' and '?' are the portable PAC glob syntax. Other characters are
  // literal: regex punctuation must not change the routing condition.
  var t = 0, p = 0, star = -1, retry = 0;
  while (t < text.length) {
    if (p < pattern.length && pattern[p] === '*') {
      star = p++; retry = t;
    } else if (p < pattern.length && (pattern[p] === '?' || pattern[p] === text[t])) {
      ++t; ++p;
    } else if (star !== -1) {
      p = star + 1; t = ++retry;
    } else return false;
  }
  while (p < pattern.length && pattern[p] === '*') ++p;
  return p === pattern.length;
}
)UDMPAC";
}
