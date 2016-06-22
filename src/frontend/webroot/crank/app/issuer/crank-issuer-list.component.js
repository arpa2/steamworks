'use strict';

angular.
  module('crankApp').
  component('crankIssuerList', {
    templateUrl: 'app/issuer/crank-issuer-list.template.html',
    controller: function CrankIssuerListController($http, config) {
      this.status = false;
      this.issuers = [];

      var self = this;
      $http.post(config.basecgi, {
        verb: 'search',
        base: 'dc=example,dc=com',
        filter: 'objectClass=tlsPoolTrustedIssuer'}).
      then(
        function(response) {
          self.issuers = response.data;
          self.status = true;
        },
        function(response) {
          self.issuers = [];
          self.status = false;
      })

    }
  });
