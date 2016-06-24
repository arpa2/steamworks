'use strict';

angular.
  module('crankApp').
  component('crankIssuerView', {
    templateUrl: 'app/issuer/crank-issuer-view.template.html',
    controller: function CrankIssuerListController($http, $routeParams, config) {
      this.status = false;
      this.issuerdn = $routeParams.issuerdn;
      this.issuerdata = undefined;
      
      var self = this;
      $http.post(config.basecgi, {
        verb: 'search',
        base: this.issuerdn,
        filter: 'objectClass=tlsPoolTrustedIssuer'}).
      then(
        function(response) {
          self.issuerdata = response.data[self.issuerdn];
          self.status = true;
        },
        function(response) {
          self.issuerdata = undefined;
          self.status = false;
      })

    }
  });
