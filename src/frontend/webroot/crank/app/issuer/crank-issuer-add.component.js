'use strict';

angular.
  module('crankApp').
  component('crankIssuerAdd', {
    templateUrl: 'app/issuer/crank-issuer-view.template.html',
    controller: function CrankIssuerAddController($http, $routeParams, config) {
      this.status = true;
      this.issuerdn = undefined;
    }
  });
