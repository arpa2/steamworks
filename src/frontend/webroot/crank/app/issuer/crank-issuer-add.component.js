'use strict';

angular.
  module('crankApp').
  component('crankIssuerAdd', {
    templateUrl: 'app/issuer/crank-issuer-add.template.html',
    controller: function CrankIssuerAddController($http, $routeParams, config) {
      this.status = false;
      this.issuerdn = undefined;
    }
  });
