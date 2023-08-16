'use strict';

const proxyPath = '/api';

module.exports = function (app) {
  let proxy = require('http-proxy').createProxyServer({});

  proxy.on('error', function (err, req) {
    console.error(err, req.url);
  });

  app.use(proxyPath, function (req, res) {
    req.url = proxyPath + '/' + req.url;
    proxy.web(req, res, { target: 'http://localhost:8000' });
  });
};
