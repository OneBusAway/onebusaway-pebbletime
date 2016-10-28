var servers = {
  // 'url':{'key':ABCD, 'lat':123, 'lon':-123}
  // Puget Sound
  'http://api.pugetsound.onebusaway.org': {
    'key':'###YOUR KEY HERE###',
    'lat':47.221315,
    'lon':-122.4051325
  },
  // Atlanta, Georgia
  'http://atlanta.onebusaway.org/api': {
    'key':'###YOUR KEY HERE###',
    'lat':33.7901797681045,
    'lon':-84.39483216212469
  },
  // GoHart - Tampa Bay
  'http://api.tampa.onebusaway.org/api': {
    'key':'###YOUR KEY HERE###',
    'lat':27.976910500000002,
    'lon':-82.445851
  },
  // New York (MTA) - http://bustime.mta.info/wiki/Developers/
  'http://bustime.mta.info': {
    'key':'###YOUR KEY HERE###',
    'lat':40.707678,
    'lon':-74.017681
  },
  // Rogue Valley, OR
  'http://oba.rvtd.org:8080/onebusaway-api-webapp': {
    'key':'###YOUR KEY HERE###',
    'lat':42.309802394046,
    'lon':-122.8202635
  },
  // // Washington, D.C. (WMATA)
  'http://buseta.wmata.com/onebusaway-api-webapp': {
    'key':'###YOUR KEY HERE###',
    'lat':38.8950925,
    'lon':-77.059196
  },
  // // York (YRT)
  'http://oba.yrt.ca': {
    'key':'###YOUR KEY HERE###',
    'lat':44.0248945,
    'lon':-79.43752
  },
  // San Diego MTS
  'http://realtime.sdmts.com/api': {
    'key':'###YOUR KEY HERE###',
    'lat':32.9002948657295,
    'lon':-116.73142875280399
  }
}

module.exports.servers = servers;