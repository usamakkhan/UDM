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
(function () {
  var weekdays = ['SUN','MON','TUE','WED','THU','FRI','SAT'];
  var months = ['JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC'];
  function between(value, first, last) {
    return first <= last ? value >= first && value <= last : value >= first || value <= last;
  }
  function integer(value, low, high) {
    return Number.isInteger(value) && value >= low && value <= high;
  }
  function clock(args) {
    var utc = args[args.length - 1] === 'GMT';
    if (utc) args.pop();
    var now = new Date(), prefix = utc ? 'getUTC' : 'get';
    return {day:now[prefix+'Date'](), month:now[prefix+'Month'](), year:now[prefix+'FullYear'](),
      weekday:now[prefix+'Day'](), hour:now[prefix+'Hours'](), minute:now[prefix+'Minutes'](), second:now[prefix+'Seconds']()};
  }
  globalThis.weekdayRange = function () {
    var args = Array.from(arguments), now = clock(args);
    if (args.length !== 1 && args.length !== 2) return false;
    var first = weekdays.indexOf(args[0]), last = weekdays.indexOf(args[args.length-1]);
    return first >= 0 && last >= 0 && between(now.weekday, first, last);
  };
  globalThis.timeRange = function () {
    var args = Array.from(arguments), now = clock(args), n = args.length;
    if (n === 1 || n === 2) {
      if (!args.every(function (v) {return integer(v,0,23);})) return false;
      // The two-hour form is ordered, matching Chromium's hour comparison.
      return now.hour >= args[0] && now.hour <= args[n-1];
    }
    if (n !== 4 && n !== 6) return false;
    var half = n/2;
    for (var i=0; i<n; ++i) if (!integer(args[i],0,i%half === 0 ? 23 : 59)) return false;
    var start = args[0]*3600 + args[1]*60 + (half === 3 ? args[2] : 0);
    var end = args[half]*3600 + args[half+1]*60 + (half === 3 ? args[5] : 59);
    return between(now.hour*3600 + now.minute*60 + now.second, start, end);
  };
  globalThis.dateRange = function () {
    var args = Array.from(arguments), now = clock(args), n = args.length;
    function kind(value) {
      if (months.indexOf(value) >= 0) return 'month';
      if (integer(value,1,31)) return 'day';
      if (integer(value,32,9999)) return 'year';
      return '';
    }
    function scalar(value, type) {return type === 'month' ? months.indexOf(value) : value;}
    if (n === 1 || n === 2) {
      var type = kind(args[0]);
      if (type && kind(args[n-1]) === type)
        return between(now[type], scalar(args[0],type), scalar(args[n-1],type));
      // Also accept the documented exact day/month form.
      return n === 2 && type === 'day' && kind(args[1]) === 'month' &&
        now.day === args[0] && now.month === months.indexOf(args[1]);
    }
    if (n !== 4 && n !== 6) return false;
    var half = n/2, first, last, current;
    if (half === 2 && kind(args[0]) === 'day' && kind(args[1]) === 'month' &&
        kind(args[2]) === 'day' && kind(args[3]) === 'month') {
      first=months.indexOf(args[1])*32+args[0]; last=months.indexOf(args[3])*32+args[2];
      current=now.month*32+now.day;
    } else if (half === 2 && kind(args[0]) === 'month' && kind(args[1]) === 'year' &&
               kind(args[2]) === 'month' && kind(args[3]) === 'year') {
      first=args[1]*12+months.indexOf(args[0]); last=args[3]*12+months.indexOf(args[2]);
      current=now.year*12+now.month;
    } else if (half === 3 && kind(args[0]) === 'day' && kind(args[1]) === 'month' && kind(args[2]) === 'year' &&
               kind(args[3]) === 'day' && kind(args[4]) === 'month' && kind(args[5]) === 'year') {
      first=args[2]*512+months.indexOf(args[1])*32+args[0];
      last=args[5]*512+months.indexOf(args[4])*32+args[3];
      current=now.year*512+now.month*32+now.day;
    } else return false;
    return between(current,first,last);
  };
})();
)UDMPAC";
}
