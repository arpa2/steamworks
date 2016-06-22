'use strict';

var app = angular.module('crankApp', ['ngRoute']);

app.constant('config', {basecgi: '/cgi-bin/'});

app.config(['$locationProvider', '$routeProvider',
    function config($locationProvider, $routeProvider) {
      $locationProvider.hashPrefix('!');

      $routeProvider.
        when('/certificates', {
          template: '<crank-certificate-list></crank-certificate-list>'
        }).
        when('/certificates/view/:certdn', {
          template: '<p>Derp</p>'
        }).
        when('/certificates/add', {
          template: '<p>Add</p>'
        }).
        when('/issuers', {
          template: '<crank-issuer-list></crank-issuer-list>'
        }).
        when('/issuers/view/:issuerdn', {
          template: '<crank-issuer-view></crank-issuer-view>'
        }).
        when('/issuers/remove/:issuerdn', {
          template: '<crank-issuer-remove></crank-issuer-remove>'
        }).
        when('/issuers/add', {
          template: '<crank-issuer-add></crank-issuer-add>'
        }).
        otherwise('/issuers');
    }
  ]);
