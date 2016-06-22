'use strict';

angular.
  module('crankApp').
  component('crankIssuerRemove', {
    templateUrl: 'app/issuer/crank-issuer-remove.template.html',
    controller: function CrankIssuerRemoveController($http, $routeParams, config) {
      this.status = false;
      this.issuerdn = $routeParams.issuerdn;
    }
  });
