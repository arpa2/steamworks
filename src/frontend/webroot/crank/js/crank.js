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
        otherwise('/certificates');
    }
  ]);
