'use strict';

angular.
  module('crankApp').
  component('crankIssuerView', {
    templateUrl: 'app/issuer/crank-issuer-view.template.html',
    controller: function CrankIssuerListController($http, $routeParams, config) {
      this.status = false;
      this.issuerdn = $routeParams.issuerdn;
    }
  });
